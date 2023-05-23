#include <sensor_driver/camera.hpp>
#include <iostream>
using namespace SMW;

using namespace cv;
using namespace std;

int main()
{   
    Camera *cam =new RealSenseD435();
    cam->initialize(0, 640, 480, 30, true);
    while (true)
    {
        // 得到原始图像和深度图像
        cam->getData();
        // namedWindow("Display Image", WINDOW_AUTOSIZE );
        // 将图片转化为灰度图
        cam->imgGray(cam->colorMat, cam->imgGrayMat);
        // 高斯模糊
        cam->imgBlur(cam->colorMat, cam->imgBlurMat);

        // 边缘检测器
        cam->imgCanny(cam->colorMat, cam->imgCannyMat);

        // 膨胀
        cam->imgDil(cam->colorMat, cam->imgDilMat);

        // 腐蚀
        cam->imgErode(cam->colorMat, cam->imgErodeMat);

        // 二值化
        cam->imgBinarize(cam->imgGrayMat, cam->imgBinarizeMat);

        // 显示图片
        cam->showImg("Display Binarize", cam->imgBinarizeMat);
        cam->showImg("Display Dil", cam->imgDilMat);
        cam->showImg("Display Erode", cam->imgErodeMat);
        cam->showImg("Display Blur", cam->imgGrayMat);
        cam->showImg("Display Canny", cam->imgCannyMat);
        cam->showImg("Display Gray", cam->imgGrayMat);
        cam->showImg("Display Image", cam->colorMat);
        cam->showImg("Display depth", cam->depthMat*15);
        waitKey(1);
    }
    return 0;

}