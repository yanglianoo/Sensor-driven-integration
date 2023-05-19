#pragma once
#include <stdint.h>
// #include "MicroStrain.h"
#include "../imu/LORD-MicroStrain/MicroStrain.h"
#include "../imu/lpmsig1opensourcelib/LpmsIG1.h"
#include <iostream>
#include <string>

class IMU {
public:
    virtual void initialize(const std::string& port, uint32_t baudRate) = 0;
    virtual void GetData()=0;
    virtual ~IMU() {}
    struct Vector3 {
        float x;
        float y;
        float z;
    };
    Vector3 angular_velocity;
    Vector3 linear_acceleration;
};


class MicroStrain:public IMU
{   
    public:
        void GetData() override;
        void initialize(const std::string& port, uint32_t baudRate) override ;
        
    private:
        mscl::Connection connection;
        mscl::InertialNode node;
        
};


class LpmsIG1:public IMU
{
    public:
        void GetData() override;
        void initialize(const std::string& port, uint32_t baudRate) override ;
    private:
        std::string TAG;
        IG1I* sensor1;
        IG1ImuDataI sd;
};





