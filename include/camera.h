#pragma
#include <stdint.h>
#include <librealsense2/rs.hpp>
#include <string>
#include <opencv2/opencv.hpp>


class Camera
{
public:
    virtual void initialize(const int device) = 0;
    virtual void GetData() = 0;
    int device_id;
    cv::Mat data;
};

class RealSenseD435 : public Camera
{
public:
    void GetData() override;
    void initialize(const int device) override;

private:
    bool is_depth;
    cv::Mat depth;
};

class USBCamera : public Camera
{
public:
    void GetData() override;
    void initialize(const int device) override;
};