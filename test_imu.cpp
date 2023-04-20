#include "imu.h"
#include <iostream>

int main()
{   
   
    IMU* imu = new MicroStrain();
    // MicroStrain imu = MicroStrain();
    imu->initialize("/dev/ttyUSB0",115200);
    while (true)
    {
        imu->GetData();
        // std::cout<<"linear_acceleration.x:"<< imu->linear_acceleration.x<<std::endl;
        // std::cout<<"linear_acceleration.y:"<< imu->linear_acceleration.y<<std::endl;
        // std::cout<<"linear_acceleration.z:"<< imu->linear_acceleration.z<<std::endl;
        // std::cout<<"angular_velocity.x:"<< imu->angular_velocity.x<<std::endl;
        // std::cout<<"angular_velocity.y:"<< imu->angular_velocity.y<<std::endl;
        // std::cout<<"angular_velocity.z:"<< imu->angular_velocity.z<<std::endl;

    }
    
    return 0;
}


