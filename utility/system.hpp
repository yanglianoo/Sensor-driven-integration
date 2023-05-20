/**
 * @author:timer
 * @brief: 配置进程核心转储，获取当前执行文件的路径，创建log目录
 * @date: 2023.5.20
*/
#pragma once

#include <string>
using std::string;



class System
{
public:
    System();
    ~System();
    
    void init();
    //获取程序的执行路径
    string get_root_path();

private:
    void core_dump();

private:
    string m_root_path;
};

