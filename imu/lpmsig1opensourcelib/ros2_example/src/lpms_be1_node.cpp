#include <string>
#include <thread>
#include <chrono>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/imu.hpp"
#include "sensor_msgs/msg/magnetic_field.hpp"
#include "sensor_msgs/msg/nav_sat_fix.hpp"
#include "sensor_msgs/msg/nav_sat_status.hpp"
#include "std_srvs/srv/set_bool.hpp"
#include "std_srvs/srv/trigger.hpp"
#include "std_msgs/msg/bool.hpp"

#include "lpsensor/LpmsIG1I.h"
#include "lpsensor/SensorDataI.h"
#include "lpsensor/LpmsIG1Registers.h"

struct IG1Command
{
    short command;
    union Data {
        uint32_t i[64];
        float f[64];
        unsigned char c[256];
    } data;
    int dataLength;
};

class LpBE1Proxy: public rclcpp::Node
{
public:
    rclcpp::TimerBase::SharedPtr updateTimer;

    // Publisher
    rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imu_pub;
    rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr autocalibration_status_pub;

    // Service
    rclcpp::Service<std_srvs::srv::SetBool>::SharedPtr autocalibration_serv;
    rclcpp::Service<std_srvs::srv::SetBool>::SharedPtr autoReconnect_serv;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr gyrocalibration_serv;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr resetHeading_serv;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr getImuData_serv;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr setStreamingMode_serv;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr setCommandMode_serv;

    sensor_msgs::msg::Imu imu_msg;

    // Parameters
    std::string comportNo;
    int baudrate;
    bool autoReconnect;
    std::string frame_id;
    int rate;

    LpBE1Proxy()
        : Node("lpms_be1_node")
    {
        this->declare_parameter<std::string>("port", "/dev/ttyUSB0");
        this->declare_parameter<int>("baudrate", 115200); 
        this->declare_parameter<bool>("autoreconnect", true); 
        this->declare_parameter<std::string>("frame_id", "imu"); 
        this->declare_parameter<int>("rate", 200); 
        this->get_parameter("port", comportNo);
        this->get_parameter("baudrate", baudrate);
        this->get_parameter("autoreconnect", autoReconnect);
        this->get_parameter("frame_id", frame_id);
        this->get_parameter("rate", rate);

        // Create LpmsBE1 object 
        sensor1 = IG1Factory();
        sensor1->setVerbose(VERBOSE_INFO);
        sensor1->setAutoReconnectStatus(autoReconnect);

        RCLCPP_INFO(this->get_logger(), "Settings");
        RCLCPP_INFO(this->get_logger(), "Port: %s", comportNo.c_str());
        RCLCPP_INFO(this->get_logger(), "Baudrate: %d", baudrate);
        RCLCPP_INFO(this->get_logger(), "Auto reconnect: %s", autoReconnect? "Enabled":"Disabled");
        
        imu_pub = this->create_publisher<sensor_msgs::msg::Imu>("data",1);
        autocalibration_status_pub = this->create_publisher<std_msgs::msg::Bool>("is_autocalibration_active",1);

        autocalibration_serv = this->create_service<std_srvs::srv::SetBool>(
                "enable_gyro_autocalibration",std::bind(&LpBE1Proxy::setAutocalibration, this,std::placeholders::_1, std::placeholders::_2));
        autoReconnect_serv = this->create_service<std_srvs::srv::SetBool>(
                "enable_auto_reconnect",std::bind(&LpBE1Proxy::setAutoReconnect, this, std::placeholders::_1, std::placeholders::_2));
        gyrocalibration_serv = this->create_service<std_srvs::srv::Trigger>(
                "calibrate_gyroscope",std::bind(&LpBE1Proxy::calibrateGyroscope, this, std::placeholders::_1, std::placeholders::_2));
        resetHeading_serv = this->create_service<std_srvs::srv::Trigger>(
                "reset_heading",std::bind(&LpBE1Proxy::resetHeading, this, std::placeholders::_1, std::placeholders::_2));
        getImuData_serv = this->create_service<std_srvs::srv::Trigger>(
                "get_imu_data",std::bind(&LpBE1Proxy::getImuData, this, std::placeholders::_1, std::placeholders::_2));
        setStreamingMode_serv = this->create_service<std_srvs::srv::Trigger>(
                "set_streaming_mode",std::bind(&LpBE1Proxy::setStreamingMode, this, std::placeholders::_1, std::placeholders::_2));
        setCommandMode_serv = this->create_service<std_srvs::srv::Trigger>(
                "set_command_mode",std::bind(&LpBE1Proxy::setCommandMode, this, std::placeholders::_1, std::placeholders::_2));
        
        // Connects to sensor
        if (!sensor1->connect(comportNo, baudrate))
        {
            RCLCPP_ERROR(this->get_logger(), "Error connecting to sensor\n");
            sensor1->release();
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        }
        
        do
        {
            RCLCPP_INFO(this->get_logger(), "Waiting for sensor to connect %d", sensor1->getStatus());
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        } while(
            rclcpp::ok() &&
            (
                !(sensor1->getStatus() == STATUS_CONNECTED) && 
                !(sensor1->getStatus() == STATUS_CONNECTION_ERROR)
            )
        );

        if (sensor1->getStatus() == STATUS_CONNECTED)
        {
            RCLCPP_INFO(this->get_logger(), "Sensor connected");
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            sensor1->commandGotoStreamingMode();
        }
        else 
        {
            RCLCPP_INFO(this->get_logger(), "Sensor connection error: %d.", sensor1->getStatus());
            rclcpp::shutdown();
        }
    }

    ~LpBE1Proxy(void)
    {
        sensor1->release();
    }

    void update()
    {
        static bool runOnce = false;

        if (sensor1->getStatus() == STATUS_CONNECTED &&
                sensor1->hasImuData())
        {
            if (!runOnce)
            {
                publishIsAutocalibrationActive();
                runOnce = true;
            }
            IG1ImuDataI sd;
            sensor1->getImuData(sd);

            /* Fill the IMU message */

            // Fill the header
            //imu_msg.header.stamp = ros::Time::now();
            imu_msg.header.stamp = this->now();
            imu_msg.header.frame_id = frame_id;

            // Fill orientation quaternion
            imu_msg.orientation.w = sd.quaternion.data[0];
            imu_msg.orientation.x = -sd.quaternion.data[1];
            imu_msg.orientation.y = -sd.quaternion.data[2];
            imu_msg.orientation.z = -sd.quaternion.data[3];

            // Fill angular velocity data
            // - scale from deg/s to rad/s
            imu_msg.angular_velocity.x = sd.gyroIAlignmentCalibrated.data[0]*3.1415926/180;
            imu_msg.angular_velocity.y = sd.gyroIAlignmentCalibrated.data[1]*3.1415926/180;
            imu_msg.angular_velocity.z = sd.gyroIAlignmentCalibrated.data[2]*3.1415926/180;

            // Fill linear acceleration data
            imu_msg.linear_acceleration.x = -sd.accCalibrated.data[0]*9.81;
            imu_msg.linear_acceleration.y = -sd.accCalibrated.data[1]*9.81;
            imu_msg.linear_acceleration.z = -sd.accCalibrated.data[2]*9.81;

            // Publish the messages
            imu_pub->publish(imu_msg);
        }
    }

    void run(void)
    {
        updateTimer = this->create_wall_timer(
            std::chrono::milliseconds(1000 / abs(rate)),
            std::bind(&LpBE1Proxy::update, this));
    }

    void publishIsAutocalibrationActive()
    {
        std_msgs::msg::Bool msg;
        IG1SettingsI settings;
        sensor1->getSettings(settings);
        msg.data = settings.enableGyroAutocalibration;
        autocalibration_status_pub->publish(msg);
    }

    ///////////////////////////////////////////////////
    // Service Callbacks
    ///////////////////////////////////////////////////
    bool setAutocalibration (
        const std::shared_ptr<std_srvs::srv::SetBool::Request> req,
        std::shared_ptr<std_srvs::srv::SetBool::Response> res)
    {
        RCLCPP_INFO(this->get_logger(), "set_autocalibration");

        // clear current settings
        IG1SettingsI settings;
        sensor1->getSettings(settings);

        // Send command
        cmdSetEnableAutocalibration(req->data);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        cmdGetEnableAutocalibration();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        double retryElapsedTime = 0;
        int retryCount = 0;
        while (!sensor1->hasSettings()) 
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            RCLCPP_INFO(this->get_logger(), "set_autocalibration wait");
            
            retryElapsedTime += 0.1;
            if (retryElapsedTime > 2.0)
            {
                retryElapsedTime = 0;
                cmdGetEnableAutocalibration();
                retryCount++;
            }

            if (retryCount > 5)
                break;
        }
        RCLCPP_INFO(this->get_logger(), "set_autocalibration done");

        // Get settings
        sensor1->getSettings(settings);

        std::string msg;
        if (settings.enableGyroAutocalibration == req->data) 
        {
            res->success = true;
            msg.append(std::string("[Success] autocalibration status set to: ") + (settings.enableGyroAutocalibration?"True":"False"));
        }
        else 
        {
            res->success = false;
            msg.append(std::string("[Failed] current autocalibration status set to: ") + (settings.enableGyroAutocalibration?"True":"False"));
        }

        RCLCPP_INFO(this->get_logger(), "%s", msg.c_str());
        res->message = msg;

        publishIsAutocalibrationActive();
        return res->success;
    }

    // Auto reconnect
    bool setAutoReconnect (
        const std::shared_ptr<std_srvs::srv::SetBool::Request> req,
        std::shared_ptr<std_srvs::srv::SetBool::Response> res)
    {
        RCLCPP_INFO(this->get_logger(), "set_auto_reconnect");
        sensor1->setAutoReconnectStatus(req->data);
        
        res->success = true;
        std::string msg;
        msg.append(std::string("[Success] auto reconnection status set to: ") + (sensor1->getAutoReconnectStatus()?"True":"False"));
    
        RCLCPP_INFO(this->get_logger(), "%s", msg.c_str());
        res->message = msg;

        return res->success;
    }
    
    // reset heading
    bool resetHeading (
        const std::shared_ptr<std_srvs::srv::Trigger::Request> req,
        std::shared_ptr<std_srvs::srv::Trigger::Response> res)
    {
        RCLCPP_INFO(this->get_logger(), "reset_heading");
        
        // Send command
        cmdResetHeading();

        res->success = true;
        res->message = "[Success] Heading reset";
        return true;
    }


    bool calibrateGyroscope (
        const std::shared_ptr<std_srvs::srv::Trigger::Request> req,
        std::shared_ptr<std_srvs::srv::Trigger::Response> res)
    {
        RCLCPP_INFO(this->get_logger(), "calibrate_gyroscope: Please make sure the sensor is stationary for 4 seconds");

        cmdCalibrateGyroscope();

        std::this_thread::sleep_for(std::chrono::milliseconds(4000));
        res->success = true;
        res->message = "[Success] Gyroscope calibration procedure completed";
        RCLCPP_INFO(this->get_logger(), "calibrate_gyroscope: Gyroscope calibration procedure completed");
        return true;
    }
    
    bool getImuData (
        const std::shared_ptr<std_srvs::srv::Trigger::Request> req,
        std::shared_ptr<std_srvs::srv::Trigger::Response> res)
    {
        cmdGotoCommandMode();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        cmdGetImuData();
        res->success = true;
        res->message = "[Success] Get imu data";
        return true;
    }

    bool setStreamingMode (
        const std::shared_ptr<std_srvs::srv::Trigger::Request> req,
        std::shared_ptr<std_srvs::srv::Trigger::Response> res)
    {
        cmdGotoStreamingMode();
        res->success = true;
        res->message = "[Success] Set streaming mode";
        return true;
    }

    bool setCommandMode (
        const std::shared_ptr<std_srvs::srv::Trigger::Request> req,
        std::shared_ptr<std_srvs::srv::Trigger::Response> res)
    {
        cmdGotoCommandMode();
        res->success = true;
        res->message = "[Success] Set command mode";
        return true;
    }


    ///////////////////////////////////////////////////
    // Helpers
    ///////////////////////////////////////////////////

    void cmdGotoCommandMode ()
    {
        IG1Command cmd;
        cmd.command = GOTO_COMMAND_MODE;
        cmd.dataLength = 0;
        sensor1->sendCommand(cmd.command, cmd.dataLength, cmd.data.c);
    }

    void cmdGotoStreamingMode ()
    {
        IG1Command cmd;
        cmd.command = GOTO_STREAM_MODE;
        cmd.dataLength = 0;
        sensor1->sendCommand(cmd.command, cmd.dataLength, cmd.data.c);
    }

    void cmdGetImuData()
    {
        IG1Command cmd;
        cmd.command = GET_IMU_DATA;
        cmd.dataLength = 0;
        sensor1->sendCommand(cmd.command, cmd.dataLength, cmd.data.c);
    }

    void cmdCalibrateGyroscope()
    {
        IG1Command cmd;
        cmd.command = START_GYR_CALIBRATION;
        cmd.dataLength = 0;
        sensor1->sendCommand(cmd.command, cmd.dataLength, cmd.data.c);
    }

    void cmdResetHeading()
    {
        IG1Command cmd;
        cmd.command = SET_ORIENTATION_OFFSET;
        cmd.dataLength = 4;
        cmd.data.i[0] = LPMS_OFFSET_MODE_HEADING;
        sensor1->sendCommand(cmd.command, cmd.dataLength, cmd.data.c);
    }

    void cmdSetEnableAutocalibration(int status)
    {
        IG1Command cmd;
        cmd.command = SET_ENABLE_GYR_AUTOCALIBRATION;
        cmd.dataLength = 4;
        cmd.data.i[0] = status;
        sensor1->sendCommand(cmd.command, cmd.dataLength, cmd.data.c);
    }

    void cmdGetEnableAutocalibration()
    {
        IG1Command cmd;
        cmd.command = GET_ENABLE_GYR_AUTOCALIBRATION;
        cmd.dataLength = 0;
        sensor1->sendCommand(cmd.command, cmd.dataLength, cmd.data.c);
    }

 private:

    // Access to LPMS data
    IG1I* sensor1;
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    auto lpBE1 = std::make_shared<LpBE1Proxy>();
    lpBE1->run();
    rclcpp::spin(lpBE1);
    rclcpp::shutdown();
    return 0;
}
