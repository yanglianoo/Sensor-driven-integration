#pragma
#include <stdint.h>
#include <librealsense2/rs.hpp>
#include <string>
#include <opencv2/opencv.hpp>


class Camera
{
public:
    virtual void initialize(const int device, bool depth_need=false) = 0;
    virtual void getData() = 0;
    virtual bool _getColor();
    cv::VideoCapture cap;
    cv::Mat color;
    cv::Mat depth;
};

class RealSenseD435 : public Camera
{
public:
    void initialize(const int device, bool depth_need) override;
    void getData() override;
    bool _getDepth();
private:
    bool is_depth;
};

class USBCamera : public Camera
{
public:
    void initialize(const int device, bool depth_need) override;
    void getData() override;
};