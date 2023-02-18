#include "imu.h"
#include <iostream>
using namespace std;


void MicroStrain::initialize(const std::string& port, uint32_t baudRate) 
{
    //建立串口通信的node
    connection = mscl::Connection::Serial(port,baudRate);
    mscl::InertialNode node(connection);
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
    // setToIdle(node);
    //获取数据
        while(true)
            {
            //get all the data packets from the node, with a timeout of 500 milliseconds
            mscl::MipDataPackets packets = node.getDataPackets();

            for(mscl::MipDataPacket packet : packets)
            {
                //print out the data
                cout << "Packet Received: "<<endl;

                //get the data in the packet
                mscl::MipDataPoints data = packet.data();
                mscl::MipDataPoint dataPoint;

                //loop through all the data points in the packet
                for(unsigned int itr = 0; itr < data.size(); itr++)
                {
                    dataPoint = data[itr];

                    // cout << dataPoint.channelName() << ": ";
                    if(dataPoint.channelName() == "scaledAccelX")
                    {
                        angular_velocity.x=dataPoint.as_float();
                        cout<<"scaledAccelX:"<<linear_acceleration.x<<endl;
                    }
                    if(dataPoint.channelName() == "scaledAccelY")
                    {
                        angular_velocity.x=dataPoint.as_float();
                        cout<<"scaledAccelY:"<<linear_acceleration.y<<endl;
                    }
                    if(dataPoint.channelName() == "scaledAccelZ")
                    {
                        angular_velocity.x=dataPoint.as_float();
                        cout<<"scaledAccelZ:"<<linear_acceleration.z<<endl;
                    }

                    if(dataPoint.channelName() == "scaledGyroX")
                    {
                        angular_velocity.x=dataPoint.as_float();
                        cout<<"scaledGyroX:"<<angular_velocity.x<<endl;
                    }
                    if(dataPoint.channelName() == "scaledGyroY")
                    {
                        angular_velocity.x=dataPoint.as_float();
                        cout<<"scaledGyroY:"<<angular_velocity.y<<endl;
                    }
                    if(dataPoint.channelName() == "scaledGyroX")
                    {
                        angular_velocity.x=dataPoint.as_float();
                        cout<<"scaledGyroZ:"<<angular_velocity.z<<endl;
                    }

                    //print out the channel data
                    //Note: The as_string() function is being used here for simplicity. 
                    //      Other methods (ie. as_float, as_uint16, as_Vector) are also available.
                    //      To determine the format that a dataPoint is stored in, use dataPoint.storedAs().
                    // cout << dataPoint.as_float() << " ";

                    //if the dataPoint is invalid
                    if(!dataPoint.valid())
                    {
                        cout << "[Invalid] ";
                    }
                }
                cout << endl;
            }
        }

}