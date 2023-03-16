cmake_minimum_required(VERSION 3.14)

project(SensorDriver)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_BUILD_TYPE Release)

# Define a variable named "flags"
set(flags "imu" CACHE STRING "Compilation flags")

# 二次封装头文件搜索路径
include_directories("include")
link_directories(${PROJECT_SOURCE_DIR}/lib)

# Check the value of "flags" variable and compile accordingly
if(flags STREQUAL "imu")
    # 编译imu的动态库
    # imu的头文件路径
    include_directories("imu/LORD-MicroStrain/c++-mscl/source")
    include_directories("imu/LORD-MicroStrain/c++-mscl/Boost/include")
    include_directories("imu/LORD-MicroStrain/include")
    include_directories("imu/lpmsig1opensourcelib/header")


    #动态库路径
    link_directories(${PROJECT_SOURCE_DIR}/imu/LORD-MicroStrain/c++-mscl/Boost/lib)
    #添加源文件
    add_library(imu SHARED src/imu.cpp)

    #链接到动态库
    target_link_libraries(imu mscl LpmsIG1_OpenSourceLib boost_system boost_filesystem pthread)

elseif(flags STREQUAL "camera")

    find_package(OpenCV 4 REQUIRED)
    set(OpenCV_DIR /usr/local/include/opencv4)
    # 编译camera的动态库
    include_directories("camera/realsense")

    #添加源文件
    add_library(camera SHARED src/camera.cpp)

    target_link_libraries(camera realsense2 ${OpenCV_LIBS})
else()
    # Invalid value for "flags" variable
    message(FATAL_ERROR "Invalid value for flags: ${flags}")
endif()

# 设置输出路径
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)