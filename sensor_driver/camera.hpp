#pragma once
#include "../camera/realsense/librealsense2/rs.hpp"
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

class Camera {
public:
    virtual void initialize(const int device, int width=640, int height=480, int fps=30, bool enable_depth=false) = 0;
    virtual void getData() = 0;
    
    void imgBinarize(const cv::Mat &origin, cv::Mat &target); //二值化
    void imgGray(const cv::Mat &origin, cv::Mat &target); //灰度化
    void imgBlur(const cv::Mat &origin, cv::Mat &target); //高斯模糊
    void imgCanny(const cv::Mat &origin, cv::Mat &target); //Canny边缘检测
    void imgDil(const cv::Mat &origin, cv::Mat &target); //膨胀
    void imgErode(const cv::Mat &origin, cv::Mat &target); //腐蚀

    void showImg(const std::string &name, const cv::Mat &img); //显示图片

    virtual ~Camera() {}
    
    int width_;
    int height_;
    int fps_;
    cv::Mat colorMat, depthMat, imgBinarizeMat, imgGrayMat, imgBlurMat, imgCannyMat, imgDilMat, imgErodeMat;
};

class RealSenseD435 : public Camera {
public:
    void initialize(const int device,int width=640, int height=480, int fps=30, bool enable_depth=false) override;
    void getData() override;

private:
    bool enable_depth_ = false;
    rs2::pipeline pipe;
    rs2::frameset frames;
    rs2::context ctx;
    rs2::config cfg;
};

// class USBCamera : public Camera {
// public:
//     void initialize(const int device,const int width=640, const int height=480, 
//                             const int fps=30, bool enable_depth=false);
//     void getData();

// private:
//     cv::VideoCapture cap;
// };
