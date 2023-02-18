#include <iostream>
#include "mscl/mscl.h"
#include <vector>
#include "include/MicroStrain.h"
using namespace std;
using namespace mscl;
int main()
{      
    try
    {
            
        mscl::Connection connection = mscl::Connection::Serial("/dev/ttyUSB0",115200);
        
        //建立串口通信的node
        mscl::InertialNode node(connection);
        //ping the Node
        bool success = node.ping();
        cout<<"通信是否成功："<< success <<endl;
        cout << "Node Information: " << endl;
        cout << "Model Name: " << node.modelName() << endl;
        cout << "Model Number: " << node.modelNumber() << endl;
        cout << "Serial: " << node.serialNumber() << endl;
        cout << "Firmware: " << node.firmwareVersion().str() << endl << endl;
        //配置IMU信息
        setCurrentConfig(node);
        //获取IMU配置信息
        getCurrentConfig(node);
        //开始数据采样
        startSampling(node);
        //挂起
        setToIdle(node);
        //获取数据
        parseData(node);


    }
    catch(mscl::Error& e)
    {
        std::cerr << e.what() << '\n';
    }

    return 0;
    }





