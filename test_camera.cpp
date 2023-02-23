#include "camera.h"
#include <iostream>
using namespace cv;
int main()
{   
    Camera *cam =new RealSenseD435();
    cam->initialize(0, 640, 480, 30, true);
    int n = 20;
    while (n--)
    {
        cam->getData();
        std::cout << cam->color.size() << std::endl;
        std::cout << cam->depth.size() << std::endl;
        // namedWindow("Display Image", WINDOW_AUTOSIZE );
        // imshow("Display Image", cam->color);
        // waitKey(1);
        // imshow("Display depth", cam->depth*15);
        // waitKey(1);
    }
    return 0;

}