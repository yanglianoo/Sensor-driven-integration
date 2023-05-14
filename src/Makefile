

CFLAGS:= -std=c++11 #c++11
INCLUDE+=-I../include
LIBPATH:=-L../lib
# boost 1.68.0
LIBPATH+=-L./imu/LORD-MicroStrain/c++-mscl/Boost/lib
# c++其他标准库
LIB:=-lstdc++
LIB+=-lpthread  #需要多线程库文件
LIBimu:=-lmscl
LIBimu+=-lboost_system
LIBimu+=-lboost_filesystem
.PHONY:libimu.so
libimu.so:imu.cpp	
	g++ $(INCLUDE) $^ -fPIC -shared -o ./lib/$@ $(LIBPATH) $(LIB) $(LIBimu) $(CFLAGS)