#include "sensor_server.hpp"
#include <stdio.h>
#include <libudev.h>
#include <sys/select.h>
#include <utility/ini_file.hpp>
#include <utility/system.hpp>
#include <utility/singleton.hpp>
#include <utility/logger.hpp>
Sensor_server::Sensor_server()
{
    // init system
    System * sys = Singleton<System>::instance();
    sys->init();

    // init logger
    Logger::instance()->open(sys->get_root_path() + "/log/snesor.log");
    
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
            
            imu_sections[imu_name] = imu_Data_Node;
        }
    }

    
}


int Sensor_server::Sensor_monitor_thread()
{

    udev *udev = udev_new();
    if (!udev) {
        printf("Failed to create udev.\n");
        return 1;
    }

    // 串口设备监听器
    struct udev_monitor *tty_mon = udev_monitor_new_from_netlink(udev, "udev");
    if (!tty_mon) {
        printf("Failed to create udev monitor for serial ports.\n");
        udev_unref(udev);
        return 1;
    }
    udev_monitor_filter_add_match_subsystem_devtype(tty_mon, "tty", NULL);
    udev_monitor_enable_receiving(tty_mon);

    // 视频设备监听器
    struct udev_monitor *video_mon = udev_monitor_new_from_netlink(udev, "udev");
    if (!video_mon) {
        printf("Failed to create udev monitor for video devices.\n");
        udev_unref(udev);
        return 1;
    }
    udev_monitor_filter_add_match_subsystem_devtype(video_mon, "video4linux", NULL);
    udev_monitor_enable_receiving(video_mon);


    // 网络设备监听器
    struct udev_monitor *net_mon = udev_monitor_new_from_netlink(udev, "udev");
    if (!net_mon) {
        printf("Failed to create udev monitor for network devices.\n");
        udev_unref(udev);
        return 1;
    }
    udev_monitor_filter_add_match_subsystem_devtype(net_mon, "net", NULL);
    udev_monitor_enable_receiving(net_mon);

    printf("Wait for the sensor to be plugged in .....\n");


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
                        printf("Serial port %s: %s\n", action, udev_device_get_devnode(dev));
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
            if (FD_ISSET(udev_monitor_get_fd(video_mon), &fds)) {
                udev_device *dev = udev_monitor_receive_device(video_mon);
                if (dev) {
                    const char *action = udev_device_get_action(dev);
                    if (action) {
                        printf("Video device %s: %s\n", action, udev_device_get_devnode(dev));
                        udev_list_entry *attrs = udev_device_get_properties_list_entry(dev);
                        udev_list_entry *attr;
                        udev_list_entry_foreach(attr, attrs) {
                            printf("%s=%s\n", udev_list_entry_get_name(attr), udev_list_entry_get_value(attr));
                        }
                    }
                    udev_device_unref(dev);
                }
                cout << "!!!!!!--------------------------------------------------------!!!!!!!!!!!!!!!!!" << endl;
            }

            //网络设备的读缓冲区有数据
            if (FD_ISSET(udev_monitor_get_fd(net_mon), &fds)) 
            {
                udev_device *dev = udev_monitor_receive_device(net_mon);
                if (dev) {
                    const char *action = udev_device_get_action(dev);
                    if (action) 
                    {
                        printf("Network device %s: %s\n", action, udev_device_get_devnode(dev));
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

