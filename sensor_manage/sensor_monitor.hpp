
/**
 * @author:timer
 * @brief: 串口 usb 网络设备 插拔检测
 * @brief: 使用 select IO 多路复用技术实现，单线程
 * @date: 2023.4.20
*/


#pragma once 

#include <stdio.h>
#include <libudev.h>
#include <sys/select.h>
#include <iostream>

using namespace std;

class Sensor_monitor
{

public:
    //实现设备热插拔检测，并更新设备管理信息
    int Sensor_monitor_thread();
};

