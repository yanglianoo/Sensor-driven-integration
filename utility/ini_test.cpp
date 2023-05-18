#include "ini_file.hpp"
#include <iostream>

int main(int argc, char const *argv[])
{
    IniFile * ini = new IniFile();
    ini->load("sensor_config.ini");
    ini->show();

    string m_ip;
    int m_port;
    int m_threads;
    int m_connects;
    int m_wait_time;
    //value 类不能隐式的转换为string类，需要显示转换一下
    m_ip = (string)(*ini)["server"]["ip"];
    m_port = (*ini)["server"]["port"];
    m_threads = (*ini)["server"]["threads"];
    m_connects = (*ini)["server"]["max_conn"];
    m_wait_time = (*ini)["server"]["wait_time"];

    std::cout<<"m_ip:"<<m_ip<<std::endl;
    std::cout<<"m_port:"<<m_port<<std::endl;
    std::cout<<"m_threads:"<<m_threads<<std::endl;
    std::cout<<"m_connects:"<<m_connects<<std::endl;
    std::cout<<"m_wait_time:"<<m_ip<<std::endl;
    return 0;
}
