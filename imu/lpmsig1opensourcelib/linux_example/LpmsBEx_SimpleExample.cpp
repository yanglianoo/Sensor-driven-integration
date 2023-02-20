#include <cstring>
#include <stdarg.h>
#include <thread>
#include <iostream>
#include <sstream>

#include "LpmsIG1I.h"
#include "SensorDataI.h"
#include "LpmsIG1Registers.h"

using namespace std;
string TAG("MAIN");
IG1I* sensor1;
// Start print data thread
std::thread *printThread;
static bool printThreadIsRunning = false;

void logd(std::string tag, const char* str, ...)
{
    va_list a_list;
    va_start(a_list, str);
    if (!tag.empty())
        printf("[ %4s] [ %-8s]: ", "INFO", tag.c_str());
    vprintf(str, a_list);
    va_end(a_list);
} 

void printTask()
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
            logd(TAG, "t(s): %.3f acc: %+2.2f %+2.2f %+2.2f gyr: %+3.2f %+3.2f %+3.2f euler: %+3.2f %+3.2f %+3.2f Hz:%3.3f \r\n", 
                sd.timestamp*0.002f, 
                sd.accCalibrated.data[0], sd.accCalibrated.data[1], sd.accCalibrated.data[2],
                sd.gyroIAlignmentCalibrated.data[0], sd.gyroIAlignmentCalibrated.data[1], sd.gyroIAlignmentCalibrated.data[2],
                sd.euler.data[0], sd.euler.data[1], sd.euler.data[2], 
                freq);
        }
        this_thread::sleep_for(chrono::milliseconds(2));
    }

    printThreadIsRunning = false;
    logd(TAG, "printTask terminated\n");
}

void printMenu()
{
    cout <<  endl;
    cout << "===================" << endl;
    cout << "Main Menu" << endl;
    cout << "===================" << endl;
    cout << "[c] Connect sensor" << endl;
    cout << "[d] Disconnect sensor" << endl;
    cout << "[0] Goto command mode" << endl;
    cout << "[1] Goto streaming mode" << endl;
    cout << "[h] Reset sensor heading" << endl;
    cout << "[i] Print sensor info" << endl;
    cout << "[s] Print sensor settings" << endl;
    cout << "[p] Print sensor data" << endl;
    cout << "[a] Toggle enable/disable auto reconnect" << endl;
    cout << "[q] quit" << endl;
    cout << endl;
}

int main(int argc, char** argv)
{
    string comportNo = "/dev/ttyUSB0";
    int baudrate = 115200;
    
    if (argc == 2)
    {
        comportNo = string(argv[1]);
    }
    else if (argc == 3)
    {
        comportNo = string(argv[1]);
        baudrate = atoi(argv[2]);
    }
	
	logd(TAG, "Connecting to %s @%d\n", comportNo.c_str(), baudrate);

    // Create LpmsIG1 object with corresponding comport and baudrate
    sensor1 = IG1Factory();
    sensor1->setVerbose(VERBOSE_INFO);
    sensor1->setAutoReconnectStatus(true);

    // Connects to sensor
    if (!sensor1->connect(comportNo, baudrate))
    {
        sensor1->release();
        logd(TAG, "Error connecting to sensor\n");
        logd(TAG, "bye\n");
        return 1;
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
        return 1;
    }


    logd(TAG, "Sensor connected\n");
    printMenu();

    bool quit = false;
    while (!quit)
    {
        char cmd = '\0';
        string input;
        getline(cin, input);
        
        if (!input.empty())
            cmd = input.at(0);
            
        switch (cmd)
        {
        case 'c':
            logd(TAG, "Connect sensor\n");
            if (!sensor1->connect(comportNo, baudrate))
                logd(TAG, "Error connecting to sensor\n");
            break;

        case 'd':
            logd(TAG, "Disconnect sensor\n");
            sensor1->disconnect();
            break;

        case '0':
            logd(TAG, "Goto command mode\n");
            sensor1->commandGotoCommandMode();
            break;

        case '1':
            logd(TAG, "Goto streaming mode\n");
            sensor1->commandGotoStreamingMode();
            break;

        case 'h':
            logd(TAG, "Reset heading\n");
            // reset sensor heading
            sensor1->commandSetOffsetMode(LPMS_OFFSET_MODE_HEADING);
            break;

        case 'i': {
            logd(TAG, "Print sensor info\n");
            // Print sensor info
            IG1InfoI info;
            sensor1->getInfo(info);
            cout << info.toString() << endl;
            break;
        }

        case 's': {
            logd(TAG, "Print sensor settings\n");
            // Print sensor settings
            IG1SettingsI settings;
            sensor1->getSettings(settings);
            cout << settings.toString() << endl;
            break;
        }

        case 'p': {     
            logd(TAG, "Print sensor data\n");       
            // Print sensor data
            if (!printThreadIsRunning)
                printThread = new std::thread(printTask);
            break;
        }

        case 'a': {
            logd(TAG, "Toggle auto reconnect\n");
            bool b = sensor1->getAutoReconnectStatus();
            sensor1->setAutoReconnectStatus(!b);
            logd(TAG, "Auto reconnect %s\n", !b? "enabled":"disabled");
            break;
        }

        case 'q':
            logd(TAG, "quit\n");
            // disconnect sensor
            sensor1->disconnect();
            quit = true;
            break;

        default:
            printThreadIsRunning = false;
	        if (printThread && printThread->joinable()) {
                printThread->join();
                printThread = NULL;
            }
            printMenu();
          break;
        }
        this_thread::sleep_for(chrono::milliseconds(100));
    }


    if (printThread && printThread->joinable())
        printThread->join();

    // release sensor resources
    sensor1->release();
    logd(TAG, "Bye\n");

    return 0;
}
