#include "camera.h"
#include <iostream>
#include <librealsense2/rs.hpp>


using namespace std;
// using namespace cv;


void Camera::GetData()
{
    // VideoCapture cap(device_id);

    // if (!cap.isOpened()) {
    //     cout << "无法打开摄像头" << endl;
    //     return;
    // }

    // // namedWindow("frame", WINDOW_NORMAL);

    // // 读取摄像头帧并保存到 Mat 对象中
    // while (true) {
    //     cap.read(data);
    //     // cout << data << endl;

    //     if (data.empty()) {
    //         cout << "无法获取摄像头帧" << endl;
    //         break;
    //     }

    //     // 在这里可以对每一帧数据进行处理

    //     // imshow("frame", data);

    //     if (waitKey(1) == 'q') {
    //         break;
    //     }

    //     // if (type == 1 && this->is_depth == true){
    //     //     this->GetDepth();
    //     // }
    // }

    // cap.release();
    // destroyAllWindows();
    

}

void RealSenseD435::initialize(const int device, bool is_depth)
{
    device_id = device;
    type = 1;
    this->is_depth = is_depth;
}

void RealSenseD435::GetDepth(){
    // rs2::pipeline p;
    // p.start();
    // rs2::frameset frames = p.wait_for_frames();
    // rs2::depth_frame depth = frames.get_depth_frame();
    // float width = depth.get_width();
    // float height = depth.get_height();
    // float dist_to_center = depth.get_distance(width / 2, height / 2);
    // std::cout << "The camera is facing an object " << dist_to_center << " meters away \r";



    std::cout << "okk" << std::endl; 
}

void USBCamera::initialize(const int device)
{
    device_id = device;
    type = 0;
}
