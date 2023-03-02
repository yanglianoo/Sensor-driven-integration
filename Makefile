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
LIBPATH:=-L./lib
# boost 1.68.0
LIBPATH+=-L./imu/LORD-MicroStrain/c++-mscl/Boost/lib

# imu的动态库
LIBimu:=-lmscl
LIBimu+=-lLpmsIG1_OpenSourceLib 
LIBimu+=-lboost_system
LIBimu+=-lboost_filesystem

# camera的动态库
LIBcamera:=-lrealsense2

# c++其他标准库
LIB:=-lstdc++
LIB+=-lpthread  #需要多线程库文件

#二次封装后的动态库名字
LIBU:=-lmiddleware


SRC:=src

CFLAGS:= -std=c++11 #c++11

# 生成libimu.so动态库
.PHONY:libimu.so
libimu.so:$(SRC)/imu.cpp	
	g++ $(INCLUDE) $^ -fPIC -shared -o ./lib/$@ $(LIBPATH) $(LIB) $(LIBimu) $(CFLAGS)

# 生成libcamera.so动态库
.PHONY:libcamera.so
libcamera.so:$(SRC)/camera.cpp	
	g++ $(INCLUDE) $^ -fPIC -shared -o ./lib/$@ $(LIBPATH) $(LIB) $(LIBcamera) $(CFLAGS)




# 生成libmiddleware动态库  感觉没必要将 imu 和 camera 打包在一起
.PHONY:libmiddleware.so
libmiddleware.so:$(SRC)/*.cpp
	g++ $(INCLUDE) $^ -fPIC -shared -o ./lib/$@ $(LIBPATH) $(LIB) $(LIBimu) $(LIBcameara) $(CFLAGS)




# 编译生成可执行文件
.PHONY:camera
camera:test_camera.cpp
	g++ $(INCLUDE)  $< -o $@.out $(LIBPATH) $(LIB) $(LIBcamera) -lcamera $(CFLAGS)  `pkg-config --cflags --libs opencv4`

.PHONY:imu
imu:test_imu.cpp
	g++ $(INCLUDE)  $< -o $@.out $(LIBPATH) $(LIB) $(LIBimu) -limu $(CFLAGS) 

.PHONY:clean
clean:
	rm -rf *.out *.so 
	rm -rf main
	rm -rf test_camera


	



