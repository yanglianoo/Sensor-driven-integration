#LORD-MicroStrain头文件搜索路径
INCLUDE:=-Iimu/LORD-MicroStrain/c++-mscl/source 
INCLUDE+=-Iimu/LORD-MicroStrain/c++-mscl/Boost/include
INCLUDE+=-Iimu/LORD-MicroStrain/include
#LpmsIG1 头文件搜索路径
INCLUDE+=-Iimu/lpmsig1opensourcelib/header
#RealSenseD435 头文件搜索路径
INCLUDE+=-Icamera/realsense
# 二次封装头文件搜索路径
INCLUDE+=-Iinclude
#指定动态链接库目录
LIBPATH=-L./lib
LIB:=-lmscl
LIB+=-lstdc++
LIB+=-lpthread  #需要多线程库文件
LIB+=-lLpmsIG1_OpenSourceLib 
LIB+=-lrealsense2
#二次封装后的动态库名字
LIBU=-lmiddleware


SRC:=src

CFLAGS:= -std=c++11 #c++11

# 生成libmiddleware动态库
.PHONY:libmiddleware.so
libmiddleware.so:$(SRC)/*.cpp
	g++ $(INCLUDE) $^ -fPIC -shared -o ./lib/$@ $(LIBPATH) $(LIB) $(CFLAGS)

# 编译生成可执行文件
.PHONY:camera
camera:test_camera.cpp
	g++ $(INCLUDE)  $< -o $@.out $(LIBPATH) $(LIB) $(LIBU) -Wl,-rpath=./lib $(CFLAGS)  `pkg-config --cflags --libs opencv4`

.PHONY:imu
imu:test_imu.cpp
	g++ $(INCLUDE)  $< -o $@.out $(LIBPATH) $(LIB) $(LIBU) -Wl,-rpath=./lib $(CFLAGS) 


.PHONY:clean
clean:
	rm -rf *.out *.so 
	rm -rf main

	



