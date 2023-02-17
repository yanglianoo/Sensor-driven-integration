#头文件搜索路径
INCLUDE:=-Ic++-mscl/source 
INCLUDE+=-Ic++-mscl/Boost/include
INCLUDE+=-I/include
#指定静态链接库目录
LIBPATH:=-Lc++-mscl
LIB:=-lmscl
LIB+=-lstdc++
LIB+=-lpthread  #需要多线程库文件


CFLAGS:= -std=c++11 #c++11

.PHONY:test
test:test.cpp

	g++ $(CFLAGS) $(INCLUDE) $< -o $@.out $(LIBPATH) $(LIB)

.PHONY:clean
clean:
	rm -rf *.out 

	



