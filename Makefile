#LORD-MicroStrain头文件搜索路径
INCLUDE:=-Iimu/LORD-MicroStrain/c++-mscl/source 
INCLUDE+=-Iimu/LORD-MicroStrain/c++-mscl/Boost/include
INCLUDE+=-Iimu/LORD-MicroStrain/include
INCLUDE+=-Iinclude
#LpmsIG1 头文件搜索路径
INCLUDE+=-Iimu/lpmsig1opensourcelib/header
#RealSenseD435 头文件搜索路径
INCLUDE+=-Icamera/realsense/librealsense2
INCLUDE+=-Icamera/realsense/librealsense2-gl
#指定动态链接库目录
LIBPATH+=-L./lib
LIB:=-lmscl
LIB+=-lstdc++
LIB+=-lpthread  #需要多线程库文件
LIB+=-lLpmsIG1_OpenSourceLib 
LIB+=-limu
LIB+=-lrealsense2


SRC:=src

CFLAGS:= -std=c++11 #c++11

# 生成liblmu动态库
.PHONY:libimu.so
libimu.so:$(SRC)/imu.cpp
	g++ $(INCLUDE) $<  -fPIC -shared -o ./lib/$@ $(LIBPATH) $(LIB) $(CFLAGS)
# 编译生成可执行文件
.PHONY:main
main:main.cpp
	g++ $(INCLUDE)  $< -o $@.out $(LIBPATH) $(LIB) -Wl,-rpath=./lib $(CFLAGS) `pkg-config --cflags --libs opencv4`

.PHONY:clean
clean:
	rm -rf *.out *.so 
	rm -rf main

	



