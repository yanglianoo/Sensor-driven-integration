#ifndef SERIALCLASS_H_INCLUDED
#define SERIALCLASS_H_INCLUDED


#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include "SiUSBXp.h"
#elif __linux__ 
#include <stdio.h>      // standard input / output functions
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <asm/termbits.h>
#include <cstring>
#include <libudev.h>
#include <iterator> 
#include <map> 
#endif
#include "LpLog.h"


const static int INCOMING_DATA_MAX_LENGTH = 2048;

class Serial
{
    const std::string TAG = "SERIAL";
public:
    enum  {
        BAUDRATE_4800=4800,
        BAUDRATE_9600=9600,
        BAUDRATE_19200=19200,
        BAUDRATE_38400=38400,
        BAUDRATE_57600=57600,
        BAUDRATE_115200=115200,
        BAUDRATE_230400=230400,
        BAUDRATE_256000=256000,
        BAUDRATE_460800=460800,
        BAUDRATE_921600=921600
    };

    static const int MODE_VCP = 0;
    static const int MODE_USB_EXPRESS = 1;

private:
    //Connection status
    bool connected;
    
#ifdef _WIN32
    //Serial comm handler
    HANDLE hSerial;
    //Get various information about the connection
    COMSTAT status;
    //Keep track of last error
    DWORD errors;

    int portNo;

    // SILAB
	HANDLE	cyHandle;
    std::string idNumber;

#elif __linux__ 

    int fd;
    std::string portNo;

    std::map<std::string, std::string> usbDeviceMap;

#endif

    int usbMode;

public:

    Serial();

    ~Serial(); 
   
#ifdef _WIN32
    bool open(int portno, int baudrate = BAUDRATE_9600);
    bool open(std::string deviceId, int baudrate);
#elif __linux__ 
    bool open(std::string portno, int baudrate = BAUDRATE_115200);
#endif

    // Silab USBXpress or VCP mode, default VCP mode
    void setMode(int mode = MODE_VCP);

    int getMode(void);

    bool close();

    int readData(unsigned char *buffer, unsigned int nbChar);
#ifdef _WIN32
    bool writeData(const char *buffer, unsigned int nbChar);
#elif __linux__ 
    bool writeData(unsigned char *buffer, unsigned int nbChar);
#endif
    bool isConnected();

#ifdef _WIN32
    int getPortNo() { return portNo; }
#elif __linux__ 
    std::string getPortNo() { return portNo; }
#endif 

#ifdef __linux__ 
    int set_interface_attribs (int fd, int speed, int parity);
    void createUsbDeviceMap();
    
#endif
    LpLog& log = LpLog::getInstance();

};

#endif // SERIALCLASS_H_INCLUDED
