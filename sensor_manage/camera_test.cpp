#include <opencv2/opencv.hpp>

using namespace cv;
int main(int argc, char const *argv[])
{
    VideoCapture cap("/dev/video2"); // 打开特定的视频设备文件
    if (!cap.isOpened())
    {
    // 摄像头打开失败，处理错误
    return -1;
    }

    namedWindow("Camera", WINDOW_NORMAL);
    Mat frame;
    while (true)
    {
        // 读取图像帧
        cap.read(frame);

        // 检查是否成功读取图像帧
        if (frame.empty()) {
            // 未能读取到有效的图像帧，退出循环
            break;
        }

        // 显示图像帧
        imshow("Camera", frame);

        // 等待用户按下ESC键退出循环
        if (waitKey(1) == 27) {
            break;
        }
    }

    cap.release();
    destroyAllWindows();

    return 0;
}


