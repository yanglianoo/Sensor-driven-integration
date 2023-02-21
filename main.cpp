// #include "imu.h"


// int main()
// {   
   
//     IMU* imu = new MicroStrain();
//     imu->initialize("/dev/ttyUSB0",115200);
//     while (true)
//     {
//         imu->GetData();
//     }
    
//     return 0;
// }

// #include "imu.h"
#include "camera.h"

int main()
{   
    Camera* cam = new RealSenseD435();
    cam->initialize(6);
    std::cout << cam->device_id << std::endl;
    // IMU* imu = new MicroStrain();
    // imu->initialize("/dev/ttyUSB0",115200);
    // while (true)
    // {
    // }
    std::cout << " " << std::endl;
    return 0;
}