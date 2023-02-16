#! /bin/bash
#用来设置程序运行时找不到libmscl.so，添加libmscl.so的搜寻环境
echo 'LORD-MicroStrain enviroment configure!'
libmscl_path=$(pwd)
libmscl_path+='/c++-mscl'
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$libmscl_path


