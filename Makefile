include Path.mk

SENSOR_DRIVER:=sensor_driver
SENSOR_MANAGE:=sensor_manage
UTILITY:=utility
#指定编译器
CC = g++
#编译选项
CFLAGS:= -std=c++11 #c++11

# 生成libmiddleware动态库  
.PHONY:libmiddleware.so
libmiddleware.so:$(SENSOR_DRIVER)/*.cpp $(SENSOR_MANAGE)/*.cpp $(UTILITY)/*.cpp
	$(CC) $(INCLUDE) $^ -fPIC -shared -o ./sensor_lib/$@ $(LIBPATH) $(LIB)  $(CFLAGS) `pkg-config --cflags --libs opencv4`


# 编译生成可执行文件
.PHONY:camera
camera:test_camera.cpp
	$(CC)  $(INCLUDE)  $< -o $@.out $(LIBPATH) $(LIB) $(LIBM) $(CFLAGS)   `pkg-config --cflags --libs opencv4`


.PHONY:laser
laser:test_laser.cpp
	$(CC)  $(INCLUDE)  $< -o $@.out $(LIBPATH) $(LIB) $(LIBM) $(CFLAGS) 


.PHONY:imu
imu:test_imu.cpp
	$(CC)  $(INCLUDE)  $< -o $@.out $(LIBPATH) $(LIB) $(LIBM) $(CFLAGS) 


.PHONY:clean
clean:
	rm -rf *.out 
	rm -rf build


	



