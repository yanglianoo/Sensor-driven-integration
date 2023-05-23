#pragma once 

#include <map>
#include <iostream>
#include <libudev.h>
#include <sys/select.h>
using namespace std;

namespace SMW
{

class Sensor_Node
{
public:
    string Action;          //表示插拔状态, add 和 remove 
    string Device_Name;     // 分配的设备描述符
    string ID_Model;        // 设备的型号  
    string ID_Vendor;       // 设备的厂商
    //string Driver_Path;   // 驱动程序路径,特指头文件路径
};


class Imu_Data_Node : public Sensor_Node
{
public:
    string   Imu_Name;      // imu 的型号
    int BaudRate;           // imu 的波特率
    int Hertz;              // imu 数据发布频率，可配置选项
public:
    void show()
    {   cout << "------------------------------------------------------------" << endl;
        cout<<"Action:"<< Action <<"  " << "Device_Name:" << Device_Name << "  " << "ID_Model:" << ID_Model << "  " << "ID_Vendor:" << ID_Vendor <<endl;
        cout<< "Imu_Name:" << Imu_Name << "  " << "BaudRate:" << BaudRate << "  " << "Hertz:" << Hertz <<endl;
        cout << "------------------------------------------------------------" << endl;
    }
};


enum Camera_Type
{
        RGB = 0,     //  普通RGB相机
        DEPTH,       //  深度相机
        INFRARED     //  红外相机
    };
class Cameara_Data_Node : public Sensor_Node
{
public:
    Camera_Type type;     // 相机类型
    string Camera_Name;   // 相机型号 
    int width;            
    int height;
};

class Sensor_server
{
       
    typedef std::map<string, Imu_Data_Node> Imu_Section;
    typedef std::map<string, Cameara_Data_Node> Camera_Section;

private:
    int imu_count;
    int camera_count;
    int laser_count;
    Imu_Section imu_sections;
    Camera_Section camera_sections;
    
public:
    int Sensor_monitor_thread();
    void Camera_Udev_Get(udev_device *dev);
    Sensor_server();
    ~Sensor_server();

};

}

