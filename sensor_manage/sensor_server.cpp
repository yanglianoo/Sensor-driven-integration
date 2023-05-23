#include "sensor_server.hpp"
#include <utility/ini_file.hpp>
#include <utility/system.hpp>
#include <utility/singleton.hpp>
#include <utility/logger.hpp>

using namespace SMW;
Sensor_server::Sensor_server()
{
    // init system
    System * sys = Singleton<System>::instance();
    sys->init();

    // init logger
    Logger::instance()->open(sys->get_root_path() + "/log/sensor.log");
    
    // init inifile
    IniFile * ini = Singleton<IniFile>::instance();
    ini->load(sys->get_root_path() + "/sensor_config.ini");


    imu_count = (*ini)["IMU_CONFIG"]["imu_count"];


    // 解析 IMU 配置文件
    if(imu_count > 1)
    {
        for (int i = 1; i <= imu_count; i++)
        {
            string imu_num = "IMU" + std::to_string(i);

            string imu_name = (*ini)[imu_num]["imu_name"];
            
            Imu_Data_Node imu_Data_Node ;

            imu_Data_Node.Imu_Name = imu_name;
            imu_Data_Node.Device_Name = (string)(*ini)[imu_num]["serial_port"];
            imu_Data_Node.BaudRate = (*ini)[imu_num]["baudRate"];
            imu_Data_Node.show();
            imu_sections[imu_name] = imu_Data_Node;
        }
    }

    log_info("配置文件读取完毕！");

    
}

void Sensor_server::Camera_Udev_Get(udev_device *dev)
{
    Cameara_Data_Node  camera_data_node;

    camera_data_node.Action = udev_device_get_action(dev);      //获取设备拔插动作
    camera_data_node.Device_Name = udev_device_get_devnode(dev);//获取设备描述符
    camera_data_node.ID_Model = udev_device_get_property_value(dev, "ID_MODEL");
    camera_data_node.ID_Vendor = udev_device_get_property_value(dev, "ID_VENDOR");
    camera_data_node.Camera_Name = udev_device_get_property_value(dev, "ID_V4L_PRODUCT");

    if(camera_data_node.Action == "add")
    {
        
        string video_cap = udev_device_get_property_value(dev, "ID_V4L_CAPABILITIES");

        size_t found = camera_data_node.Camera_Name.find("Depth");
        if (found != std::string::npos) {
            camera_data_node.type = DEPTH;
        } 

        found = camera_data_node.Camera_Name.find("USB");

        if (found != std::string::npos) {
            camera_data_node.type = RGB;
        } 

        //当该视频设备符能够捕获视频时才输出
        found = video_cap.find("capture");
        if (found != std::string::npos) 
        {
            camera_sections[camera_data_node.Device_Name] = camera_data_node;
            cout<<"!----------------------"<< "检测到设备插入" << "------------------------!"<<endl;
            log_info("检测到设备插入:%s",string(camera_data_node.Camera_Name));
            cout<<"!----------------------"<< camera_data_node.Camera_Name << "------------------------!"<<endl;
            cout<<"Action:"<<camera_data_node.Action<<endl;
            cout<<"Device_Name:"<<camera_data_node.Device_Name <<endl;
            cout<<"Camera_Name:"<<camera_data_node.Camera_Name <<endl;
            cout<<"ID_Model:"<<camera_data_node.ID_Model <<endl;
            cout<<"ID_Vendor:"<<camera_data_node.ID_Vendor <<endl;
            switch (camera_data_node.type)
            {
            case RGB:
                cout<<"type:"<<"RGB" <<endl;
                break;
            case DEPTH:
                cout<<"type:"<<"Depth" <<endl;
                break;
            default:
                break;
            }
        } 
    }
    else if(camera_data_node.Action == "remove")
    {
        auto it =camera_sections.find(camera_data_node.Device_Name);
        if (it != camera_sections.end()) 
        {
            camera_sections.erase(it); // 删除节点
            cout<<"!----------------------"<< "检测到设备拔出" << "------------------------!"<<endl;
            log_info("检测到设备拔出:%s",string(camera_data_node.Camera_Name));
            cout<<"Remove:" << camera_data_node.ID_Model << "  form Sensor_Manage"<<endl;
        }
    }

}
int Sensor_server::Sensor_monitor_thread()
{


    udev *udev = udev_new();
    if (!udev) {
        cout<<"Failed to create udev.\n"<<endl;
        return 1;
    }

    // 串口设备监听器
    struct udev_monitor *tty_mon = udev_monitor_new_from_netlink(udev, "udev");
    if (!tty_mon) {
        cout<<"Failed to create udev monitor for serial ports.\n"<<endl;
        udev_unref(udev);
        return 1;
    }
    udev_monitor_filter_add_match_subsystem_devtype(tty_mon, "tty", NULL);
    udev_monitor_enable_receiving(tty_mon);

    // 视频设备监听器
    struct udev_monitor *video_mon = udev_monitor_new_from_netlink(udev, "udev");
    if (!video_mon) {
        cout<<"Failed to create udev monitor for video devices.\n"<<endl;
        udev_unref(udev);
        return 1;
    }
    udev_monitor_filter_add_match_subsystem_devtype(video_mon, "video4linux", NULL);
    udev_monitor_enable_receiving(video_mon);


    // 网络设备监听器
    struct udev_monitor *net_mon = udev_monitor_new_from_netlink(udev, "udev");
    if (!net_mon) {
        cout<<"Failed to create udev monitor for network devices.\n"<<endl;
        udev_unref(udev);
        return 1;
    }
    udev_monitor_filter_add_match_subsystem_devtype(net_mon, "net", NULL);
    udev_monitor_enable_receiving(net_mon);

    std::cout<<"Wait for the sensor to be plugged in .....\n"<<std::endl;


    while (1) {
        fd_set fds;
        FD_ZERO(&fds);
        //FD_SET(udev_monitor_get_fd(usb_mon), &fds);
        FD_SET(udev_monitor_get_fd(tty_mon), &fds);
        FD_SET(udev_monitor_get_fd(net_mon), &fds);
        FD_SET(udev_monitor_get_fd(video_mon), &fds);
        int max_fd = udev_monitor_get_fd(video_mon) + 1;

        //判断 读缓冲区是否有有数据，如果有数据进行数据解析
        int ret = select(max_fd, &fds, NULL, NULL, NULL);

        if (ret > 0) {
            // 串口设备的读缓冲区有数据
            if (FD_ISSET(udev_monitor_get_fd(tty_mon), &fds)) 
            {
                udev_device *dev = udev_monitor_receive_device(tty_mon);
                
                if (dev) {
                    const char *action = udev_device_get_action(dev);
                    if (action) {
                        cout<<"Serial port:"<< action<< udev_device_get_devnode(dev)<<endl;
                        // udev_list_entry *attrs = udev_device_get_properties_list_entry(dev);
                        // udev_list_entry *attr;
                        // udev_list_entry_foreach(attr, attrs)
                        // {
                        //     printf("%s=%s\n", udev_list_entry_get_name(attr), udev_list_entry_get_value(attr));
                        // }
                    }
                    udev_device_unref(dev);
                }
                cout<<"!!!!!!--------------------------------------------------------!!!!!!!!!!!!!!!!!"<<endl;
            }

            // 视频设备
            if (FD_ISSET(udev_monitor_get_fd(video_mon), &fds)) 
            {
                udev_device *dev = udev_monitor_receive_device(video_mon);
                if (dev) 
                {
                    const char *action = udev_device_get_action(dev);
                    if (action) 
                    {
                        Camera_Udev_Get(dev);
                    }
                    udev_device_unref(dev);
                }
            }

            //网络设备的读缓冲区有数据
            if (FD_ISSET(udev_monitor_get_fd(net_mon), &fds)) 
            {
                udev_device *dev = udev_monitor_receive_device(net_mon);
                if (dev) {
                    const char *action = udev_device_get_action(dev);
                    if (action) 
                    {
                        std::cout << "Network device " << action << ": " << udev_device_get_devnode(dev) << std::endl;
                        // udev_list_entry *attrs = udev_device_get_properties_list_entry(dev);
                        // udev_list_entry *attr;
                        // udev_list_entry_foreach(attr, attrs)
                        // {
                        //     printf("%s=%s\n", udev_list_entry_get_name(attr), udev_list_entry_get_value(attr));
                        // }
                    }
                    udev_device_unref(dev);
                }
                cout<<"!!!!!!--------------------------------------------------------!!!!!!!!!!!!!!!!!"<<endl;
            }   
        }
    }
        // udev_monitor_unref(usb_mon);
        udev_monitor_unref(tty_mon);
        udev_monitor_unref(video_mon);
        udev_monitor_unref(net_mon);

        udev_unref(udev);   
}

