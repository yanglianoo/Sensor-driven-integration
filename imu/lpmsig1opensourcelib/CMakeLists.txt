#***********************************************************************
#* Copyright (C) 2019 LP-Research
#* All rights reserved.
#* Contact: LP-Research (info@lp-research.com)
#*
#* This file is part of the Open Motion Analysis Toolkit (OpenMAT).
#*
#* Redistribution and use in source and binary forms, with 
#* or without modification, are permitted provided that the 
#* following conditions are met:
#*
#* Redistributions of source code must retain the above copyright 
#* notice, this list of conditions and the following disclaimer.
#* Redistributions in binary form must reproduce the above copyright 
#* notice, this list of conditions and the following disclaimer in 
#* the documentation and/or other materials provided with the 
#* distribution.
#*
#* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
#* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
#* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
#* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
#* HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
#* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
#* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
#* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
#* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
#* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
#* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#**********************************************************************/

cmake_minimum_required(VERSION 2.4.6)


set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release" )

set(BUILD_ARCHITECTURE "32-bit" CACHE STRING "")
#set(BUILD_ARCHITECTURE "64-bit" CACHE STRING "")
set_property(CACHE BUILD_ARCHITECTURE PROPERTY STRINGS "32-bit" "64-bit")

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    project (LpmsIG1_OpenSourceLib)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    project (LpmsIG1_OpenSourceLibD)
endif()


include_directories("./header")


if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(sources
        src/MicroMeasure.cpp
        src/SerialPortLinux.cpp
        src/LpmsIG1.cpp
        src/LpMatrix.c
        src/LpUtil.cpp
        src/version.rc
    )
endif()

  
set(headers
    header/MicroMeasure.h
    header/SerialPort.h
    header/SensorData.h
    header/SensorDataI.h
    header/LpUtil.h
    header/LpmsIG1.h
    header/LpmsIG1I.h
    header/LpmsIG1Registers.h
    header/LpMatrix.h
    header/LpLog.h
)


if (BUILD_ARCHITECTURE STREQUAL "32-bit")
    set(LIBRARY_OUTPUT_PATH ./ CACHE STRING "")
    set(EXECUTABLE_OUTPUT_PATH ./ CACHE STRING "")
endif()

if (BUILD_ARCHITECTURE STREQUAL "64-bit")
    set(LIBRARY_OUTPUT_PATH ./ CACHE STRING "")
    set(EXECUTABLE_OUTPUT_PATH ./ CACHE STRING "")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -std=gnu++11") 
    #SET ( CMAKE_CXX_FLAGS "-D_GLIBCXX_USE_CXX11_ABI=1" )
endif()


if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_library(${CMAKE_PROJECT_NAME} SHARED ${sources} ${headers})
        target_link_libraries(${CMAKE_PROJECT_NAME} pthread)
        target_link_libraries(${CMAKE_PROJECT_NAME} rt)
        target_link_libraries(${CMAKE_PROJECT_NAME} udev)
    endif()

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_library(${CMAKE_PROJECT_NAME} SHARED ${sources} ${headers})
        target_link_libraries(${CMAKE_PROJECT_NAME} pthread)
        target_link_libraries(${CMAKE_PROJECT_NAME} rt)
        target_link_libraries(${CMAKE_PROJECT_NAME} udev)
    endif()
# # 安装到 /usr/include/lpsensor
#     install(FILES ${headers_linux_install} DESTINATION include/lpsensor)
# # 安装到 /usr/lib  
#     install(TARGETS ${CMAKE_PROJECT_NAME}
#         RUNTIME DESTINATION bin
#         LIBRARY DESTINATION lib
#         ARCHIVE DESTINATION lib
#     )
endif()

    # include(InstallRequiredSystemLibraries)
    # set(MAJOR_VERSION "0")
    # set(MINOR_VERSION "3")
    # set(PATCH_VERSION "2")
    # SET(CPACK_GENERATOR "DEB")
    # SET(CPACK_PACKAGE_NAME "libLpmsIG1_OpenSource")
    # set(CPACK_PACKAGE_VENDOR "LP-Research Inc. <www.lp-research.com>")
    # set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Library for communicating and interfacing with LP-Research sensors.")
    # set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/SUMMARY.txt")
    # # set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
    # set(CPACK_PACKAGE_VERSION_MAJOR "${MAJOR_VERSION}")
    # set(CPACK_PACKAGE_VERSION_MINOR "${MINOR_VERSION}")
    # set(CPACK_PACKAGE_VERSION_PATCH "${PATCH_VERSION}")
    # SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "H.E. YAP <yap@lp-research.com>") #required
    
    # include(CPack)
