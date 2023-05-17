#include "sensor_monitor.hpp"


int main(int argc, char const *argv[])
{
    Sensor_monitor *udev = new Sensor_monitor();

    udev->Sensor_monitor_thread();

    delete [] udev;
    return 0;
}
