#pragma once

#include "../c++-mscl/source/mscl/mscl.h"

static void setCurrentConfig(mscl::InertialNode& node)
{
    //many other settings are available than shown below
    //reference the documentation for the full list of commands

    //if the node supports AHRS/IMU
    if(node.features().supportsCategory(mscl::MipTypes::CLASS_AHRS_IMU))
    {   
        //加速度计和陀螺仪
        mscl::MipChannels ahrsImuChs;
        ahrsImuChs.push_back(mscl::MipChannel(mscl::MipTypes::CH_FIELD_SENSOR_SCALED_ACCEL_VEC, mscl::SampleRate::Hertz(500)));   
        ahrsImuChs.push_back(mscl::MipChannel(mscl::MipTypes::CH_FIELD_SENSOR_SCALED_GYRO_VEC, mscl::SampleRate::Hertz(100)));
        
        //apply to the node
        node.setActiveChannelFields(mscl::MipTypes::CLASS_AHRS_IMU, ahrsImuChs);
        cout<<"AHRS/IMU setconfig success!"<<endl;
    }

    //if the node supports Estimation Filter
    if(node.features().supportsCategory(mscl::MipTypes::CLASS_ESTFILTER))
    {
        mscl::MipChannels estFilterChs;
        estFilterChs.push_back(mscl::MipChannel(mscl::MipTypes::CH_FIELD_ESTFILTER_ESTIMATED_GYRO_BIAS, mscl::SampleRate::Hertz(100)));

        //apply to the node
        node.setActiveChannelFields(mscl::MipTypes::CLASS_ESTFILTER, estFilterChs);
        cout<<"Estimation Filter setconfig success!"<<endl;
    }

    // //if the node supports GNSS 不支持
    // if(node.features().supportsCategory(mscl::MipTypes::CLASS_GNSS))
    // {
    //     mscl::MipChannels gnssChs;
    //     gnssChs.push_back(mscl::MipChannel(mscl::MipTypes::CH_FIELD_GNSS_LLH_POSITION, mscl::SampleRate::Hertz(1)));

    //     //apply to the node
    //     node.setActiveChannelFields(mscl::MipTypes::CLASS_GNSS, gnssChs);
    //     cout<<"GNSS setconfig success!"<<endl;
    // }

    node.setPitchRollAid(true);

    // node.setAltitudeAid(false);

    // mscl::PositionOffset offset(0.0f, 0.0f, 0.0f);
    // node.setAntennaOffset(offset);
}