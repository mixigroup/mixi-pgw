cmake_minimum_required(VERSION 3.0)
set(PRJ sgw_tun)
project(${PRJ})

set(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O0 -g")
INCLUDE_DIRECTORIES("../../../inc")
INCLUDE_DIRECTORIES("../../../")
LINK_DIRECTORIES("../../../_build/src/lib/")

set(SOURCE_FILES
        main.cc
        utun.cc
        udp.cc
        gtpc.cc
        )
add_executable(${PRJ} ${SOURCE_FILES})
TARGET_LINK_LIBRARIES(${PRJ} pthread)
TARGET_LINK_LIBRARIES(${PRJ} mixi-pgw-misc)

