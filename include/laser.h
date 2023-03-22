#include "sl_lidar.h"
#include "sl_lidar_driver.h"
#include "sl_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

using namespace sl;


class lidar
{
public:
    virtual void init(const char* port, sl_u32 baudRate) = 0;
    virtual void get_data() = 0;

};


class SL_lidar:public lidar
{
public:
    void init(const char* port, sl_u32 baudRate) override;
    void get_data() override;
    void ctrlc(int);
    // SL_lidar();
private:
    const char * opt_is_channel; 
	const char * opt_channel;
    const char * opt_channel_param_first;
	sl_u32        opt_channel_param_second;
    sl_u32        baudrateArray[2] = {115200, 256000};
    sl_result     op_result;
	int          opt_channel_type;
    IChannel* _channel;
    ILidarDriver * drv; 
    // ILidarDriver * drv = *createLidarDriver();
    sl_lidar_response_device_info_t devinfo;
    bool ctrl_c_pressed;
    bool connectSuccess = false;
    // sl_lidar_response_measurement_node_hq_t nodes[8192];
    // size_t   count ;
};
































































