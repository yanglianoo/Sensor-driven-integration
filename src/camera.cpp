#include "camera.h"
#include <iostream>

using namespace std;
using namespace cv;


void RealSenseD435::GetData()
{
    VideoCapture cap(device_id);

    if (!cap.isOpened()) {
        cout << "无法打开摄像头" << endl;
        return;
    }

    namedWindow("frame", WINDOW_NORMAL);

    // 读取摄像头帧并保存到 Mat 对象中
    while (true) {
        cap.read(data);

        if (data.empty()) {
            cout << "无法获取摄像头帧" << endl;
            break;
        }

        // 在这里可以对每一帧数据进行处理

        imshow("frame", data);

        if (waitKey(1) == 'q') {
            break;
        }
    }

    cap.release();
    destroyAllWindows();

}

void RealSenseD435::initialize(const int device)
{
    device_id = device;
    // is_depth = is_depth;

}

void USBCamera::GetData()
{
    cout << " 1" << endl;
}

void USBCamera::initialize(const int device)
{
    cout << " 1" << endl;

}
