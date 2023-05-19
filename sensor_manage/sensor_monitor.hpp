
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
#include <map>
using namespace std;

class Sensor_Node
{
    string ACTION;          //表示插拔状态, add 和 remove 
    string DEVICE_NAME;     // 分配的设备描述符
    string ID_MODEL;        // 设备的型号  
    string ID_VENDOR;       // 设备的厂商
    string DRIVER_PATH;     // 驱动程序路径,特指头文件路径
};


class Imu_Data_Node : public Sensor_Node
{
    string   Imu_Name;      // imu 的型号
    uint32_t BaudRate;      // imu 的波特率
    uint32_t Hertz;         // imu 数据发布频率，可配置选项
};


class Cameara_Data_Node : public Sensor_Node
{
    enum Camera_Type
    {
        RGB = 0,     //  普通RGB相机
        DEPTH,       //  深度相机
        INFRARED     //  红外相机
    };
    Camera_Type type;     // 相机类型
    string Camera_Name;   // 相机型号 
    int width;      
    int height;
};

class Sensor_Data
{
    typedef std::map<string, Imu_Data_Node> Imu_Section;
    typedef std::map<string, Cameara_Data_Node> Camera_Section;
public:
    Imu_Section imu_sections;
    Camera_Section camera_sections;
};



class Sensor_monitor
{
private:
    Sensor_Data sensor_data;
public:
    //实现设备热插拔检测，并更新设备管理信息
    int Sensor_monitor_thread();
};

