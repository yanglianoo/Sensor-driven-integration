#include "mainwindow.h"
#include <QApplication>
#include <sensor_manage/sensor_server.hpp>
#include <utility/system.hpp>
#include <utility/singleton.hpp>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

//    Sensor_server * sensor_server = Singleton<Sensor_server>::instance();
//    sensor_server->Sensor_monitor_thread();
//    std::cout<<"hello"<<std::endl;

    w.show();
    return a.exec();
}
