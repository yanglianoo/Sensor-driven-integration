#include <sl_lidar.h>
#include "sl_lidar_driver.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
class lidar
{
public:
    virtual void init();
    /* virtual void connect(); */
    virtual void get_data();
private:
    /* data */

    
};


class SL_lidar : public lidar
{
public:
    void init(const char& port, sl_u32 baudRate) override;
    void get_data() override;
    void print_usage();
    void ctrlc(int);
private:
    const char * opt_is_channel = NULL; 
	const char * opt_channel = NULL;
    const char * opt_channel_param_first = NULL;
	sl_u32         opt_channel_param_second = 0;
    sl_u32         baudrateArray[2] = {115200, 256000};
    sl_result     op_result;
	int          opt_channel_type = CHANNEL_TYPE_SERIALPORT;
    IChannel* _channel;
    ILidarDriver * drv; 
	bool useArgcBaudrate = false;
    bool ctrl_c_pressed;
}
































































