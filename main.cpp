#include "imu.h"


int main()
{   
   
    IMU* imu = new LpmsIG1();
    imu->initialize("/dev/ttyUSB0",921600);
    return 0;
}
