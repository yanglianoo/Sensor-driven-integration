#include "imu.h"


int main()
{   
   
    LpmsIG1* imu = new LpmsIG1();
    imu->initialize("/dev/ttyUSB0",921600);
    while (true)
    {
        imu->printTask();
    }
    
    return 0;
}
