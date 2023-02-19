#include "imu.h"
#include <iostream>
#include <thread>
#include <stdarg.h>
using namespace std;


void logd(std::string tag, const char* str, ...)
{
    va_list a_list;
    va_start(a_list, str);
    if (!tag.empty())
        printf("[ %4s] [ %-8s]: ", "INFO", tag.c_str());
    vprintf(str, a_list);
    va_end(a_list);
} 

void MicroStrain::initialize(const std::string& port, uint32_t baudRate) 
{
    //建立串口通信的node
    connection = mscl::Connection::Serial(port,baudRate);
    mscl::InertialNode node(connection);
    bool success = node.ping();
    cout<<"通信是否成功："<< success <<endl;
    cout << "Node Information: " << endl;
    cout << "Model Name: " << node.modelName() << endl;
    cout << "Model Number: " << node.modelNumber() << endl;
    cout << "Serial: " << node.serialNumber() << endl;
    cout << "Firmware: " << node.firmwareVersion().str() << endl << endl;
    //配置IMU信息
    setCurrentConfig(node);
    //获取IMU配置信息
    getCurrentConfig(node);
    //开始数据采样
    startSampling(node);
    //挂起
    // setToIdle(node);
    //获取数据
        while(true)
            {
            //get all the data packets from the node, with a timeout of 500 milliseconds
            mscl::MipDataPackets packets = node.getDataPackets();

            for(mscl::MipDataPacket packet : packets)
            {
                //print out the data
                cout << "Packet Received: "<<endl;

                //get the data in the packet
                mscl::MipDataPoints data = packet.data();
                mscl::MipDataPoint dataPoint;

                //loop through all the data points in the packet
                for(unsigned int itr = 0; itr < data.size(); itr++)
                {
                    dataPoint = data[itr];

                    // cout << dataPoint.channelName() << ": ";
                    if(dataPoint.channelName() == "scaledAccelX")
                    {
                        linear_acceleration.x=dataPoint.as_float();
                        cout<<"scaledAccelX:"<<linear_acceleration.x<<endl;
                    }
                    if(dataPoint.channelName() == "scaledAccelY")
                    {
                        linear_acceleration.x=dataPoint.as_float();
                        cout<<"scaledAccelY:"<<linear_acceleration.y<<endl;
                    }
                    if(dataPoint.channelName() == "scaledAccelZ")
                    {
                        linear_acceleration.x=dataPoint.as_float();
                        cout<<"scaledAccelZ:"<<linear_acceleration.z<<endl;
                    }

                    if(dataPoint.channelName() == "scaledGyroX")
                    {
                        angular_velocity.x=dataPoint.as_float();
                        cout<<"scaledGyroX:"<<angular_velocity.x<<endl;
                    }
                    if(dataPoint.channelName() == "scaledGyroY")
                    {
                        angular_velocity.x=dataPoint.as_float();
                        cout<<"scaledGyroY:"<<angular_velocity.y<<endl;
                    }
                    if(dataPoint.channelName() == "scaledGyroX")
                    {
                        angular_velocity.x=dataPoint.as_float();
                        cout<<"scaledGyroZ:"<<angular_velocity.z<<endl;
                    }

                    //print out the channel data
                    //Note: The as_string() function is being used here for simplicity. 
                    //      Other methods (ie. as_float, as_uint16, as_Vector) are also available.
                    //      To determine the format that a dataPoint is stored in, use dataPoint.storedAs().
                    // cout << dataPoint.as_float() << " ";

                    //if the dataPoint is invalid
                    if(!dataPoint.valid())
                    {
                        cout << "[Invalid] ";
                    }
                }
                cout << endl;
            }
        }

}
static bool printThreadIsRunning=false;
void LpmsIG1::printTask()
{   
    if (sensor1->getStatus() != STATUS_CONNECTED)
    {
        printThreadIsRunning = false;
        logd(TAG, "Sensor is not connected. Sensor Status: %d\n", sensor1->getStatus());
        logd(TAG, "printTask terminated\n");
        return;
    }
    
    sensor1->commandGotoStreamingMode();
    
    printThreadIsRunning = true;
    while (sensor1->getStatus() == STATUS_CONNECTED && printThreadIsRunning)
    {
        IG1ImuDataI sd;
        if (sensor1->hasImuData())
        {
            sensor1->getImuData(sd);
            float freq = sensor1->getDataFrequency();
            // logd(TAG, "t(s): %.3f acc: %+2.2f %+2.2f %+2.2f gyr: %+3.2f %+3.2f %+3.2f euler: %+3.2f %+3.2f %+3.2f Hz:%3.3f \r\n", 
            //     sd.timestamp*0.002f, 
            //     sd.accCalibrated.data[0], sd.accCalibrated.data[1], sd.accCalibrated.data[2],
            //     sd.gyroIAlignmentCalibrated.data[0], sd.gyroIAlignmentCalibrated.data[1], sd.gyroIAlignmentCalibrated.data[2],
            //     sd.euler.data[0], sd.euler.data[1], sd.euler.data[2], 
            //     freq);
            angular_velocity.x =  sd.accCalibrated.data[0];
            angular_velocity.y =  sd.accCalibrated.data[1];
            angular_velocity.z =  sd.accCalibrated.data[2];
            linear_acceleration.x = sd.gyroIAlignmentCalibrated.data[0];
            linear_acceleration.y = sd.gyroIAlignmentCalibrated.data[1];
            linear_acceleration.z = sd.gyroIAlignmentCalibrated.data[2];
        }
        this_thread::sleep_for(chrono::milliseconds(2));
    }

    printThreadIsRunning = false;
    logd(TAG, "printTask terminated\n");
}

void LpmsIG1::initialize(const std::string& port, uint32_t baudRate=921600)
{
    TAG = "LpmsIG1->MAIN";
    printThreadIsRunning = false;
    sensor1 = IG1Factory();
    sensor1->setVerbose(VERBOSE_INFO);
    sensor1->setAutoReconnectStatus(true);
    // Connects to sensor
    if (!sensor1->connect(port, baudRate))
    {
        sensor1->release();
        logd(TAG, "Error connecting to sensor\n");
        logd(TAG, "bye\n");
    }
    do
    {
        logd(TAG, "Waiting for sensor to connect\r\n");
        this_thread::sleep_for(chrono::milliseconds(1000));
    } while (
        !(sensor1->getStatus() == STATUS_CONNECTED) && 
        !(sensor1->getStatus() == STATUS_CONNECTION_ERROR)
    );
    if (sensor1->getStatus() != STATUS_CONNECTED)
    {
        logd(TAG, "Sensor connection error status: %d\n", sensor1->getStatus());
        sensor1->release();
        logd(TAG, "bye\n");
    }
    logd(TAG, "Sensor connected\n");
    printTask();
}