#! /bin/bash
libmiddleware_path=$(pwd)
libmiddleware_path+='/sensor_lib'
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$libmiddleware_path
libboost_path=$(pwd)
libboost_path+='/imu/LORD-MicroStrain/c++-mscl/Boost/lib'
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$libboost_path
# echo "1" | sudo chmod 777 /dev/ttyUSB*
# echo '设置串口权限成功！'

