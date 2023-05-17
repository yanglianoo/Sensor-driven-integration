


.PHONY:sensor_monitor
sensor_monitor:sensor_monitor.cpp test.cpp
	g++ $^ -o $@.out `pkg-config --libs --cflags libudev`



.PHONY:camera_test
camera_test:camera_test.cpp
	g++ $^ -o $@.out `pkg-config --cflags --libs opencv4`


.PHONY:clean
clean:
	rm -rf *.out 