#include <iostream>
#include "mscl/mscl.h"

int main()
{
    std::cout << mscl::MSCL_VERSION.str() << std::endl;
    //create the connection object with port and baud rate
    mscl::Connection connection = mscl::Connection::Serial("COM3", 921600);
    //create the BaseStation, passing in the connection
    mscl::BaseStation basestation(connection);
    return 0;
}





