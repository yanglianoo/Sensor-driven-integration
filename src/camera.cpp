#include "camera.h"
#include <iostream>

using namespace std;
using namespace cv;

bool Camera::_getColor(){
    cap.read(color);

    if (color.empty()) {
        cout << "Can't get the camera content!" << endl;
        return false;
    }
    return true;
}


void RealSenseD435::getData()
{

    if (!_getColor()){
        cout << "Get color info error!" << endl;
    }    

    if (is_depth && !_getDepth()){
        cout << "Get depth info error!" << endl;
    }
}

void RealSenseD435::initialize(const int device, bool depth_need)
{
    cap = VideoCapture(device);
    if (!cap.isOpened()) {
        cout << "无法打开摄像头" << endl;
        return;
    }

    is_depth = depth_need;
}

bool RealSenseD435::_getDepth(){
    // rs2::pipeline p;
    // p.start();
    // rs2::frameset frames = p.wait_for_frames();
    // rs2::depth_frame depth = frames.get_depth_frame();
    // float width = depth.get_width();
    // float height = depth.get_height();
    // float dist_to_center = depth.get_distance(width / 2, height / 2);
    // std::cout << "The camera is facing an object " << dist_to_center << " meters away \r";
    cout << "get depth info "  << endl;
    
    depth.zeros(3,3,CV_32F);
    return true;
}

void USBCamera::initialize(const int device, bool need_depth)
{
    cap = VideoCapture(device);
    if (!cap.isOpened()) {
        cout << "无法打开摄像头" << endl;
        return;
    }
}

void USBCamera::getData(){
    if (!_getColor()){
        cout << "Get color info error!" << endl;
    }   
}