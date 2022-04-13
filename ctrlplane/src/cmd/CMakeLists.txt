cmake_minimum_required(VERSION 3.1)

add_definitions(-DVERSION="1.00")
SET(PRJ mixi_pgw_ctrl_plane_delete_bearer)
PROJECT(${PRJ})

OPTION(DB_HOST "Config DB_HOST" OFF)
OPTION(SRCADDRESS "Config SRCADDRESS" OFF)
OPTION(DB_SERVERID "Config DB_SERVERID" OFF)
OPTION(DB_USER "Config DB_USER" OFF)
OPTION(DB_PSWD "Config DB_PSWD" OFF)
OPTION(DB_INST "Config DB_INST" OFF)
OPTION(DEBUG "if Debug." OFF)
OPTION(SINGLE_CREATE_SESSION "Single Create Session Worker" OFF)
OPTION(PRODUCTION "Production Environment" OFF)

IF(NOT DB_HOST)
  SET(DB_HOST "127.0.0.1")
ENDIF()
IF(NOT SRCADDRESS)
  SET(SRCADDRESS "192.168.10.10")
ENDIF()
IF(NOT DB_SERVERID)
  SET(DB_SERVERID "a00000")
ENDIF()
IF(NOT DB_USER)
  SET(DB_USER "mixipgw")
ENDIF()
IF(NOT DB_PSWD)
  SET(DB_PSWD "password")
ENDIF()
IF(NOT DB_INST)
  SET(DB_INST "mixipgw")
ENDIF()

MESSAGE(STATUS "DB_HOST=${DB_HOST}")
MESSAGE(STATUS "SRCADDRESS=${SRCADDRESS}")
MESSAGE(STATUS "DB_SERVERID=${DB_SERVERID}")
MESSAGE(STATUS "DB_USER=${DB_USER}")
MESSAGE(STATUS "DB_PSWD=${DB_PSWD}")
MESSAGE(STATUS "DB_INST=${DB_INST}")

SET(CMAKE_C_FLAGS_DEBUG "-Wall -D__DEBUG_PACKET__ -DDHOST=${DB_HOST} -DDSRCADDRESS=${SRCADDRESS} -DDSERVERID=${DB_SERVERID} -DDUSER=${DB_USER} -DDPSWD=${DB_PSWD} -DDINST=${DB_INST}")
SET(CMAKE_C_FLAGS "-Wall -D__DEBUG_PACKET__ -DDHOST=${DB_HOST} -DDSRCADDRESS=${SRCADDRESS} -DDSERVERID=${DB_SERVERID} -DDUSER=${DB_USER} -DDPSWD=${DB_PSWD} -DDINST=${DB_INST}")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -D__DEBUG_PACKET__ -DDHOST=${DB_HOST} -DDSRCADDRESS=${SRCADDRESS} -DDSERVERID=${DB_SERVERID} -DDUSER=${DB_USER} -DDPSWD=${DB_PSWD} -DDINST=${DB_INST}")


if (UNIX AND NOT APPLE)                                                          
    SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D__LINUX__")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__LINUX__")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__LINUX__")
endif()

IF (SINGLE_CREATE_SESSION)
    SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DSINGLE_CREATE_SESSION")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DSINGLE_CREATE_SESSION")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSINGLE_CREATE_SESSION")
ENDIF()

IF (NOT DEBUG)
    SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O2 ")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 ")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 ")
ELSE()
    SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -g ")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g ")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g ")
ENDIF()



INCLUDE_DIRECTORIES("../../inc")
INCLUDE_DIRECTORIES("/usr/local/include/")

### common source
SET(SOURCE_FILES
        ../cmn/panic.c
        ../pkt/gtpc_validate_header.c
        ../pkt/gtpc_header.c
        ../pkt/gtpc_item_func.c
        ../pkt/gtpc_item_func_v1.c
        ../pkt/gtpu_validate_header.c
        ../pkt/gtpu_header.c
        ../pkt/gtp_alloc_packet.c
        ../pkt/gtp_free_packet.c
        ../pkt/gtp_memory_status.c

        ../cmn/send.c
        ../cmn/deq_burst.c
        ../cmn/enq.c
        ../cmn/panic.c
        ../cmn/free_packet.c
        ../cmn/alloc_packet.c
        )








### c source.
SET(SOURCE_FILES
        ${SOURCE_FILES}
        cmd_delete_bearer.c)
ADD_EXECUTABLE(${PRJ} ${SOURCE_FILES})
TARGET_LINK_LIBRARIES(${PRJ} pthread)
TARGET_LINK_LIBRARIES(${PRJ} mysqlclient)
TARGET_LINK_LIBRARIES(${PRJ} event)
TARGET_LINK_LIBRARIES(${PRJ} event_pthreads)

if (UNIX AND NOT APPLE)
    target_link_libraries(${PRJ} rt)
else()
    target_link_libraries(${PRJ} ncurses)
endif()

