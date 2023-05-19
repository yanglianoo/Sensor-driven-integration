#指定动态链接库目录
LIBPATH:=-L./sensor_lib
# boost 1.68.0
LIBPATH+=-L./imu/LORD-MicroStrain/c++-mscl/Boost/lib

# imu的动态库
LIB:=-lmscl
LIB+=-lboost_system
LIB+=-lboost_filesystem
LIB+=-lLpmsIG1_OpenSourceLib 


# camera的动态库
LIB+=-lrealsense2
#laser的库文件
LIB+=-lsl_lidar_sdk

# c++其他标准库
LIB+=-lstdc++
LIB+=-lpthread  #需要多线程库文件

#二次封装后的动态库名字
LIBM:=-lmiddleware



#编译头文件搜索路径
INCLUDE:=-Iimu/LORD-MicroStrain/c++-mscl/source 
INCLUDE+= -I.