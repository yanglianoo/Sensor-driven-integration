#pragma once
#include <stdint.h>
#include "MicroStrain.h"
#include "LpmsIG1.h"


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
        string TAG;
        IG1I* sensor1;
        IG1ImuDataI sd;
};





