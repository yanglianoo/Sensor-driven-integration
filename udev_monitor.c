/**
 * @author:timer
 * @brief: 串口 usb 网络设备 插拔检测
 * @date: 2023.4.20
*/

//设备描述结构体定义
typedef struct DeviceInfo {
    char dev_node[64];    // 设备节点路径
    char dev_name[64];    // 设备名称
    char dev_type[64];    // 设备类型
    char driver[64];      // 设备使用的驱动程序
    char vendor_id[16];   // 设备制造商ID
    char product_id[16];  // 设备产品ID
    char serial[64];      // 设备序列号
    // 其他设备属性可以继续添加
} DeviceInfo;


#include <stdio.h>
#include <libudev.h>

int main()
{
    struct udev *udev = udev_new();
    if (!udev) {
        printf("Failed to create udev.\n");
        return 1;
    }

    // USB device monitoring
    struct udev_monitor *usb_mon = udev_monitor_new_from_netlink(udev, "udev");
    if (!usb_mon) {
        printf("Failed to create udev monitor for USB devices.\n");
        udev_unref(udev);
        return 1;
    }
    udev_monitor_filter_add_match_subsystem_devtype(usb_mon, "usb", "usb_device");
    udev_monitor_enable_receiving(usb_mon);

    // Serial port monitoring
    struct udev_monitor *tty_mon = udev_monitor_new_from_netlink(udev, "udev");
    if (!tty_mon) {
        printf("Failed to create udev monitor for serial ports.\n");
        udev_unref(udev);
        return 1;
    }
    udev_monitor_filter_add_match_subsystem_devtype(tty_mon, "tty", NULL);
    udev_monitor_enable_receiving(tty_mon);

    // Network device monitoring
    struct udev_monitor *net_mon = udev_monitor_new_from_netlink(udev, "udev");
    if (!net_mon) {
        printf("Failed to create udev monitor for network devices.\n");
        udev_unref(udev);
        return 1;
    }
    udev_monitor_filter_add_match_subsystem_devtype(net_mon, "net", NULL);
    udev_monitor_enable_receiving(net_mon);

    printf("Listening for udev events...\n");

    while (1) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(udev_monitor_get_fd(usb_mon), &fds);
        FD_SET(udev_monitor_get_fd(tty_mon), &fds);
        FD_SET(udev_monitor_get_fd(net_mon), &fds);
        int max_fd = udev_monitor_get_fd(net_mon) + 1;

        int ret = select(max_fd, &fds, NULL, NULL, NULL);
        if (ret > 0) {
            if (FD_ISSET(udev_monitor_get_fd(usb_mon), &fds)) {
                struct udev_device *dev = udev_monitor_receive_device(usb_mon);
                if (dev) {
                    const char *action = udev_device_get_action(dev);
                    if (action) {
                        printf("USB device %s: %s\n", action, udev_device_get_devnode(dev));
                        struct udev_list_entry *attrs = udev_device_get_properties_list_entry(dev);
                        struct udev_list_entry *attr;
                        udev_list_entry_foreach(attr, attrs) {
                            printf("%s=%s\n", udev_list_entry_get_name(attr), udev_list_entry_get_value(attr));
                        }
                    }
                    udev_device_unref(dev);
                }
            }
            if (FD_ISSET(udev_monitor_get_fd(tty_mon), &fds)) {
                struct udev_device *dev = udev_monitor_receive_device(tty_mon);
                if (dev) {
                    const char *action = udev_device_get_action(dev);
                    if (action) {
                        printf("Serial port %s: %s\n", action, udev_device_get_devnode(dev));
                        struct udev_list_entry *attrs = udev_device_get_properties_list_entry(dev);
                        struct udev_list_entry *attr;
                        udev_list_entry_foreach(attr, attrs)
                        {
                            printf("%s=%s\n", udev_list_entry_get_name(attr), udev_list_entry_get_value(attr));
                        }
                    }
                    udev_device_unref(dev);
                }
            }
            if (FD_ISSET(udev_monitor_get_fd(net_mon), &fds)) {
                struct udev_device *dev = udev_monitor_receive_device(net_mon);
                if (dev) {
                    const char *action = udev_device_get_action(dev);
                    if (action) 
                    {
                        printf("Network device %s: %s\n", action, udev_device_get_devnode(dev));
                        struct udev_list_entry *attrs = udev_device_get_properties_list_entry(dev);
                        struct udev_list_entry *attr;
                        udev_list_entry_foreach(attr, attrs)
                        {
                            printf("%s=%s\n", udev_list_entry_get_name(attr), udev_list_entry_get_value(attr));
                        }
                    }
                    udev_device_unref(dev);
                }
            }   
        }
    }
        udev_monitor_unref(usb_mon);
        udev_monitor_unref(tty_mon);
        udev_monitor_unref(net_mon);
        udev_unref(udev);       
        return 0;
}
