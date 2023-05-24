#include <sensor_driver/imu.hpp>
#include <iostream>
#include <sensor_manage/sensor_server.hpp>
using namespace SMW;
// using namespace cv;
int main()
{   
    
    Sensor_server * sensor_server = new Sensor_server();
    sensor_server->Sensor_monitor_thread();
    // std::thread sensorThread([&]() {
    // sensor_server->Sensor_monitor_thread();
    // });
    // // 等待线程执行完毕
    // sensorThread.join();

    // std::thread CameraThread([&]() {
    //     while (true)
    //     {
    //         // auto it = sensor_server->camera_sections.find("Intel_R__RealSense_TM__Depth_Camera_435");
    //         // if (it != sensor_server->camera_sections.end()) 
    //         // {
    //         //         // Camera *cam =new RealSenseD435();
    //         //         // cam->initialize(0, 640, 480, 30, true);
    //         //         // cam->getData();
    //         //         // cam->showImg("Display Image", cam->colorMat);
    //         //         // cam->showImg("Display depth", cam->depthMat*15);
    //         // }
    //     }
        
    
    // CameraThread.join();
    // IMU* imu = new MicroStrain();
    // // MicroStrain imu = MicroStrain();
    // imu->initialize("/dev/ttyUSB0",115200);
    // while (true)
    // {
    //     imu->GetData();
    //     // std::cout<<"linear_acceleration.x:"<< imu->linear_acceleration.x<<std::endl;
    //     // std::cout<<"linear_acceleration.y:"<< imu->linear_acceleration.y<<std::endl;
    //     // std::cout<<"linear_acceleration.z:"<< imu->linear_acceleration.z<<std::endl;
    //     // std::cout<<"angular_velocity.x:"<< imu->angular_velocity.x<<std::endl;
    //     // std::cout<<"angular_velocity.y:"<< imu->angular_velocity.y<<std::endl;
    //     // std::cout<<"angular_velocity.z:"<< imu->angular_velocity.z<<std::endl;

    // }
    
    return 0;
}


