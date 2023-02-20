#include "LpmsIG1.h"
#include "LpUtil.h"

using namespace std;
#ifdef _WIN32
IG1I* APIENTRY IG1Factory()
{
    return new IG1();
}
#else
IG1I* IG1Factory()
{
    return new IG1();
}
#endif
/////////////////////////////////////////////
// Constructor/Destructor
/////////////////////////////////////////////
IG1::IG1()
{
#ifdef _WIN32
    portno = 0;
#else
    portno = "";
#endif
    baudrate = LPMS_UART_BAUDRATE_921600;

    connectionMode = Serial::MODE_VCP;
    connectionInterface = CONNECTION_INTERFACE_RS232_USB;
    ctrlGpio = -1;
    ctrlGpioToggleWaitMs = DEFAULT_GPIO_TOGGLE_WAIT_MS;

    autoReconnect = true;
    reconnectCount = 0;
    
    startupSensorMode = SENSOR_MODE_STREAMING;
    currentSensorMode = startupSensorMode;
}

IG1::IG1(const IG1 &obj)
{
    //TODO
}

IG1::~IG1()
{
    //incomingDataRate = 0.01f;
    //if (sensorStatus != STATUS_DISCONNECTED || 
    //    sensorStatus != STATUS_CONNECTION_ERROR)
    disconnect();
}

void IG1::init()
{
    // LPBus
    packet.reset();
    ackReceived = false;
    nackReceived = false;
    dataReceived = false;

    // Internal thread
    connectionState = CONNECTION_STATE_DISCONNECTED;
    mmDataFreq.reset();
    mmDataIdle.reset();
    mmUpdating.reset();

    // Sensor
    currentSensorMode = startupSensorMode;
    sensorStatus = STATUS_DISCONNECTED;
    errMsg = "";
    timeoutThreshold = TIMEOUT_IDLE;
    memset(incomingData, 0, INCOMING_DATA_MAX_LENGTH);

    // Stats
    incomingDataRate = 0;

    // Command queue
    mLockCommandQueue.lock();
    while (!commandQueue.empty())
    {
        commandQueue.pop();
    }
    mLockCommandQueue.unlock();
    mmCommandTimer.reset();
    lastSendCommandTime = 0;

    // Response
    mLockSensorResponseQueue.lock();
    while (!sensorResponseQueue.empty())
    {
        sensorResponseQueue.pop();
    }
    mLockSensorResponseQueue.unlock();

    // Info
    hasNewInfo = false;
    sensorInfo.reset();

    // Sensor settings
    hasNewSettings = false;
    sensorSettings.reset();
    mmTransmitDataRegisterStatus.reset();
    useNewChecksum = true;

    // Imu data
    sensorDataQueueSize = SENSOR_DATA_QUEUE_SIZE;
    clearSensorDataQueue();
    latestImuData.reset();

    // Gps data
    mLockGpsDataQueue.lock();
    while (!gpsDataQueue.empty())
    {
        gpsDataQueue.pop();
    }
    mLockGpsDataQueue.unlock();
    latestGpsData.reset();

    // Firmware update
    firmwarePages = 0;
    firmwarePageSize = FIRMWARE_PACKET_LENGTH;
    firmwareRemainder = 0;
    updateCommand = 0;
    memset(cBuffer, 0, 1024);

    // Data saving
    isDataSaving = false;
    savedImuDataBuffer.reserve(SAVE_DATA_LIMIT);
    savedImuDataCount = 0;
    savedImuDataBuffer.clear();

    savedGpsDataBuffer.reserve(SAVE_DATA_LIMIT);
    savedGpsDataCount = 0;
    savedGpsDataBuffer.clear();
}

/////////////////////////////////////
// Connection
/////////////////////////////////////
void IG1::setPCBaudrate(int baud)
{
    baudrate = baud;
    log.d(TAG, "Baudrate: %d\n", baudrate);
}

#ifdef _WIN32
void IG1::setPCPort(int port)
{
    portno = port;
    log.d(TAG, "COM: %d\n", port);
}
#else
void IG1::setPCPort(string port)
{
    portno = port;
    log.d(TAG, "COM: %s\n", port);
}
#endif

#ifdef _WIN32
int IG1::connect(int _portno, int _baudrate)
#else
int IG1::connect(string _portno, int _baudrate)
#endif
{
    if (sensorStatus == STATUS_CONNECTING || 
        sensorStatus == STATUS_CONNECTED  ||
        sensorStatus == STATUS_UPDATING)
    {
        log.i(TAG, "Another connection established\n");
    } 
    else 
    {
        connectionMode = Serial::MODE_VCP;
        portno = _portno;
        baudrate = _baudrate;
        t = new std::thread(&IG1::updateData, this);
        this_thread::sleep_for(chrono::milliseconds(200));
    }
    return sensorStatus;
}

#ifdef _WIN32
int IG1::connect(std::string sensorName ,int _baudrate)
{ 
    if (sensorStatus == STATUS_CONNECTING ||
        sensorStatus == STATUS_CONNECTED ||
        sensorStatus == STATUS_UPDATING)
    {
        log.i(TAG, "Another connection established\n");
    }
    else
    {
        connectionMode = Serial::MODE_USBEXPRESS;
        sensorId = sensorName;
        baudrate = _baudrate;
        t = new std::thread(&IG1::updateData, this);
    }
    return sensorStatus;
}
#endif

void IG1::cleanup()
{
    if (ctrlGpio >= 0)
        gpioUnexport(ctrlGpio);
    ctrlGpio = -1;
}

bool IG1::disconnect()
{
    if (isStopThread)
    {
        log.i(TAG, "%s already disconnected\n", portno.c_str());
        return false;
    }

    isStopThread = true;

    if (t != NULL && t->joinable())
        t->join();
    t = NULL;
    if (sp.isConnected())
    {
        sp.close();
    }
#ifdef _WIN32
    log.i(TAG, "COM:%d disconnected\n", portno);
#else
    log.i(TAG, "%s disconnected\n", portno.c_str());
#endif

    sensorStatus = STATUS_DISCONNECTED;
    return true;
}

int IG1::getReconnectCount()
{
    return reconnectCount;
}

void IG1::setConnectionInterface(int interface)
{
    connectionInterface = interface;
}

void IG1::setControlGPIOForRs485(int gpio)
{
    if (gpio < 0)
        return;

    ctrlGpio = gpio;
}

 void IG1::setControlGPIOToggleWaitMs(unsigned int ms)
 {
    ctrlGpioToggleWaitMs = ms;
 }

/////////////////////////////////////
// Commands:
/////////////////////////////////////
void IG1::commandGotoCommandMode()
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    clearCommandQueue();
    triggerRS485CommandMode();
    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(IG1Command(GET_SENSOR_STATUS, WAIT_IGNORE));
    currentSensorMode = SENSOR_MODE_COMMAND;
}

void IG1::commandGotoStreamingMode()
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    clearCommandQueue();
    triggerRS485CommandMode();
    addCommandQueue(IG1Command(GOTO_STREAM_MODE));

    // Note 20200708
    // Not obtaining sensor status for IG1-RS485, otherwise this following
    // command will put IG1-RS485 back into command mode
    if (connectionInterface != CONNECTION_INTERFACE_RS485)
        addCommandQueue(IG1Command(GET_SENSOR_STATUS, WAIT_IGNORE));
    currentSensorMode = SENSOR_MODE_STREAMING;
}

//Sensor ID
void IG1::commandGetSensorID(void)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(IG1Command(GET_IMU_ID, WAIT_IGNORE));
    
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
    //else if(connectionInterface != CONNECTION_INTERFACE_RS485 || autoReconnect)
    //   addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandSetSensorID(uint32_t id)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    IG1Command cmd1;
    cmd1.command = SET_IMU_ID;
    cmd1.dataLength = 4;
    cmd1.data.i[0] = id;

    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(cmd1);
    addCommandQueue(IG1Command(GET_IMU_ID, WAIT_IGNORE));
    
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

//Sensor Freq
void IG1::commandGetSensorFrequency(void)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(IG1Command(GET_STREAM_FREQ, WAIT_IGNORE));
   
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandSetSensorFrequency(uint32_t freq)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    IG1Command cmd1;
    cmd1.command = SET_STREAM_FREQ;
    cmd1.dataLength = 4;
    cmd1.data.i[0] = freq;

    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(cmd1);
    addCommandQueue(IG1Command(GET_STREAM_FREQ, WAIT_IGNORE));
 
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandGetSensorGyroRange(void)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(IG1Command(GET_GYR_RANGE, WAIT_IGNORE));
   
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandSetSensorGyroRange(uint32_t range)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    IG1Command cmd1;
    cmd1.command = SET_GYR_RANGE;
    cmd1.dataLength = 4;
    cmd1.data.i[0] = range;

    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(cmd1);
    addCommandQueue(IG1Command(GET_GYR_RANGE, WAIT_IGNORE));
  
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandStartGyroCalibration(void)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(IG1Command(START_GYR_CALIBRATION, WAIT_IGNORE));
   
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandGetSensorAccRange(void)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(IG1Command(GET_ACC_RANGE, WAIT_IGNORE));
  
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandSetSensorAccRange(uint32_t range)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    IG1Command cmd1;
    cmd1.command = SET_ACC_RANGE;
    cmd1.dataLength = 4;
    cmd1.data.i[0] = range;

    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(cmd1);
    addCommandQueue(IG1Command(GET_ACC_RANGE, WAIT_IGNORE));
   
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandGetSensorMagRange(void)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(IG1Command(GET_MAG_RANGE, WAIT_IGNORE));
   
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandSetSensorMagRange(uint32_t range)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    IG1Command cmd1;
    cmd1.command = SET_MAG_RANGE;
    cmd1.dataLength = 4;
    cmd1.data.i[0] = range;

    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(cmd1);
    addCommandQueue(IG1Command(GET_MAG_RANGE, WAIT_IGNORE));
  
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandGetSensorUseRadianOutput(void)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();

    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(IG1Command(GET_DEGRAD_OUTPUT, WAIT_IGNORE));
  
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandSetSensorUseRadianOutput(bool b)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    IG1Command cmd1;
    cmd1.command = SET_DEGRAD_OUTPUT;
    cmd1.dataLength = 4;
    cmd1.data.i[0] = b;

    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(cmd1);
    addCommandQueue(IG1Command(GET_DEGRAD_OUTPUT, WAIT_IGNORE));
    
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandStartMagCalibration(void)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();

    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(IG1Command(START_MAG_CALIBRATION, WAIT_IGNORE));
  
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandStopMagCalibration(void)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();

    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(IG1Command(STOP_MAG_CALIBRATION, WAIT_IGNORE));
 
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandGetMagRefrence(void)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();

    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(IG1Command(GET_MAG_REFERENCE, WAIT_IGNORE));
    
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandSetMagRefrence(uint32_t reference)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    IG1Command cmd1;
    cmd1.command = SET_MAG_REFERENCE;
    cmd1.dataLength = 4;
    cmd1.data.i[0] = reference;

    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(cmd1);
    addCommandQueue(IG1Command(GET_MAG_REFERENCE, WAIT_IGNORE));
    
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandGetSensorMagCalibrationTimeout(void)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(IG1Command(GET_MAG_CALIBRATION_TIMEOUT, WAIT_IGNORE));
    
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandSetSensorMagCalibrationTimeout(float second)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    IG1Command cmd1;
    cmd1.command = SET_MAG_CALIBRATION_TIMEOUT;
    cmd1.dataLength = 4;
    cmd1.data.f[0] = second;

    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(cmd1);
    addCommandQueue(IG1Command(GET_MAG_CALIBRATION_TIMEOUT, WAIT_IGNORE));
    
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

//CAN bus
void IG1::commandGetCanStartId(void)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(IG1Command(GET_CAN_START_ID, WAIT_IGNORE));
    
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));

}

void IG1::commandSetCanStartId(uint32_t data)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    IG1Command cmd1;
    cmd1.command = SET_CAN_START_ID;
    cmd1.dataLength = 4;
    cmd1.data.i[0] = data;

    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(cmd1);
    addCommandQueue(IG1Command(GET_CAN_START_ID, WAIT_IGNORE));
   
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandGetCanBaudrate(void)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(IG1Command(GET_CAN_BAUDRATE, WAIT_IGNORE));
  
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));

}

void IG1::commandSetCanBaudrate(uint32_t baudrate)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    IG1Command cmd1;
    cmd1.command = SET_CAN_BAUDRATE;
    cmd1.dataLength = 4;
    cmd1.data.i[0] = baudrate;

    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(cmd1);
    addCommandQueue(IG1Command(GET_CAN_BAUDRATE, WAIT_IGNORE));
   
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}


void IG1::commandGetCanDataPrecision(void)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(IG1Command(GET_CAN_DATA_PRECISION, WAIT_IGNORE));
    
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandSetCanDataPrecision(uint32_t data)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    IG1Command cmd1;
    cmd1.command = SET_CAN_DATA_PRECISION;
    cmd1.dataLength = 4;
    cmd1.data.i[0] = data;

    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(cmd1);
    addCommandQueue(IG1Command(GET_CAN_DATA_PRECISION, WAIT_IGNORE));
   
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandGetCanChannelMode(void)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(IG1Command(GET_CAN_MODE, WAIT_IGNORE));
  
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandSetCanChannelMode(uint32_t data)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    IG1Command cmd1;
    cmd1.command = SET_CAN_MODE;
    cmd1.dataLength = 4;
    cmd1.data.i[0] = data;

    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(cmd1);
    addCommandQueue(IG1Command(GET_CAN_MODE, WAIT_IGNORE));
    
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandGetCanMapping(void)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(IG1Command(GET_CAN_MAPPING, WAIT_IGNORE));
  
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandSetCanMapping(uint32_t map[16])
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();

    IG1Command cmd1;
    cmd1.command = SET_CAN_MAPPING;
    cmd1.dataLength = 16*4;
    memcpy(cmd1.data.i, map, sizeof(map[0])*16);

    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(cmd1);
    addCommandQueue(IG1Command(GET_CAN_MAPPING, WAIT_IGNORE));
   
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandGetCanHeartbeat(void)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(IG1Command(GET_CAN_HEARTBEAT, WAIT_IGNORE));
   
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandSetCanHeartbeat(uint32_t data)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    IG1Command cmd1;
    cmd1.command = SET_CAN_HEARTBEAT;
    cmd1.dataLength = 4;
    cmd1.data.i[0] = data;

    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(cmd1);
    addCommandQueue(IG1Command(GET_CAN_HEARTBEAT, WAIT_IGNORE));
    
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandSetFilterMode(uint32_t data)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    IG1Command cmd1;
    cmd1.command = SET_FILTER_MODE;
    cmd1.dataLength = 4;
    cmd1.data.i[0] = data;

    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(cmd1);
    addCommandQueue(IG1Command(GET_FILTER_MODE, WAIT_IGNORE));
   
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandGetFilterMode(void)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(IG1Command(GET_FILTER_MODE, WAIT_IGNORE));

    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandSetGyroAutoCalibration(bool enable)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    IG1Command cmd1;
    cmd1.command = SET_ENABLE_GYR_AUTOCALIBRATION;
    cmd1.dataLength = 4;
    cmd1.data.i[0] = enable;

    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(cmd1);
    addCommandQueue(IG1Command(GET_ENABLE_GYR_AUTOCALIBRATION, WAIT_IGNORE));
  
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandGetGyroAutoCalibration(void)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(IG1Command(GET_ENABLE_GYR_AUTOCALIBRATION, WAIT_IGNORE));
  
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandSetOffsetMode(uint32_t data)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    IG1Command cmd1;
    cmd1.command = SET_ORIENTATION_OFFSET;
    cmd1.dataLength = 4;
    cmd1.data.i[0] = data;

    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(cmd1);
   
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandResetOffsetMode(void)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    clearCommandQueue();
    triggerRS485CommandMode();
    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(IG1Command(RESET_ORIENTATION_OFFSET, WAIT_IGNORE));
 
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandSaveGPSState(void)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(IG1Command(SAVE_GPS_STATE, WAIT_IGNORE));
   
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandClearGPSState(void)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(IG1Command(CLEAR_GPS_STATE, WAIT_IGNORE));
  
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandSetGpsTransmitData(uint32_t data, uint32_t data1)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    IG1Command cmd1;
    cmd1.command = SET_GPS_TRANSMIT_DATA;
    cmd1.dataLength = 8;
    cmd1.data.i[0] = data;
    cmd1.data.i[1] = data1;

    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(cmd1);
    addCommandQueue(IG1Command(GET_GPS_TRANSMIT_DATA, WAIT_IGNORE));
  
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandGetGpsTransmitData(void)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(IG1Command(GET_GPS_TRANSMIT_DATA, WAIT_IGNORE));
  
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandSetUartBaudRate(uint32_t data)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    IG1Command cmd1;
    cmd1.command = SET_UART_BAUDRATE;
    cmd1.dataLength = 4;
    cmd1.data.i[0] = data;

    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(cmd1);
    addCommandQueue(IG1Command(GET_UART_BAUDRATE, WAIT_IGNORE));
   
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}        

void IG1::commandGetUartBaudRate(void)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(IG1Command(GET_UART_BAUDRATE, WAIT_IGNORE));
    
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandSetUartDataFormat(uint32_t data)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    IG1Command cmd1;
    cmd1.command = SET_UART_FORMAT;
    cmd1.dataLength = 4;
    cmd1.data.i[0] = data;

    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(cmd1);
    addCommandQueue(IG1Command(GET_UART_FORMAT, WAIT_IGNORE));
    
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandGetUartDataFormat(void)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(IG1Command(GET_UART_FORMAT, WAIT_IGNORE));
  
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandSetUartDataPrecision(uint32_t data)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    IG1Command cmd1;
    cmd1.command = SET_LPBUS_DATA_PRECISION;
    cmd1.dataLength = 4;
    cmd1.data.i[0] = data;

    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(cmd1);
    addCommandQueue(IG1Command(GET_LPBUS_DATA_PRECISION, WAIT_IGNORE));
   
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandGetUartDataPrecision(void)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(IG1Command(GET_LPBUS_DATA_PRECISION, WAIT_IGNORE));
   
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandGetSensorInfo()
{
    if (sensorStatus != STATUS_CONNECTING &&
        sensorStatus != STATUS_CONNECTED)
        return;
    
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(IG1Command(GET_SERIAL_NUMBER, WAIT_FOR_SERIAL_NUMBER));
    addCommandQueue(IG1Command(GET_SENSOR_MODEL, WAIT_FOR_SENSOR_MODEL));
    addCommandQueue(IG1Command(GET_FIRMWARE_INFO, WAIT_FOR_FIRMWARE_INFO));
    addCommandQueue(IG1Command(GET_FILTER_VERSION, WAIT_FOR_FILTER_VERSION));
    addCommandQueue(IG1Command(GET_IAP_CHECKSTATUS, WAIT_FOR_IAP_CHECKSTATUS));

    addCommandQueue(IG1Command(GET_IMU_TRANSMIT_DATA, WAIT_FOR_TRANSMIT_DATA_REGISTER));
    addCommandQueue(IG1Command(GET_LPBUS_DATA_PRECISION, WAIT_FOR_LPBUS_DATA_PRECISION));
    addCommandQueue(IG1Command(GET_DEGRAD_OUTPUT, WAIT_FOR_DEGRAD_OUTPUT));

    addCommandQueue(IG1Command(GET_IMU_ID, WAIT_IGNORE));
    addCommandQueue(IG1Command(GET_STREAM_FREQ, WAIT_IGNORE));
    addCommandQueue(IG1Command(GET_ACC_RANGE, WAIT_IGNORE));
    addCommandQueue(IG1Command(GET_GYR_RANGE, WAIT_IGNORE));
    addCommandQueue(IG1Command(GET_ENABLE_GYR_AUTOCALIBRATION, WAIT_IGNORE));
    addCommandQueue(IG1Command(GET_GYR_THRESHOLD, WAIT_IGNORE));
    addCommandQueue(IG1Command(GET_MAG_RANGE, WAIT_IGNORE));
    addCommandQueue(IG1Command(GET_MAG_CALIBRATION_TIMEOUT, WAIT_IGNORE));
    addCommandQueue(IG1Command(GET_FILTER_MODE, WAIT_IGNORE));

    /*
    addCommandQueue(IG1Command(GET_CAN_START_ID, WAIT_IGNORE));
    addCommandQueue(IG1Command(GET_CAN_BAUDRATE, WAIT_IGNORE));
    addCommandQueue(IG1Command(GET_CAN_DATA_PRECISION, WAIT_IGNORE));
    addCommandQueue(IG1Command(GET_CAN_MODE, WAIT_IGNORE));
    addCommandQueue(IG1Command(GET_CAN_MAPPING, WAIT_IGNORE));
    addCommandQueue(IG1Command(GET_CAN_HEARTBEAT, WAIT_IGNORE));
    */

    addCommandQueue(IG1Command(GET_UART_BAUDRATE, WAIT_IGNORE));
    addCommandQueue(IG1Command(GET_UART_FORMAT, WAIT_IGNORE));

    addCommandQueue(IG1Command(GET_GPS_TRANSMIT_DATA, WAIT_IGNORE));

    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandSaveParameters()
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(IG1Command(WRITE_REGISTERS));
   
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandResetFactory()
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(IG1Command(RESTORE_FACTORY_VALUE));
   
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::commandSetTransmitData(uint32_t config)
{
    if (sensorStatus != STATUS_CONNECTED)
    {
        log.e(TAG, "Sensor not connected\n");
        return;
    }
    int previousSensorMode = currentSensorMode;
    
    clearCommandQueue();
    triggerRS485CommandMode();
    IG1Command cmd1;
    cmd1.command = SET_IMU_TRANSMIT_DATA;
    cmd1.dataLength = 4;
    cmd1.data.i[0] = config;

    addCommandQueue(IG1Command(GOTO_COMMAND_MODE));
    addCommandQueue(cmd1);
    addCommandQueue(IG1Command(GET_IMU_TRANSMIT_DATA, WAIT_FOR_TRANSMIT_DATA_REGISTER));
   
    if (previousSensorMode == SENSOR_MODE_STREAMING || autoReconnect)
        addCommandQueue(IG1Command(GOTO_STREAM_MODE));
}

void IG1::sendCommand(uint16_t cmd, uint16_t length, uint8_t* data)
{

    if (data == NULL)
        length = 0;

    uint8_t cmdBuffer[512];
    int idx = 0;
    cmdBuffer[idx++] = 0x3A;
    cmdBuffer[idx++] = 0x00;
    cmdBuffer[idx++] = 0x00;
    cmdBuffer[idx++] = cmd & 0xFF;
    cmdBuffer[idx++] = (cmd >> 8) & 0xFF;
    cmdBuffer[idx++] = length & 0xFF;
    cmdBuffer[idx++] = (length >> 8) & 0xFF;

    uint16_t txLrcCheck = 0;
    //checksum 
    if (useNewChecksum) 
    {
        for (int i = 1; i < 7; i++)
        {
            txLrcCheck += cmdBuffer[i];
        }
    }
    else 
    {
        txLrcCheck = cmd + length;
    }

    if (data != NULL)
    {
        for (int i = 0; i < length; ++i)
        {
            cmdBuffer[idx] = data[i];
            txLrcCheck += data[i];
            idx++;
        }
    }

    cmdBuffer[idx++] = txLrcCheck & 0xFF;
    cmdBuffer[idx++] = (txLrcCheck>>8) & 0xFF;
    cmdBuffer[idx++] = 0x0D;
    cmdBuffer[idx++] = 0x0A;

    if(connectionInterface == CONNECTION_INTERFACE_RS485 && ctrlGpio >= 0)
    {
        gpioSetValue(ctrlGpio, 1); //TX
        this_thread::sleep_for(chrono::milliseconds(ctrlGpioToggleWaitMs));//milliseconds
        
    }
#ifdef _WIN32
    sp.writeData((const char*)cmdBuffer, idx);
#else
    sp.writeData(cmdBuffer, idx);
#endif


    if(connectionInterface == CONNECTION_INTERFACE_RS485 && ctrlGpio >= 0)
    {
        this_thread::sleep_for(chrono::milliseconds(ctrlGpioToggleWaitMs));//milliseconds
        gpioSetValue(ctrlGpio, 0); //RX
    }
}

/////////////////////////////////////
// Sensor interface
/////////////////////////////////////
// General
void IG1::setStartupSensorMode(int mode)
{
    if (mode > SENSOR_MODE_STREAMING)
        mode = SENSOR_MODE_STREAMING;
    startupSensorMode = mode;
}

void IG1::setAutoReconnectStatus(bool b)
{ 
    autoReconnect = b; 

    // reset data idle time if auto reconnect enabled
    if (autoReconnect)
        mmDataIdle.reset();

};

bool IG1::getAutoReconnectStatus(void)
{ 
    return autoReconnect; 
};

int IG1::getStatus() 
{ 
    return sensorStatus; 
};

float IG1::getDataFrequency()
{
    if (incomingDataRate == 0.0f)
        return 0;
    return 1.0f / incomingDataRate;
}

// info
bool IG1::hasInfo()
{
    return hasNewInfo;
}

void IG1::getInfo(IG1InfoI &info)
{
    info = sensorInfo;
    hasNewInfo = false;
}

// settings
bool IG1::hasSettings()
{
    return hasNewSettings;
}

void IG1::getSettings(IG1SettingsI &settings)
{
    //memcpy(&settings.transmitDataConfig, &sensorSettings.transmitDataConfig, sizeof(settings));
    settings = sensorSettings;
    hasNewSettings = false;
}

// response from sensor
int IG1::hasResponse()
{
    return sensorResponseQueue.size();
}

bool IG1::getResponse(std::string &s)
{

    mLockSensorResponseQueue.lock();
    if (sensorResponseQueue.empty())
    {
        mLockSensorResponseQueue.unlock();
        return false;
    }
    s = sensorResponseQueue.front();
    sensorResponseQueue.pop();
    mLockSensorResponseQueue.unlock();

    return true;
}

// Imu data
int IG1::hasImuData()
{
    if (sensorStatus != STATUS_CONNECTED)
        return 0;
    return imuDataQueue.size();
}

bool IG1::getImuData(IG1ImuDataI &sd)
{
    mLockImuDataQueue.lock();
    if (imuDataQueue.empty())
    {
        sd = latestImuData;
        mLockImuDataQueue.unlock();
        return false;
    }
    sd = imuDataQueue.front();
    imuDataQueue.pop();
    mLockImuDataQueue.unlock();
    return true;
}

// gps data
int IG1::hasGpsData()
{
    if (sensorStatus != STATUS_CONNECTED)
        return 0;
    return gpsDataQueue.size();
}

bool IG1::getGpsData(IG1GpsDataI &data)
{

    mLockGpsDataQueue.lock();
    if (gpsDataQueue.empty())
    {
        data = latestGpsData;
        mLockGpsDataQueue.unlock();
        return false;
    }
    data = gpsDataQueue.front();
    gpsDataQueue.pop();
    mLockGpsDataQueue.unlock();
    return true;
}


// Sensor Data
bool IG1::isAccRawEnabled()
{
    if (sensorSettings.transmitDataConfig & TDR_ACC_RAW_OUTPUT_ENABLED)
        return true;
    return false;
}


bool IG1::isAccCalibratedEnabled()
{
    if (sensorSettings.transmitDataConfig & TDR_ACC_CALIBRATED_OUTPUT_ENABLED)
        return true;
    return false;
}

bool IG1::isGyroIRawEnabled()
{
    if (sensorSettings.transmitDataConfig & TDR_GYR0_RAW_OUTPUT_ENABLED)
        return true;
    return false;
}

bool IG1::isGyroIBiasCalibratedEnabled()
{
    if (sensorSettings.transmitDataConfig & TDR_GYR0_BIAS_CALIBRATED_OUTPUT_ENABLED)
        return true;
    return false;
}


bool IG1::isGyroIAlignCalibratedEnabled()
{
    if (sensorSettings.transmitDataConfig & TDR_GYR0_ALIGN_CALIBRATED_OUTPUT_ENABLED)
        return true;
    return false;
}

bool IG1::isGyroIIRawEnabled()
{
    if (sensorSettings.transmitDataConfig & TDR_GYR1_RAW_OUTPUT_ENABLED)
        return true;
    return false;
}

bool IG1::isGyroIIBiasCalibratedEnabled()
{
    if (sensorSettings.transmitDataConfig & TDR_GYR1_BIAS_CALIBRATED_OUTPUT_ENABLED)
        return true;
    return false;
}

bool IG1::isGyroIIAlignCalibratedEnabled()
{
    if (sensorSettings.transmitDataConfig & TDR_GYR1_ALIGN_CALIBRATED_OUTPUT_ENABLED)
        return true;
    return false;
}


bool IG1::isMagRawEnabled()
{
    if (sensorSettings.transmitDataConfig & TDR_MAG_RAW_OUTPUT_ENABLED)
        return true;
    return false;
}

bool IG1::isMagCalibratedEnabled()
{
    if (sensorSettings.transmitDataConfig & TDR_MAG_CALIBRATED_OUTPUT_ENABLED)
        return true;
    return false;
}

bool IG1::isAngularVelocityEnabled()
{
    if (sensorSettings.transmitDataConfig & TDR_ANGULAR_VELOCITY_OUTPUT_ENABLED)
        return true;
    return false;
}

bool IG1::isQuaternionEnabled()
{
    if (sensorSettings.transmitDataConfig & TDR_QUAT_OUTPUT_ENABLED)
        return true;
    return false;
}

bool IG1::isEulerEnabled()
{
    if (sensorSettings.transmitDataConfig & TDR_EULER_OUTPUT_ENABLED)
        return true;
    return false;
}


bool IG1::isLinearAccelerationEnabled() 
{
    if (sensorSettings.transmitDataConfig & TDR_LINACC_OUTPUT_ENABLED)
        return true;
    return false;
}
bool IG1::isPressureEnabled()
{
    if (sensorSettings.transmitDataConfig & TDR_PRESSURE_OUTPUT_ENABLED)
        return true;
    return false;
}

bool IG1::isAltitudeEnabled()
{
    if (sensorSettings.transmitDataConfig & TDR_ALTITUDE_OUTPUT_ENABLED)
        return true;
    return false;
}

bool IG1::isTemperatureEnabled() 
{
    if (sensorSettings.transmitDataConfig & TDR_TEMPERATURE_OUTPUT_ENABLED)
        return true;
    return false;
}

bool IG1::isUseRadianOutput()
{
    return sensorSettings.useRadianOutput;
}

void IG1::setSensorDataQueueSize(unsigned int size)
{
    sensorDataQueueSize = size;

    mLockImuDataQueue.lock();
    while (imuDataQueue.size() > sensorDataQueueSize)
    {
        imuDataQueue.pop();
    }
    mLockImuDataQueue.unlock();

    mLockGpsDataQueue.lock();
    while (gpsDataQueue.size() > sensorDataQueueSize)
    {
        gpsDataQueue.pop();
    }
    mLockGpsDataQueue.unlock();

}

int IG1::getSensorDataQueueSize()
{
    return sensorDataQueueSize;
}

int IG1::getFilePages() 
{ 
    return firmwarePages; 
};

bool IG1::getIsUpdatingStatus() 
{ 
    return connectionState == CONNECTION_STATE_FIRMWARE_UPDATE; 
};

// Error 
std::string IG1::getLastErrMsg() 
{ 
    return errMsg; 
};

void IG1::setVerbose(int level)
{
    log.setVerbose(level);
}

/////////////////////////////////////////////
// Data saving
/////////////////////////////////////////////
bool IG1::startDataSaving()
{
    if (isDataSaving)
    {
        log.d(TAG, "Data save action running\n");
        return false;
    }
    isDataSaving = true;

    // Clear imu data queue
    mLockSavedImuDataQueue.lock();
    savedImuDataBuffer.clear();
    mLockSavedImuDataQueue.unlock();
    savedImuDataCount = 0;

    // Clear gps data queue
    mLockSavedGpsDataQueue.lock();
    savedGpsDataBuffer.clear();
    mLockSavedGpsDataQueue.unlock();
    savedGpsDataCount = 0;
    return true;
}

bool IG1::stopDataSaving()
{
    isDataSaving = false;
    return true;
}

int IG1::getSavedImuDataCount()
{
    return savedImuDataCount;
}

IG1ImuData IG1::getSavedImuData(int i)
{
    IG1ImuData d;
    if (i > savedImuDataBuffer.size())
        return d;
    d = savedImuDataBuffer[i];
    return d;
}

int IG1::getSavedGpsDataCount()
{
    return savedGpsDataCount;
}

IG1GpsData IG1::getSavedGpsData(int i)
{
    IG1GpsData d;
    if (i > savedGpsDataBuffer.size())
        return d;
    d = savedGpsDataBuffer[i];
    return d;
}

/////////////////////////////////////////////////////
// Private
/////////////////////////////////////////////////////
void IG1::triggerRS485CommandMode()
{
    if (connectionInterface == CONNECTION_INTERFACE_RS485)
    {
        
        uint8_t cmdBuffer[512];
        int idx = 0;
        uint8_t dummy = 0x0A;
        cmdBuffer[idx++] = 0x3A;
        cmdBuffer[idx++] = 0x00;
        cmdBuffer[idx++] = 0x00;
        cmdBuffer[idx++] = 0x08;
        cmdBuffer[idx++] = 0x00;
        cmdBuffer[idx++] = 0x00;
        cmdBuffer[idx++] = 0x00;
        cmdBuffer[idx++] = 0x08;
        cmdBuffer[idx++] = 0x00;
        cmdBuffer[idx++] = 0x0D;
        cmdBuffer[idx++] = 0x0A;

        if(ctrlGpio >= 0)
        {
            gpioSetValue(ctrlGpio, 1); //TX
            this_thread::sleep_for(chrono::milliseconds(ctrlGpioToggleWaitMs));//milliseconds
           
        }
        
        for (int i=0; i<10; ++i) 
        {
    #ifdef _WIN32
            sp.writeData((const char*)&dummy, 1);
    #else
            sp.writeData(&dummy, 1);
    #endif
            this_thread::sleep_for(chrono::milliseconds(1));//milliseconds
        }

        //this_thread::sleep_for(chrono::milliseconds(10));//milliseconds
        for (int i=0; i<idx; ++i) 
        {
    #ifdef _WIN32
            sp.writeData((const char*)cmdBuffer+i, 1);
    #else
            sp.writeData(cmdBuffer+i, 1);
    #endif
    
            this_thread::sleep_for(chrono::milliseconds(1));//milliseconds
        }
        


        if(ctrlGpio >= 0)
        {
            this_thread::sleep_for(chrono::milliseconds(ctrlGpioToggleWaitMs));//milliseconds
            gpioSetValue(ctrlGpio, 0); //RX
        }
        
    }
}

void IG1::processCommandQueue()
{
    // Send command
    mLockCommandQueue.lock();
    if (!commandQueue.empty() && mmCommandTimer.measure() > TIMEOUT_COMMAND_TIMER)
    {
        
        if (!commandQueue.front().sent) 
        {
            //IG1Command cmd = commandQueue.front();
            sendCommand(commandQueue.front().command, commandQueue.front().dataLength, commandQueue.front().data.c);
            log.d(TAG, "Sent command: %d\n", commandQueue.front().command);
            commandQueue.front().sent = true;
            
            mmCommandTimer.reset();
            if (commandQueue.front().expectedResponse == WAIT_IGNORE)
            {
                commandQueue.front().processed = true;
            }
        } 
        // Process sent command
        else 
        {
            if (mmCommandTimer.measure() > 200000) // .2sec no response, skip
            {
                if (commandQueue.front().expectedResponse != WAIT_IGNORE)
                {
                    log.e(TAG, "Command timeout. Resending Command : %d Retry: %d\r\n", commandQueue.front().command, ++commandQueue.front().retryCount);
                    sendCommand(commandQueue.front().command, commandQueue.front().dataLength, commandQueue.front().data.c);
                    commandQueue.front().sent = true;
                    mmCommandTimer.reset();
                } 
            
                // Check max resend
                if (commandQueue.front().retryCount > 5) // max 5 retries
                {
                    commandQueue.front().processed = true;
                }
            }
            
        }
        
        if (commandQueue.front().processed)
            commandQueue.pop();
    }
    mLockCommandQueue.unlock();
}

void IG1::processIncomingData()
{
    // Read data
    int readResult = sp.readData(incomingData, INCOMING_DATA_MAX_LENGTH);
    
    // parse data
    if (readResult > 0)
    {
        mmDataIdle.reset();
        //mmTransmitDataRegisterStatus.reset();
        //log.d(TAG, "data: %d\n", readResult);
        parseModbusByte(readResult);

        if (sensorSettings.uartDataFormat == LPMS_UART_DATA_FORMAT_ASCII) {
            parseASCII(readResult);
        }
    }
}

void IG1::clearSensorDataQueue()
{
    mLockImuDataQueue.lock();
    while (!imuDataQueue.empty())
        imuDataQueue.pop();
    mLockImuDataQueue.unlock();
}

void IG1::updateData()
{
    reconnectCount = 0;
    sensorStatus = STATUS_CONNECTING;
    isStopThread = false;

    gpioInit();
    // Starting point for each new connection/reconnection
    do
    {   
        /////////////////////////////////////////////////
        // Reset all connection related parameters
        /////////////////////////////////////////////////
        init();

        /////////////////////////////////////////////////
        // Establish serial connection to sensor
        /////////////////////////////////////////////////
        if (connectionMode == Serial::MODE_VCP)
        {
            sp.setMode(Serial::MODE_VCP);
            if (sp.open(portno, baudrate))
            {
                sensorStatus = STATUS_CONNECTING;
#ifdef _WIN32
                log.d(TAG, "COM:%d connection established\n", portno);
#else
                log.d(TAG, "%s connection established\n", sp.getPortNo().c_str());
#endif
            }
            else
            {
                sensorStatus = STATUS_CONNECTION_ERROR;
                stringstream ss;
#ifdef _WIN32
                ss << "Error connecting to COM: " << portno << "@" << baudrate;
#else
                ss << "Error connecting to: " << portno << "@" << baudrate;
#endif
                errMsg = ss.str();
                log.e(TAG, "%s\n", errMsg.c_str());
            }
        }
        else if (connectionMode == Serial::MODE_USB_EXPRESS)
        {
            sp.setMode(Serial::MODE_USB_EXPRESS);
            if (sp.open(sensorId, baudrate))
            {
                sensorStatus = STATUS_CONNECTING;
                log.d(TAG, "%s connection established\n", sensorId.c_str());
            }
            else
            {
                sensorStatus = STATUS_CONNECTION_ERROR;
                stringstream ss;
                ss << "Error connecting to sensor: " << sensorId << endl;
                errMsg = ss.str();
                log.e(TAG, "%s\n", errMsg.c_str());
            }
        }
        else 
        {
            sensorStatus = STATUS_CONNECTION_ERROR;
            log.e(TAG, "Unknown connection mode");
        }


        /////////////////////////////////////////////////
        //  Sensor communication
        /////////////////////////////////////////////////
        if (sensorStatus != STATUS_CONNECTION_ERROR)
        {
            // Initialize 
            int TDRRetryCount = 0;

            mmDataFreq.reset();
            mmDataIdle.reset();
            mmCommandTimer.reset();

            //commandGetSensorInfo();

            sensorStatus = STATUS_CONNECTING;
            connectionState = CONNECTION_STATE_GET_SENSOR_INFO;

            while (!isStopThread)
            {
                //////////////////////////////
                // Break point
                //////////////////////////////
                // Read data
                if (!sp.isConnected()) 
                {
                    sensorStatus = STATUS_CONNECTION_ERROR;
                }

                if (sensorStatus == STATUS_CONNECTION_ERROR)
                    break;

                // Only check data timeout if autoreconnect is enabled
                if (autoReconnect && mmDataIdle.measure() > timeoutThreshold) // 5 secs no data
                {
                    errMsg = "Data timeout";
                    sensorStatus = STATUS_DATA_TIMEOUT;
                    break;
                }

                //////////////////////////////
                // Connection state
                //////////////////////////////
                switch (connectionState)
                {
                    case CONNECTION_STATE_GET_SENSOR_INFO:
                    {
                        mmDataIdle.reset();
                        sensorStatus = STATUS_CONNECTING;
                        connectionState = CONNECTION_STATE_WAITING_SENSOR_INFO;
                        mmTransmitDataRegisterStatus.reset();
                        log.i(TAG, "Get sensor info\n");
                        commandGetSensorInfo();
                        break;
                    }

                    case CONNECTION_STATE_WAITING_SENSOR_INFO:
                    {
                        sensorStatus = STATUS_CONNECTING;
                        // Resend get transmit data if no response after 5 sec
                        if (mmTransmitDataRegisterStatus.measure() > TIMEOUT_TDR_STATUS)
                        {
                            connectionState = CONNECTION_STATE_GET_SENSOR_INFO;
                            log.i(TAG, "Get sensor info timeout, retry: %d\n", ++TDRRetryCount);
                        }

                        if (TDRRetryCount > 3) 
                        {
                            TDRRetryCount = 0;
                            connectionState = CONNECTION_STATE_TDR_ERROR;
                            stringstream ss;
                            ss << "[" + currentDateTime("%Y/%m/%d %H:%M:%S") + "] Error getting transmit data register\n";
                            addSensorResponseToQueue(ss.str());
                        }

                        break;
                    }

                    case CONNECTION_STATE_VALID_SENSOR_INFO:
                    {
                        connectionState = CONNECTION_STATE_CONNECTED;
                        reconnectCount = 0;
                        log.i(TAG, "Sensor connected\n");
                        break;
                    }

                    case CONNECTION_STATE_TDR_ERROR:
                        sensorStatus = STATUS_CONNECTION_ERROR;
                    break;

                    case CONNECTION_STATE_FIRMWARE_UPDATE:
                        sensorStatus = STATUS_CONNECTED;
                    break;

                    case CONNECTION_STATE_CONNECTED:
                        sensorStatus = STATUS_CONNECTED;
                    break;

                    default:
                    break;
                }
                //////////////////////////////
                // Process command queue
                //////////////////////////////
                processCommandQueue();

                //////////////////////////////
                // Process incoming data from sensor
                //////////////////////////////
                processIncomingData();

                

                this_thread::sleep_for(chrono::milliseconds(1));
            } // while (!isStopThread)
        }

        if (sp.isConnected())
        {
            sp.close();
        }
        
        if (sensorStatus == STATUS_DATA_TIMEOUT)
        {
#ifdef _WIN32

            if (connectionMode == Serial::MODE_VCP)
                log.e(TAG, "COM:%d Data timeout\n", portno);
            else if (connectionMode = Serial::MODE_USBEXPRESS)
                log.e(TAG, "%s Data timeout\n", sensorName.c_str());
#else
            log.e(TAG, "%s Data timeout\n", portno.c_str());
#endif
        }
        else if (sensorStatus == STATUS_CONNECTION_ERROR)
        {
#ifdef _WIN32
            if (connectionMode == Serial::MODE_VCP)
                log.e(TAG, "COM:%d Connection error\n", portno);
            else if (connectionMode = Serial::MODE_USBEXPRESS)
                log.e(TAG, "%s Connection error\n", sensorName.c_str());
#else
            log.e(TAG, "%s Connection error\n", portno.c_str());
#endif
        }
        else 
        {
            log.d(TAG, "Update data thread stopped\n");
        }


        if (autoReconnect && !isStopThread)
        {
            reconnectCount++;
            log.i(TAG, "Reconnecting %d\n",  reconnectCount);
            this_thread::sleep_for(chrono::milliseconds(1000));
        }
    } while (autoReconnect && !isStopThread);

    gpioDeinit();
    //t = NULL;
}

bool IG1::parseModbusByte(int n)
{
    unsigned char b;
    for (int i = 0; i < n; ++i)
    {
        b = incomingData[i];

        switch (packet.rxState) 
        {
        case PACKET_START:  
            if (b == BYTE_START)
            {
                packet.rxState = PACKET_ADDRESS0; 
                packet.rawDataIndex = 0;
                packet.cs = 0;
            }
            break;

        case PACKET_ADDRESS0:
            packet.address = b;
            packet.cs += b;
            packet.rxState = PACKET_ADDRESS1;
            break;

        case PACKET_ADDRESS1:
            packet.address += ((unsigned)b * 256);
            packet.cs += b;
            packet.rxState = PACKET_FUNCTION0;
            break;

        case PACKET_FUNCTION0:
            packet.function = b;
            packet.cs += b;
            packet.rxState = PACKET_FUNCTION1;
            break;

        case PACKET_FUNCTION1:
            packet.function += ((unsigned)b * 256);
            packet.cs += b;
            packet.rxState = PACKET_LENGTH0;
            break;

        case PACKET_LENGTH0:
            packet.length = b;
            packet.cs += b;
            packet.rxState = PACKET_LENGTH1;
            break;

        case PACKET_LENGTH1:
            packet.length += ((unsigned)b * 256);
            packet.cs += b;
            if (packet.length > LPPACKET_MAX_BUFFER)
                packet.rxState = PACKET_START;
            else 
            {
                if (packet.length > 0)
                {
                    packet.rxState = PACKET_RAW_DATA;
                    packet.rawDataIndex = 0;
                }
                else
                {
                    packet.rxState = PACKET_LRC_CHECK0;
                }
            }
            break;

        case PACKET_RAW_DATA:
            if (packet.rawDataIndex < packet.length && packet.rawDataIndex < LPPACKET_MAX_BUFFER) 
            {
                packet.data[packet.rawDataIndex++] = b;
                if (packet.rawDataIndex == packet.length)
                    packet.rxState = PACKET_LRC_CHECK0;
            }
            else
                packet.rxState = PACKET_START;

            break;

        case PACKET_LRC_CHECK0:
            packet.chksum = b;
            packet.rxState = PACKET_LRC_CHECK1;
            break;

        case PACKET_LRC_CHECK1:
            packet.chksum += ((unsigned)b * 256);
            packet.rxState = PACKET_END0;
            break;

        case PACKET_END0:
            if (b == BYTE_END0)
                packet.rxState = PACKET_END1;
            else
                packet.rxState = PACKET_START;
            break;

        case PACKET_END1:
            if (b == BYTE_END1) 
            {
                for (int j = 0; j < packet.length; ++j)
                    packet.cs += packet.data[j];
                if (packet.cs == packet.chksum)
                    parseSensorData(packet);
            }

            packet.rxState = PACKET_START;
            break;

        default:
            packet.rxState = PACKET_START;
            break;
        }
    }
    return true;
}

bool IG1::parseASCII(int n)
{
    unsigned char b;
    stringstream ss;
    ss.clear();
    for (int i = 0; i < n; ++i)
    {
        b = incomingData[i];
        ss << b;
    }
    addSensorResponseToQueue(ss.str());
    return false;
}

bool IG1::parseSensorData(const LPPacket &p)
{
    stringstream ss;
    string res("");
    string s("");
    //char binaryFormat[36] = { 0 };
    uint2char i2c;
    float2char f2c;

    switch (p.function)
    {
    /////////////////////////////////
    // Essentials
    /////////////////////////////////
    case REPLY_ACK:
        ackReceived = true;
        mLockCommandQueue.lock();
        if (!commandQueue.empty())
        {
            if (commandQueue.front().expectedResponse == WAIT_FOR_ACKNACK)
                commandQueue.front().processed = true;
        }
        mLockCommandQueue.unlock();
        res = "["+currentDateTime("%Y/%m/%d %H:%M:%S")+"] ACK";
        addSensorResponseToQueue(res);
        log.d(TAG, "Received ack\n");
        break;

    case REPLY_NACK:
        nackReceived = true;
        mLockCommandQueue.lock();
        if (!commandQueue.empty())
        {
            if (commandQueue.front().expectedResponse == WAIT_FOR_ACKNACK)
                commandQueue.front().processed = true;
        }
        mLockCommandQueue.unlock();
        res = "[" + currentDateTime("%Y/%m/%d %H:%M:%S") + "] NACK";
        addSensorResponseToQueue(res);
        log.d(TAG, "Received Nack\n");
        break;

    /////////////////////////////////
    // Sensor data
    /////////////////////////////////
    case GET_IMU_DATA:
    {
        if (connectionState != CONNECTION_STATE_CONNECTED) break;
        float dt = float(mmDataFreq.measure()) / 1000000.0f;

        incomingDataRate = 0.995f*incomingDataRate + 0.005f * dt;
        mmDataFreq.reset();

        mLockImuDataQueue.lock();
        latestImuData.reset();

        if (sensorSettings.uartDataPrecision == LPMS_UART_DATA_PRECISION_FIXED_POINT) 
        {
           latestImuData.setData16bit(sensorSettings.useRadianOutput, sensorSettings.transmitDataConfig, packet.data);
        }
        else 
        {
           latestImuData.setData(sensorSettings.transmitDataConfig, packet.data, packet.length);
        }

        if (imuDataQueue.size() < sensorDataQueueSize) {
            imuDataQueue.push(latestImuData);
        }
        else
        {
            imuDataQueue.pop();
            imuDataQueue.push(latestImuData);
        }
        mLockImuDataQueue.unlock();

        // Data saving
        if (isDataSaving)
        {
            if (savedImuDataCount < SAVE_DATA_LIMIT)
            {
                mLockSavedImuDataQueue.lock();
                savedImuDataBuffer.push_back(latestImuData);
                mLockSavedImuDataQueue.unlock();
                savedImuDataCount++;
            }
        }

        break;
    }

    case GET_GPS_DATA:
    {
        if (connectionState != CONNECTION_STATE_CONNECTED) break;
        //memcpy(&latestGpsData, packet.data, packet.length);
        mLockGpsDataQueue.lock();
        latestGpsData.reset();

        latestGpsData.setData(sensorSettings.gpsTransmitDataConfig[0], sensorSettings.gpsTransmitDataConfig[1], packet.data);

        if (gpsDataQueue.size() < sensorDataQueueSize) 
        {
            gpsDataQueue.push(latestGpsData);
        }
        else
        {
            gpsDataQueue.pop();
            gpsDataQueue.push(latestGpsData);
        }
        mLockGpsDataQueue.unlock();

        // Data saving
        if (isDataSaving)
        {
            if (savedGpsDataCount < SAVE_DATA_LIMIT)
            {
                mLockSavedGpsDataQueue.lock();
                savedGpsDataBuffer.push_back(latestGpsData);
                mLockSavedGpsDataQueue.unlock();
                savedGpsDataCount++;
            }
        }
        break;
    }

    /////////////////////////////////
    // Sensor info
    /////////////////////////////////
    case GET_SENSOR_MODEL:

        mLockCommandQueue.lock();
        if (!commandQueue.empty())
        {
            if (commandQueue.front().expectedResponse == WAIT_FOR_SENSOR_MODEL)
                commandQueue.front().processed = true;
        }
        mLockCommandQueue.unlock();

        s.assign((char*)packet.data, packet.length);
        sensorInfo.deviceName = trimString(s);

        res = "[" + currentDateTime("%Y/%m/%d %H:%M:%S") + "] GET DEVICE NAME: " + sensorInfo.deviceName;
        addSensorResponseToQueue(res);
        hasNewInfo = true;

        log.d(TAG, "Received GET_SENSOR_MODEL: %s\n", sensorInfo.deviceName.c_str());
        break;

    case GET_FIRMWARE_INFO:
        mLockCommandQueue.lock();
        if (!commandQueue.empty())
        {
            if (commandQueue.front().expectedResponse == WAIT_FOR_FIRMWARE_INFO)
                commandQueue.front().processed = true;
        }
        mLockCommandQueue.unlock();

        s.assign((char*)packet.data, packet.length);
        sensorInfo.firmwareInfo = trimString(s);

        res = "[" + currentDateTime("%Y/%m/%d %H:%M:%S") + "] GET FIRMWARE INFO: " + sensorInfo.firmwareInfo;
        addSensorResponseToQueue(res);
        hasNewInfo = true;

        log.d(TAG, "Received GET_FIRMWARE_INFO: %s\n", sensorInfo.firmwareInfo.c_str());

        if (sensorInfo.firmwareInfo.find("IG1-3.1.2") != string::npos
            || sensorInfo.firmwareInfo.find("IG1-3.1.1") != string::npos
            || sensorInfo.firmwareInfo.find("IG1-3.1.0") != string::npos
            || sensorInfo.firmwareInfo.find("IG1-3.0") != string::npos
            )
        {
            useNewChecksum = false;
        }
        else
        {
            useNewChecksum = true;
        }

        break;

    case GET_SERIAL_NUMBER:
        mLockCommandQueue.lock();
        if (!commandQueue.empty())
        {
            if (commandQueue.front().expectedResponse == WAIT_FOR_SERIAL_NUMBER)
                commandQueue.front().processed = true;
        }
        mLockCommandQueue.unlock();

        s.assign((char*)packet.data, packet.length);
        sensorInfo.serialNumber = trimString(s);
        res = "[" + currentDateTime("%Y/%m/%d %H:%M:%S") + "] GET SERIAL NUMBER: " + sensorInfo.serialNumber;
        addSensorResponseToQueue(res);
        hasNewInfo = true;

        log.d(TAG, "Received GET_SERIAL_NUMBER: %s\n", sensorInfo.serialNumber.c_str());

        break;

    case GET_FILTER_VERSION:
        mLockCommandQueue.lock();
        if (!commandQueue.empty())
        {
            if (commandQueue.front().expectedResponse == WAIT_FOR_FILTER_VERSION)
                commandQueue.front().processed = true;
        }
        mLockCommandQueue.unlock();

        s.assign((char*)packet.data, packet.length);
        sensorInfo.filterVersion = trimString(s);
        res = "[" + currentDateTime("%Y/%m/%d %H:%M:%S") + "] GET FILTER VERSION: " + sensorInfo.filterVersion;
        addSensorResponseToQueue(res);
        hasNewInfo = true;

        log.d(TAG, "Received GET_FILTER_VERSION: %s\n", sensorInfo.filterVersion.c_str());
        break;

    case GET_IAP_CHECKSTATUS:
        mLockCommandQueue.lock();
        if (!commandQueue.empty())
        {
            if (commandQueue.front().expectedResponse == WAIT_FOR_IAP_CHECKSTATUS)
                commandQueue.front().processed = true;
        }
        mLockCommandQueue.unlock();

        memcpy(i2c.c, packet.data, packet.length);
        sensorInfo.iapCheckStatus = i2c.int_val;
        res = "[" + currentDateTime("%Y/%m/%d %H:%M:%S") + "] GET IAP CHECKSTATUS: ";
        ss.clear();
        ss << res << sensorInfo.iapCheckStatus;
        addSensorResponseToQueue(res);
        hasNewInfo = true;

        log.d(TAG, "Received GET_IAP_CHECKSTATUS: %s\n", sensorInfo.iapCheckStatus? "Ready":"NA");
        break;

    /////////////////////////////////
    // OpenMAT ID
    /////////////////////////////////
    case GET_IMU_TRANSMIT_DATA:
        mLockCommandQueue.lock();
        if (!commandQueue.empty())
        {
            if (commandQueue.front().expectedResponse == WAIT_FOR_TRANSMIT_DATA_REGISTER)
                commandQueue.front().processed = true;
        }
        mLockCommandQueue.unlock();

        memcpy(i2c.c, packet.data, packet.length);
        sensorSettings.transmitDataConfig = i2c.int_val;

        res = "[" + currentDateTime("%Y/%m/%d %H:%M:%S") + "] GET_IMU_TRANSMIT_DATA: ";
        ss.clear();
        ss << res << sensorSettings.uint32ToBinaryPP(sensorSettings.transmitDataConfig);
        addSensorResponseToQueue(ss.str());
        hasNewSettings = true;
        connectionState = CONNECTION_STATE_VALID_SENSOR_INFO;
        log.d(TAG, "Received GET_IMU_TRANSMIT_DATA\n");
        break;

    case GET_IMU_ID:
        memcpy(i2c.c, packet.data, packet.length);
        sensorSettings.sensorId = i2c.int_val;

        res = "[" + currentDateTime("%Y/%m/%d %H:%M:%S") + "] GET IMU ID: ";
        ss.clear();
        ss << res << sensorSettings.sensorId;
        addSensorResponseToQueue(ss.str());
        hasNewSettings = true;

        log.d(TAG, "Received GET_IMU_ID: %i\n", sensorSettings.sensorId);
        break;


    case GET_STREAM_FREQ: 
    {
        memcpy(i2c.c, packet.data, packet.length);
        sensorSettings.dataStreamFrequency = i2c.int_val;

        res = "[" + currentDateTime("%Y/%m/%d %H:%M:%S") + "] GET STREAM FREQ: ";
        ss.clear();
        ss << res << sensorSettings.dataStreamFrequency;
        addSensorResponseToQueue(ss.str());
        hasNewSettings = true;

        log.d(TAG, "Received GET_STREAM_FREQ: %i\n", sensorSettings.dataStreamFrequency);
        break;
    }

    case GET_DEGRAD_OUTPUT:
        mLockCommandQueue.lock();
        if (!commandQueue.empty())
        {
            if (commandQueue.front().expectedResponse == WAIT_FOR_DEGRAD_OUTPUT)
                commandQueue.front().processed = true;
        }
        mLockCommandQueue.unlock();

        memcpy(i2c.c, packet.data, packet.length);
        sensorSettings.useRadianOutput = i2c.int_val;
        res = "[" + currentDateTime("%Y/%m/%d %H:%M:%S") + "] GET_DEGRAD_OUTPUT: ";
        ss.clear();
        ss << res << sensorSettings.useRadianOutput;
        addSensorResponseToQueue(ss.str());
        hasNewSettings = true;

        log.d(TAG, "Received GET_DEGRAD_OUTPUT: %s\n", sensorSettings.useRadianOutput?"rad":"deg");
        break;

    case GET_GYR_THRESHOLD:
        memcpy(f2c.c, packet.data, packet.length);
        sensorSettings.gyroThreshold = f2c.float_val;

        res = "[" + currentDateTime("%Y/%m/%d %H:%M:%S") + "] GET GYRO THRESHOLD: ";
        ss.clear();
        ss << res << sensorSettings.gyroThreshold;
        addSensorResponseToQueue(ss.str());
        hasNewSettings = true;

        log.d(TAG, "Received GET_GYR_THRESHOLD: %f\n", sensorSettings.gyroThreshold);
        break;

    /////////////////////////////////
    // Filter parameters
    /////////////////////////////////
    case GET_FILTER_MODE:
        memcpy(i2c.c, packet.data, packet.length);
        sensorSettings.filterMode = i2c.int_val;
        res = "[" + currentDateTime("%Y/%m/%d %H:%M:%S") + "] GET FILTER MODE: ";
        ss.clear();
        ss << res << sensorSettings.filterMode;
        addSensorResponseToQueue(ss.str());
        hasNewSettings = true;

        log.d(TAG, "Received GET_FILTER_MODE: %i\n", sensorSettings.filterMode);
        break;

    case GET_ENABLE_GYR_AUTOCALIBRATION:
        memcpy(i2c.c, packet.data, packet.length);
        sensorSettings.enableGyroAutocalibration = i2c.int_val;
        res = "[" + currentDateTime("%Y/%m/%d %H:%M:%S") + "] GET ENABLE GYRO AUTOCALIBRATION: ";
        ss.clear();
        ss << res << sensorSettings.enableGyroAutocalibration;
        addSensorResponseToQueue(ss.str());
        hasNewSettings = true;

        log.d(TAG, "Received GET_ENABLE_GYR_AUTOCALIBRATION: %i\n", sensorSettings.enableGyroAutocalibration);
        break;
        
    /////////////////////////////////
    // Acc
    /////////////////////////////////
    case GET_ACC_RANGE:
        memcpy(i2c.c, packet.data, packet.length);
        sensorSettings.accRange = i2c.int_val;
        res = "[" + currentDateTime("%Y/%m/%d %H:%M:%S") + "] GET ACC RANGE: ";
        ss.clear();
        ss << res << sensorSettings.accRange;
        addSensorResponseToQueue(ss.str());
        hasNewSettings = true;

        log.d(TAG, "Received GET_ACC_RANGE: %i\n", sensorSettings.accRange);
        break;

    /////////////////////////////////
    // Gyro
    /////////////////////////////////
    case GET_GYR_RANGE:
        memcpy(i2c.c, packet.data, packet.length);
        sensorSettings.gyroRange = i2c.int_val;
        res = "[" + currentDateTime("%Y/%m/%d %H:%M:%S") + "] GET GYRO RANGE: ";
        ss.clear();
        ss << res << sensorSettings.gyroRange;
        addSensorResponseToQueue(ss.str());
        hasNewSettings = true;

        log.d(TAG, "Received GET_GYR_RANGE: %i\n", sensorSettings.gyroRange);
        break;

    /////////////////////////////////
    // Mag
    /////////////////////////////////
    case GET_MAG_RANGE:
        memcpy(i2c.c, packet.data, packet.length);
        sensorSettings.magRange = i2c.int_val;
        res = "[" + currentDateTime("%Y/%m/%d %H:%M:%S") + "] GET MAG RANGE: ";
        ss.clear();
        ss << res << sensorSettings.magRange;
        addSensorResponseToQueue(ss.str());
        hasNewSettings = true;

        log.d(TAG, "Received GET_MAG_RANGE: %i\n", sensorSettings.magRange);
        break;

    case GET_MAG_CALIBRATION_TIMEOUT:
        memcpy(f2c.c, packet.data, packet.length);
        sensorSettings.magCalibrationTimeout = f2c.float_val;
        res = "[" + currentDateTime("%Y/%m/%d %H:%M:%S") + "] GET MAG CALIBRATION TIME OUT: ";
        ss.clear();
        ss << res << sensorSettings.magCalibrationTimeout;
        addSensorResponseToQueue(ss.str());
        hasNewSettings = true;

        log.d(TAG, "Received GET_MAG_CALIBRATION_TIMEOUT: %f\n", sensorSettings.magCalibrationTimeout);
        break;
    
    
    //////////////////////////////////////////
    // CAN
    //////////////////////////////////////////
    case GET_CAN_START_ID:
        memcpy(&sensorSettings.canStartId, packet.data, packet.length);
        res = "[" + currentDateTime("%Y/%m/%d %H:%M:%S") + "] GET CAN STARTID: ";
        ss.clear();
        ss << res << sensorSettings.canStartId;
        addSensorResponseToQueue(ss.str());
        hasNewSettings = true;
        break;

    case GET_CAN_BAUDRATE:
        memcpy(&sensorSettings.canBaudrate, packet.data, packet.length);
        res = "[" + currentDateTime("%Y/%m/%d %H:%M:%S") + "] GET CAN BAUDRATE: ";
        ss.clear();
        ss << res << sensorSettings.canBaudrate;
        addSensorResponseToQueue(ss.str());
        hasNewSettings = true;
        break;

    case GET_CAN_DATA_PRECISION:
        memcpy(&sensorSettings.canDataPrecision, packet.data, packet.length);
        res = "[" + currentDateTime("%Y/%m/%d %H:%M:%S") + "] GET CAN DATA PRECISION: ";
        ss.clear();
        ss << res << sensorSettings.canDataPrecision;
        addSensorResponseToQueue(ss.str());
        hasNewSettings = true;
        break;

    case GET_CAN_MODE:
        memcpy(&sensorSettings.canMode, packet.data, packet.length);
        res = "[" + currentDateTime("%Y/%m/%d %H:%M:%S") + "] GET CAN MODE: ";
        ss.clear();
        ss << res << sensorSettings.canMode;
        addSensorResponseToQueue(ss.str());
        hasNewSettings = true;
        break;

    case GET_CAN_MAPPING:
        {
            memcpy(&sensorSettings.canMapping, packet.data, packet.length);

            res = "[" + currentDateTime("%Y/%m/%d %H:%M:%S") + "] GET CAN MAPPING: \n";
            ss.clear();
            ss << res;
            for (int i = 0; i < packet.length; ++i) {
                if ((i + 1) % 4 == 0)
                {
                    memcpy(i2c.c, packet.data + i - 3, 4);
                    ss << i / 4 + 1 << ": " << i2c.int_val;

                    if (packet.length - i > 1)
                    {
                        ss << "\n";
                    }
                }
            }

            addSensorResponseToQueue(ss.str());
            hasNewSettings = true;
            break;
        }

    case GET_CAN_HEARTBEAT:
        memcpy(&sensorSettings.canHeartbeatTime, packet.data, packet.length);

        res = "[" + currentDateTime("%Y/%m/%d %H:%M:%S") + "] GET CAN HEARTBEAT: ";
        ss.clear();
        ss << res << sensorSettings.canHeartbeatTime;
        addSensorResponseToQueue(ss.str());
        hasNewSettings = true;
        break;

    /////////////////////////////////
    // UART/RS232
    /////////////////////////////////
    case GET_UART_BAUDRATE:
        memcpy(&sensorSettings.uartBaudrate, packet.data, packet.length);

        res = "[" + currentDateTime("%Y/%m/%d %H:%M:%S") + "] GET UART BAUDRATE: ";
        ss.clear();
        ss << res << sensorSettings.uartBaudrate;
        addSensorResponseToQueue(ss.str());
        hasNewSettings = true;


        log.d(TAG, "Received GET_UART_BAUDRATE: %d\n", sensorSettings.uartBaudrate);
        break;

    case GET_UART_FORMAT:
        memcpy(&sensorSettings.uartDataFormat, packet.data, packet.length);

        res = "[" + currentDateTime("%Y/%m/%d %H:%M:%S") + "] GET UART FORMAT: ";
        ss.clear();
        ss << res << sensorSettings.uartDataFormat;
        addSensorResponseToQueue(ss.str());
        hasNewSettings = true;

        log.d(TAG, "Received GET_UART_FORMAT: %d\n", sensorSettings.uartDataFormat);
        break;


    case GET_LPBUS_DATA_PRECISION:
        mLockCommandQueue.lock();
        if (!commandQueue.empty())
        {
            if (commandQueue.front().expectedResponse == WAIT_FOR_LPBUS_DATA_PRECISION)
                commandQueue.front().processed = true;
        }
        mLockCommandQueue.unlock();

        memcpy(&sensorSettings.uartDataPrecision, packet.data, packet.length);

        res = "[" + currentDateTime("%Y/%m/%d %H:%M:%S") + "] GET UART LPBUS DATA PRECISION: ";
        ss.clear();
        ss << res << sensorSettings.uartDataPrecision;
        addSensorResponseToQueue(ss.str());
        hasNewSettings = true;

        log.d(TAG, "Received GET_LPBUS_DATA_PRECISION: %d\n", sensorSettings.uartDataPrecision);
        break;

    /////////////////////////////////
    // GPS parameters
    /////////////////////////////////
    case GET_GPS_TRANSMIT_DATA:
        memcpy(&sensorSettings.gpsTransmitDataConfig, packet.data, packet.length);

        res = "[" + currentDateTime("%Y/%m/%d %H:%M:%S") + "] GET GPS TRANSMIT DATA: ";
        ss.clear();
        ss << res << sensorSettings.uint32ToBinaryPP(sensorSettings.gpsTransmitDataConfig[0]) << " || "
         << sensorSettings.uint32ToBinaryPP(sensorSettings.gpsTransmitDataConfig[1]);
        addSensorResponseToQueue(ss.str());
        hasNewSettings = true;
        break;


    default:
    {
        ss.clear();

        res = "[" + currentDateTime("%Y/%m/%d %H:%M:%S") + "] Got:\n";
        ss << res;
        ss << std::hex;
        for (int i = 0; i < packet.length; ++i)
        {
            ss << hex << uppercase << setfill('0') << setw(2) << (int)packet.data[i] << " ";

            if ((i + 1) % 4 == 0)
            {

                memcpy(i2c.c, packet.data + i - 3, 4);
                memcpy(f2c.c, packet.data + i - 3, 4);
                ss << dec << " (" << i2c.int_val << " | " << f2c.float_val << ")";

                if (packet.length - i > 1)
                {
                    ss << "\n";
                }
            }
        }
        addSensorResponseToQueue(ss.str());
        break;
    }

    }
    return true;
}

void IG1::addSensorResponseToQueue(string s)
{
    mLockSensorResponseQueue.lock();
    if (sensorResponseQueue.size() < SENSOR_RESPONSE_QUEUE_SIZE) {
        sensorResponseQueue.push(s);
    }
    else
    {
        sensorResponseQueue.pop();
        sensorResponseQueue.push(s);
    }
    mLockSensorResponseQueue.unlock();
}

void IG1::addCommandQueue(IG1Command cmd)
{
    mLockCommandQueue.lock();
    commandQueue.push(cmd);
    mLockCommandQueue.unlock();
}

void IG1::clearCommandQueue()
{
    mLockCommandQueue.lock();
    while (!commandQueue.empty())
        commandQueue.pop();
    mLockCommandQueue.unlock();
}

void IG1::gpioInit()
{
    if (ctrlGpio < 0)
        return;

    gpioExport(ctrlGpio);
    this_thread::sleep_for(chrono::milliseconds(100));

    gpioSetDirection(ctrlGpio, 1);
    this_thread::sleep_for(chrono::milliseconds(100));

    gpioSetValue(ctrlGpio, 1); //TX
    this_thread::sleep_for(chrono::milliseconds(100));
}

void IG1::gpioDeinit()
{
    if (ctrlGpio < 0)
        return;    
    gpioUnexport(ctrlGpio);
}

int IG1::gpioExport(unsigned int gpio)
{
#ifdef __linux__
    int fileDescriptor, length;
    char commandBuffer[MAX_BUF];

    log.d(TAG, "GPIO: %d\n", gpio);
    
    fileDescriptor = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
    if (fileDescriptor < 0) 
    {
        char errorBuffer[128] ;
        snprintf(errorBuffer,sizeof(errorBuffer), "gpioExport unable to open gpio%d",gpio) ;
        perror(errorBuffer);
        return fileDescriptor;
    }

    length = snprintf(commandBuffer, sizeof(commandBuffer), "%d", gpio);
    if (write(fileDescriptor, commandBuffer, length) != length) 
    {
        perror("gpioExport");
        return fileDescriptor ;

    }
    close(fileDescriptor);
    return 0;
#else
    return 0;
#endif
}

int IG1::gpioUnexport(unsigned int gpio)
{
#ifdef __linux__
    int fileDescriptor, length;
    char commandBuffer[MAX_BUF];

    fileDescriptor = open(SYSFS_GPIO_DIR "/unexport", O_WRONLY);
    if (fileDescriptor < 0) 
    {
        char errorBuffer[128] ;
        snprintf(errorBuffer,sizeof(errorBuffer), "gpioUnexport unable to open gpio%d",gpio) ;
        perror(errorBuffer);
        return fileDescriptor;
    }

    length = snprintf(commandBuffer, sizeof(commandBuffer), "%d", gpio);
    if (write(fileDescriptor, commandBuffer, length) != length)
     {
        perror("gpioUnexport") ;
        return fileDescriptor ;
    }
    close(fileDescriptor);
    return 0;
#else
    return 0;
#endif
}

int IG1::gpioSetDirection(unsigned int gpio, unsigned int out_flag)
{
#ifdef __linux__
    int fileDescriptor;
    char commandBuffer[MAX_BUF];

    snprintf(commandBuffer, sizeof(commandBuffer), SYSFS_GPIO_DIR  "/gpio%d/direction", gpio);

    fileDescriptor = open(commandBuffer, O_WRONLY);
    if (fileDescriptor < 0) 
    {
        char errorBuffer[128] ;
        snprintf(errorBuffer,sizeof(errorBuffer), "gpioSetDirection unable to open gpio%d",gpio) ;
        perror(errorBuffer);
        return fileDescriptor;
    }

    if (out_flag) 
    {
        if (write(fileDescriptor, "out", 4) != 4) 
        {
            perror("gpioSetDirection") ;
            return fileDescriptor ;
        }
    }
    else 
    {
        if (write(fileDescriptor, "in", 3) != 3) 
        {
            perror("gpioSetDirection") ;
            return fileDescriptor ;
        }
    }
    close(fileDescriptor);

    return 0;
#else
    return 0;
#endif
}

int IG1::gpioSetValue(unsigned int gpio, unsigned int value)
{
#ifdef __linux__
    int fileDescriptor;
    char commandBuffer[MAX_BUF];

    snprintf(commandBuffer, sizeof(commandBuffer), SYSFS_GPIO_DIR "/gpio%d/value", gpio);

    fileDescriptor = open(commandBuffer, O_WRONLY);
    if (fileDescriptor < 0) {
        char errorBuffer[128] ;
        snprintf(errorBuffer,sizeof(errorBuffer), "gpioSetValue unable to open gpio%d",gpio) ;
        perror(errorBuffer);
        return fileDescriptor;
    }

    if (value) {
        if (write(fileDescriptor, "1", 2) != 2) {
            perror("gpioSetValue") ;
            return fileDescriptor ;
        }
    }
    else {
        if (write(fileDescriptor, "0", 2) != 2) {
            perror("gpioSetValue") ;
            return fileDescriptor ;
        }
    }

    close(fileDescriptor);
    return 0;
#else
    return 0;
#endif


}

int IG1::gpioGetValue(unsigned int gpio, unsigned int *value)
{
#ifdef __linux__
    int fileDescriptor;
    char commandBuffer[MAX_BUF];
    char ch;

    snprintf(commandBuffer, sizeof(commandBuffer), SYSFS_GPIO_DIR "/gpio%d/value", gpio);

    fileDescriptor = open(commandBuffer, O_RDONLY);
    if (fileDescriptor < 0) {
        char errorBuffer[128] ;
        snprintf(errorBuffer,sizeof(errorBuffer), "gpioGetValue unable to open gpio%d",gpio) ;
        perror(errorBuffer);
        return fileDescriptor;
    }

    if (read(fileDescriptor, &ch, 1) != 1) {
        perror("gpioGetValue") ;
        return fileDescriptor ;
     }

    if (ch != '0') {
        *value = 1;
    } else {
        *value = 0;
    }

    close(fileDescriptor);
    return 0;
#else
    return 0;
#endif
}
