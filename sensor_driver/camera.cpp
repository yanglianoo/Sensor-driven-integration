#include "camera.hpp"
using namespace std;
using namespace cv;
using namespace SMW;
void Camera::imgBinarize(const cv::Mat &origin, cv::Mat &target){

    threshold(origin, target, 128, 255, THRESH_BINARY);

} //二值化

void Camera::imgGray(const cv::Mat &origin, cv::Mat &target){

    cvtColor(origin, target, COLOR_BGR2GRAY);

} //灰度化

void Camera::imgBlur(const cv::Mat &origin, cv::Mat &target){

    GaussianBlur(origin, target, Size(3, 3), 3, 0);


} //高斯模糊

void Camera::imgCanny(const cv::Mat &origin, cv::Mat &target){

    Canny(origin, target, 25, 75);


} //Canny边缘检测

void Camera::imgDil(const cv::Mat &origin, cv::Mat &target){

    Mat kernel = getStructuringElement(MORPH_RECT, Size(3, 3));
    dilate(origin, target, kernel);

} //膨胀

void Camera::imgErode(const cv::Mat &origin, cv::Mat &target){

    Mat kernel = getStructuringElement(MORPH_RECT, Size(3, 3));
    erode(origin, target, kernel);

} //腐蚀


void Camera::showImg(const std::string &name, const cv::Mat &img){

    imshow(name, img);

}

void RealSenseD435::initialize(const int device,int width, int height, int fps, bool enable_depth)
{
    auto list = ctx.query_devices();
    if (list.size() == 0)
    {
        throw std::runtime_error("No device detected. Is it plugged in?");
    }
    rs2::device dev = list[device];

    cout << "Using device " + to_string(device) + ": " <<  dev.get_info(RS2_CAMERA_INFO_NAME) << endl;
    cout << "Serial number: " <<  dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER) << endl;
    cout << "Firmware version: " <<  dev.get_info(RS2_CAMERA_INFO_FIRMWARE_VERSION) << endl;
    cout << "Physical port: " <<  dev.get_info(RS2_CAMERA_INFO_PHYSICAL_PORT) << endl;

    cfg.enable_stream(RS2_STREAM_COLOR, width, height, RS2_FORMAT_BGR8, fps);
    if (enable_depth)
    {
        cfg.enable_stream(RS2_STREAM_DEPTH, width, height, RS2_FORMAT_Z16, fps);
    }
    width_ = width;
    height_ = height;
    fps_ = fps;
    enable_depth_ = enable_depth;

    pipe.start(cfg);
}

void RealSenseD435::getData()
{
    frames = pipe.wait_for_frames();
    rs2::frame color_frame = frames.get_color_frame();
    colorMat = cv::Mat(cv::Size(width_, height_), CV_8UC3, (void*)color_frame.get_data(), cv::Mat::AUTO_STEP);

    if (enable_depth_){
        rs2::frame depth_frame = frames.get_depth_frame();
        depthMat = cv::Mat(cv::Size(width_, height_), CV_16UC1, (void*)depth_frame.get_data(), cv::Mat::AUTO_STEP);

    }
}
