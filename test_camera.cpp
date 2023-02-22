#include "camera.h"
#include <iostream>
int main()
{   
    USBCamera cam =  USBCamera();
    cam.initialize(6);
    // std::cout << cam->device_id << std::endl;
    // std::cout << " " << std::endl;
    // return 0;
    std::cout << "cameara id" << cam.device_id<<std::endl;
}

// #include <librealsense2/rs.hpp>
// #include <iostream>

// int main(){
//     rs2::pipeline p;
//     p.start();
//     rs2::frameset frames = p.wait_for_frames();
//     rs2::depth_frame depth = frames.get_depth_frame();
//     float width = depth.get_width();
//     float height = depth.get_height();
//     float dist_to_center = depth.get_distance(width / 2, height / 2);
//     std::cout << "The camera is facing an object " << dist_to_center << " meters away \r";



//     std::cout << "okk" << std::endl; 
//     return 0;
// }