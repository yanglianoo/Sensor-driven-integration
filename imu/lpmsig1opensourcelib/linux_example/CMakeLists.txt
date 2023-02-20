cmake_minimum_required(VERSION 2.4.6)

project (LpmsIG1Example)

if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
    cmake_policy(SET CMP0015 NEW)
endif(COMMAND cmake_policy)

set(CMAKE_BUILD_TYPE "Release")
set(BUILD_ARCHITECTURE "32-bit" CACHE STRING "")
set_property(CACHE BUILD_ARCHITECTURE PROPERTY STRINGS "32-bit" "64-bit")

set(IG1_INC "./..")
set(IG1_LIB "./lib")

include_directories("${IG1_INC}")
link_directories("${IG1_LIB}")

ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -std=gnu++11") 
endif()

set(headers
    ${IG1_INC}/LpMatrix.h
    ${IG1_INC}/LpmsIG1Registers.h
    ${IG1_INC}/LpmsIG1I.h
    ${IG1_INC}/SensorDataI.h
)

set(LpmsIG1_SimpleExample_SRCS
    ${IG1_INC}/LpMatrix.c
    LpmsIG1_SimpleExample.cpp
)

set(LpmsIG1_RS485Example_SRCS
    ${IG1_INC}/LpMatrix.c
    LpmsIG1_RS485Example.cpp
)

set(LpmsBEx_SimpleExample_SRCS
    ${IG1_INC}/LpMatrix.c
    LpmsBEx_SimpleExample.cpp
)

set(LpmsNAV3_SimpleExample_SRCS
    ${IG1_INC}/LpMatrix.c
    LpmsNAV3_SimpleExample.cpp
)

set(LpmsCURS3_SimpleExample_SRCS
    ${IG1_INC}/LpMatrix.c
    LpmsCURS3_SimpleExample.cpp
)

set(LpmsINC1_SimpleExample_SRCS
    ${IG1_INC}/LpMatrix.c
    LpmsINC1_SimpleExample.cpp
)

# LpmsIG1 Simple Example
add_executable(LpmsIG1_SimpleExample ${LpmsIG1_SimpleExample_SRCS} ${headers})
target_link_libraries(LpmsIG1_SimpleExample LpmsIG1_OpenSourceLib rt pthread udev)

# LpmsIG1_RS485 Example
add_executable(LpmsIG1_RS485Example ${LpmsIG1_RS485Example_SRCS} ${headers})
target_link_libraries(LpmsIG1_RS485Example LpmsIG1_OpenSourceLib rt pthread udev)

# LpmsBEx Simple Example
add_executable(LpmsBEx_SimpleExample ${LpmsBEx_SimpleExample_SRCS} ${headers})
target_link_libraries(LpmsBEx_SimpleExample LpmsIG1_OpenSourceLib rt pthread udev)


# LpmsNAV3 Simple Example
add_executable(LpmsNAV3_SimpleExample ${LpmsNAV3_SimpleExample_SRCS} ${headers})
target_link_libraries(LpmsNAV3_SimpleExample LpmsIG1_OpenSourceLib rt pthread udev)

# LpmsCURS3 Simple Example
add_executable(LpmsCURS3_SimpleExample ${LpmsCURS3_SimpleExample_SRCS} ${headers})
target_link_libraries(LpmsCURS3_SimpleExample LpmsIG1_OpenSourceLib rt pthread udev)

# LpmsINC1 Simple Example
add_executable(LpmsINC1_SimpleExample ${LpmsINC1_SimpleExample_SRCS} ${headers})
target_link_libraries(LpmsINC1_SimpleExample LpmsIG1_OpenSourceLib rt pthread udev)