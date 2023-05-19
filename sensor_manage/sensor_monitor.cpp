#include "sensor_monitor.hpp"

int Sensor_monitor::Sensor_monitor_thread()
{

    udev *udev = udev_new();
    if (!udev) {
        printf("Failed to create udev.\n");
        return 1;
    }

    // // USB设备监听器
    // struct udev_monitor *usb_mon = udev_monitor_new_from_netlink(udev, "udev");
    // if (!usb_mon) {
    //     printf("Failed to create udev monitor for USB devices.\n");
    //     udev_unref(udev);
    //     return 1;
    // }
    // udev_monitor_filter_add_match_subsystem_devtype(usb_mon, "usb", "usb_device");
    // udev_monitor_enable_receiving(usb_mon);


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
            // USB 设备的读缓冲区有数据
            // if (FD_ISSET(udev_monitor_get_fd(usb_mon), &fds)) 
            // {
            //     udev_device *dev = udev_monitor_receive_device(usb_mon);
            //     if (dev) {
            //         const char *action = udev_device_get_action(dev);
            //         if (action) {
            //             printf("USB device %s: %s\n", action, udev_device_get_devnode(dev));
            //             // udev_list_entry *attrs = udev_device_get_properties_list_entry(dev);
            //             // udev_list_entry *attr;
            //             // udev_list_entry_foreach(attr, attrs) {
            //             //     printf("%s=%s\n", udev_list_entry_get_name(attr), udev_list_entry_get_value(attr));
            //             // }
            //         }
            //         udev_device_unref(dev);
            //     }
            //     cout<<"!!!!!!--------------------------------------------------------!!!!!!!!!!!!!!!!!"<<endl;
            // }
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