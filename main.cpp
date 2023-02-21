#include "imu.h"


int main()
{   
   
    IMU* imu = new MicroStrain();
    imu->initialize("/dev/ttyUSB0",115200);
    while (true)
    {
        imu->GetData();
    }
    
    return 0;
}
