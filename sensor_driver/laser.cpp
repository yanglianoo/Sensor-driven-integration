#include "laser.hpp"
#ifndef _countof
#define _countof(_Array) (int)(sizeof(_Array) / sizeof(_Array[0]))
#endif

#include <unistd.h>
static inline void delay(sl_word_size_t ms){
    while (ms>=1000){
        usleep(1000*1000);
        ms-=1000;
    };
    if (ms!=0)
        usleep(ms*1000);
}



void SL_lidar::ctrlc(int)
{
    ctrl_c_pressed = true;
}

void SL_lidar::init(const char* port,sl_u32 baudrate){
    opt_is_channel = NULL; 
    opt_channel = NULL;
    opt_channel_param_first = NULL;
	opt_channel_param_second = 0;
 	opt_channel_type = sl::CHANNEL_TYPE_SERIALPORT;
    opt_channel_param_first = port;
    // opt_channel_param_second = strtoul(baudrate,NULL,10);
    opt_channel_param_second = baudrate;
    if(!opt_channel_param_first){
        opt_channel_param_first = "/dev/ttyUSB0";
    }
    drv = *createLidarDriver();
    if (!drv) {
        fprintf(stderr, "insufficent memory, exit\n");
        exit(-2);
    }
    _channel = (*createSerialPortChannel(opt_channel_param_first,opt_channel_param_second));
    if (SL_IS_OK((drv)->connect(_channel))) {
        op_result = drv->getDeviceInfo(devinfo);

        if (SL_IS_OK(op_result)) 
        {
	       connectSuccess = true;
        }
    else{
            delete drv;
		    drv = NULL;
        }
    }
 if (!connectSuccess) {
        (opt_channel_type == CHANNEL_TYPE_SERIALPORT)?
			(fprintf(stderr, "Error, cannot bind to the specified serial port %s.\n"
				, opt_channel_param_first)):(fprintf(stderr, "Error, cannot connect to the specified ip addr %s.\n"
				, opt_channel_param_first));
		
        //goto on_finished;
    }

    // print out the device serial number, firmware and hardware version number..
    printf("SLAMTEC LIDAR S/N: ");
    for (int pos = 0; pos < 16 ;++pos) {
        printf("%02X", devinfo.serialnum[pos]);
    }

    printf("\n"
            "Firmware Ver: %d.%02d\n"
            "Hardware Rev: %d\n"
            , devinfo.firmware_version>>8
            , devinfo.firmware_version & 0xFF
            , (int)devinfo.hardware_version);


 }
void SL_lidar::get_data(){
    if (!drv) {
        fprintf(stderr, "insufficent memory, exit\n");
        exit(-2);
    }
    if(opt_channel_type == CHANNEL_TYPE_SERIALPORT)
        drv->setMotorSpeed();
    // start scan...
    drv->startScan(0,1);
    while (1) {
        sl_lidar_response_measurement_node_hq_t nodes[8192];
        size_t   count = _countof(nodes);

      op_result = drv->grabScanDataHq(nodes, count);
          
        if (SL_IS_OK(op_result)) {
            drv->ascendScanData(nodes, count);
            for (int pos = 0; pos < (int)count ; ++pos) {
                printf("%s theta: %03.2f Dist: %08.2f Q: %d \n", 
                    (nodes[pos].flag & SL_LIDAR_RESP_HQ_FLAG_SYNCBIT) ?"S ":"  ", 
                    (nodes[pos].angle_z_q14 * 90.f) / 16384.f,
                    nodes[pos].dist_mm_q2/4.0f,
                    nodes[pos].quality >> SL_LIDAR_RESP_MEASUREMENT_QUALITY_SHIFT);
            } 
        }

        if (ctrl_c_pressed){ 
            break;
        }
    }
    drv->stop();
	delay(200);
	if(opt_channel_type == CHANNEL_TYPE_SERIALPORT)
        drv->setMotorSpeed(0);
    // done!
on_finished:
    if(drv) {
        delete drv;
        drv = NULL;
    }

}
    