
#include "SerialPort.h"
#include "LpUtil.h"
using namespace std;

Serial::Serial():
    portNo(""),
    connected(false)
{
}

Serial::~Serial()
{
    //Check if we are connected before trying to disconnect
    close();
}

bool Serial::open(std::string portno, int baudrate)
{
    if (isConnected())
        close();

    createUsbDeviceMap();

    map<string, string>::iterator iter = usbDeviceMap.find(portno);
    if (iter != usbDeviceMap.end())
    {
        portno = iter->second;
    }

    fd = ::open (portno.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0)
    {
        log.e(TAG, "Error opening %s: Err %d %s\n", portno.c_str(), errno, strerror (errno));
        return false;
    }

    if (set_interface_attribs (fd, baudrate, 0) == -1)
    {
        log.e(TAG, "Error configuring serial attributes");
        return false;
    } 

    //tcflush(fd,TCIOFLUSH);
    portNo = portno;
    this->connected = true;
    return true;
}


bool Serial::close()
{
    if (this->connected)
    {
        //We're no longer connected
        this->connected = false;

        //tcflush(fd,TCIOFLUSH);
        //Close the serial handler
        ::close(fd);
    }

    return true;
}

int Serial::readData(unsigned char *buffer, unsigned int nbChar)
{
    if (!isConnected())
        return 0;
   
    int bytes_avail;
    ioctl(fd, FIONREAD, &bytes_avail);

    if (bytes_avail > INCOMING_DATA_MAX_LENGTH) {
        log.d(TAG, "Warning: Buffer overflow: %d\n", bytes_avail);
        bytes_avail = INCOMING_DATA_MAX_LENGTH;
    } else if (bytes_avail > nbChar)
        bytes_avail = nbChar; 

    int n = ::read(fd, buffer, bytes_avail);//sizeof(rxBuffer));  // read up to 100 characters if ready to read
    
    return n;
}

void Serial::setMode(int mode)
{
    usbMode = mode;
}

int Serial::getMode(void)
{
    return usbMode;
}

bool Serial::writeData(unsigned char *buffer, unsigned int nbChar)
{
    if (!isConnected())
    {
        log.e(TAG, "Error: dongle not connected\n");
        return false;
    }
    int ret;
    ret = ::write(fd, buffer, nbChar);           // send 7 character greeting
    return true;
}

bool Serial::isConnected()
{
    //Simply return the connection status
    return this->connected;
}


int Serial::set_interface_attribs (int fd, int speed, int parity)
{
    /*
    struct termios2 tty;


    if (ioctl(fd, TCGETS2, &tty) < 0)
    {
        return -1;
    }
    tty.c_cflag &= ~CBAUD;
    tty.c_cflag |= BOTHER;
    tty.c_ispeed = speed;
    tty.c_ospeed = speed;

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    tty.c_iflag &= ~IGNBRK;         // disable break processing
    tty.c_lflag = 0;                // no signaling chars, no echo,
                                    // no canonical processing
    tty.c_oflag = 0;                // no remapping, no delays
    tty.c_cc[VMIN]  = 0;            // read doesn't block
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY |INLCR | IGNCR | ICRNL); //enable xon
    tty.c_iflag |=IXOFF;

    tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                    // enable reading

    tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
    tty.c_cflag |= parity;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (ioctl(fd, TCSETS2, &tty) < 0)
    {
        return -1;
    }
    */

     struct termios2 config;

    if (ioctl(fd, TCGETS2, &config) < 0)
    {
        return -1;
    }

    config.c_cflag &= ~CBAUD;
    config.c_cflag |= BOTHER;
    config.c_ispeed = speed;
    config.c_ospeed = speed;

    
    config.c_iflag &= ~(IGNBRK | IXANY | INLCR | IGNCR | ICRNL);
    config.c_iflag &= ~IXON;              // disable XON/XOFF flow control (output)
    config.c_iflag &= ~IXOFF;             // disable XON/XOFF flow control (input)
    config.c_cflag &= ~CRTSCTS;           // disable RTS flow control
    config.c_lflag = 0;
    config.c_oflag = 0;
    config.c_cflag &= ~CSIZE;
    config.c_cflag |= CS8;                // 8-bit chars
    config.c_cflag |= CLOCAL;             // ignore modem controls
    config.c_cflag |= CREAD;              // enable reading
    config.c_cflag &= ~(PARENB | PARODD); // disable parity
    config.c_cflag &= ~CSTOPB;            // one stop bit
    config.c_cc[VMIN] = 0;                // read doesn´t block
    config.c_cc[VTIME] = 5;               // 0.5 seconds read timeout

    config.c_cflag |= parity;

    if (ioctl(fd, TCSETS2, &config) < 0)
    {
        return -1;
    }

    /*
    struct termios2 config;

    if (ioctl(fd, TCGETS2, &config) < 0)
    {
        return -1;
    }
 	config.c_iflag &= ~(IGNBRK | IXANY | INLCR | IGNCR | ICRNL);
    config.c_iflag &= ~IXON;    		  // disable XON/XOFF flow control (output)
    config.c_iflag &= ~IXOFF;   		  // disable XON/XOFF flow control (input)
    config.c_cflag &= ~CRTSCTS; 		  // disable RTS flow control
    config.c_lflag = 0;
    config.c_oflag = 0;
    config.c_cflag &= ~CSIZE;
    config.c_cflag |= CS8;                // 8-bit chars
    config.c_cflag |= CLOCAL;             // ignore modem controls
    config.c_cflag |= CREAD;              // enable reading
    config.c_cflag &= ~(PARENB | PARODD); // disable parity
    config.c_cflag &= ~CSTOPB;            // one stop bit
    config.c_cc[VMIN] = 0;                // read doesn´t block
    config.c_cc[VTIME] = 5;               // 0.5 seconds read timeout


    if (ioctl(fd, TCSETS2, &config) < 0)
    {
        return -1;
    }
    */
    
    return 0;
}


void Serial::createUsbDeviceMap()
{
    struct udev *udev;
    struct udev_device *dev;
    struct udev_enumerate *enumerate;
    struct udev_list_entry *list, *node;
    const char *path;

    usbDeviceMap.clear();
    udev = udev_new();
    if (!udev) 
    {
        printf("can not create udev");
    }

    enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "tty");
    udev_enumerate_scan_devices(enumerate);

    list = udev_enumerate_get_list_entry(enumerate);
    udev_list_entry_foreach(node, list) 
    {
        path = udev_list_entry_get_name(node);
        dev = udev_device_new_from_syspath(udev, path);
        
        
        if (udev_device_get_property_value(dev, "ID_SERIAL_SHORT") &&
            udev_device_get_property_value(dev, "DEVNAME"))
        {
            string serialID(udev_device_get_property_value(dev, "ID_SERIAL_SHORT"));
            string devName(udev_device_get_property_value(dev, "DEVNAME"));
            
            string vendorId(udev_device_get_property_value(dev, "ID_VENDOR_ID"));
            string productId(udev_device_get_property_value(dev, "ID_MODEL_ID"));
           
	       // Check device usb is CP2102 
            if (vendorId=="10c4" && (productId=="ea60" || productId == "ea61" ))
                usbDeviceMap.insert(pair<string, string>(serialID, devName));
        }
        udev_device_unref(dev);
    }
}

