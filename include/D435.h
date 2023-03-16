#pragma
#include <../realsense/librealsense2/rs.hpp>
#include <opencv2/opencv.hpp>

class Camera {
public:
    virtual void initialize(const int device, int width=640, int height=480, int fps=30, bool enable_depth=false) = 0;
    virtual void getData() = 0;

    int width_;
    int height_;
    int fps_;
    cv::Mat color;
    cv::Mat depth;
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
