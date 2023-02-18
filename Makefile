#头文件搜索路径
INCLUDE:=-Iimu/LORD-MicroStrain/c++-mscl/source 
INCLUDE+=-Iimu/LORD-MicroStrain/c++-mscl/Boost/include
INCLUDE+=-Iimu/LORD-MicroStrain/include
INCLUDE+=-Iinclude
#指定链接库目录
LIBPATH+=-L./lib
LIB:=-lmscl
LIB+=-lstdc++
LIB+=-lpthread  #需要多线程库文件
LIB+=-limu


SRC:=src

CFLAGS:= -std=c++11 #c++11


.PHONY:libimu.so
libimu.so:$(SRC)/imu.cpp
	g++ $(INCLUDE) $<  -fPIC -shared -o ./lib/$@ $(LIBPATH) $(LIB)
# 编译生成可执行文件
.PHONY:main
main:main.cpp

	g++ $(INCLUDE)  $< -o $@.out $(LIBPATH) $(LIB) -Wl,-rpath=./lib

.PHONY:clean
clean:
	rm -rf *.out *.so 
	rm -rf main

	


