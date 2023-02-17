#头文件搜索路径
INCLUDE:=-Iimu/LORD-MicroStrain/c++-mscl/source 
INCLUDE+=-Iimu/LORD-MicroStrain/c++-mscl/Boost/include
INCLUDE+=-Iimu/LORD-MicroStrain/include
INCLUDE+=-Iinclude
#指定静态链接库目录
LIBPATH:=-Limu/LORD-MicroStrain/c++-mscl
LIB:=-lmscl
LIB+=-lstdc++
LIB+=-lpthread  #需要多线程库文件


CFLAGS:= -std=c++11 #c++11
# 编译生成可执行文件
.PHONY:main
main:src/main.cpp

	g++ $(CFLAGS) $(INCLUDE) $< -o $@.out $(LIBPATH) $(LIB)

.PHONY:clean
clean:
	rm -rf *.out 

	



