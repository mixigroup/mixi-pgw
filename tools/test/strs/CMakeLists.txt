CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
SET(PRJ strs)
PROJECT(${PRJ})

INCLUDE_DIRECTORIES("../../deps/netmap/sys/")
INCLUDE_DIRECTORIES("/usr/local/include/")
INCLUDE_DIRECTORIES(("../../"))
INCLUDE_DIRECTORIES("/usr/local/share/include/")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -O0")

SET(SOURCE_FILES
        strs.cc
        strs_send.cc
        strs_setup.cc
        strs_recv.cc
        strs_observer.cc
        strs_send_packets.cc
        strs_recv_packets.cc strs_recv_send.cc)

if (__IPV6__)
    MESSAGE("ip version[6]")
    SET(__IPV6__, ON)
else()
    MESSAGE("ip version[4]")
endif()
MESSAGE("tag vlan on")


ADD_EXECUTABLE(${PRJ} ${SOURCE_FILES})
TARGET_LINK_LIBRARIES(${PRJ} pthread)
IF (UNIX AND NOT APPLE)
    TARGET_LINK_LIBRARIES(${PRJ} rt)
ENDIF()
