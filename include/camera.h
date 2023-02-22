#pragma
#include <stdint.h>
#include <librealsense2/rs.hpp>
#include <string>
#include <opencv2/opencv.hpp>


class Camera
{
public:
    virtual void initialize(const int device){};
    virtual void GetData();
    int device_id; //设备id
    int type; //设备类型目前只有两种：0:usb摄像头 1:realsense
    cv::Mat data;
};

class RealSenseD435 : public Camera
{
public:
    void initialize(const int device, bool is_depth);
    void GetDepth();
private:
    bool is_depth;
    cv::Mat depth;
};

class USBCamera : public Camera
{
public:
    void initialize(const int device) override;
};