CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
SET(PRJ mixi-pgw-sgw-sim)
PROJECT(${PRJ})

SET(BOOST_ROOT "/usr/local/share/")
FIND_PACKAGE(Boost COMPONENTS thread regex program_options system REQUIRED)
FIND_LIBRARY(LOG4CPP_LIBRARY_PATH NAMES log4cpp PATHS "/usr/local/lib/")

INCLUDE_DIRECTORIES("../../deps/netmap/sys/")
INCLUDE_DIRECTORIES("/usr/local/include/")
INCLUDE_DIRECTORIES(("../../../inc/"))
INCLUDE_DIRECTORIES(("../../../"))
INCLUDE_DIRECTORIES("/usr/local/share/include/")


#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O0 -g -D__WITHOUT_ECM__")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2 -D__WITHOUT_ECM__")

SET(SOURCE_FILES
        mixipgw_mod_sgw.cc
        func/mixipgw_init.cc
        func/mixipgw_menu.cc
        func/mixipgw_gtpc_recv.cc
        func/mixipgw_gtpu_recv.cc
        func/mixipgw_gtpu_send.cc
        func/mixipgw_gtpc_send_create_session.cc
        func/mixipgw_gtpc_send_delete_session.cc
        func/mixipgw_bfd_recv.cc
        func/mixipgw_gtpc_send_echo.cc
        func/mixipgw_gtpu_send_echo.cc
        func/mixipgw_gtpc_send_modify_bearer.cc
        func/mixipgw_gtpc_send_delete_bearer.cc
        func/mixipgw_gtpc_send_resume_notification.cc
        func/mixipgw_gtpc_send_suspend_notification.cc
   )
ADD_EXECUTABLE(${PRJ} ${SOURCE_FILES})
TARGET_LINK_LIBRARIES(${PRJ} pthread)
TARGET_LINK_LIBRARIES(${PRJ} ${Boost_LIBRARIES})
TARGET_LINK_LIBRARIES(${PRJ} ${LOG4CPP_LIBRARY_PATH})
TARGET_LINK_LIBRARIES(${PRJ} mixi-pgw-misc)
TARGET_LINK_LIBRARIES(${PRJ} mixi-pgw-server)
TARGET_LINK_LIBRARIES(${PRJ} mixi-pgw-db)
TARGET_LINK_LIBRARIES(${PRJ} mysqlclient)
target_link_libraries(${PRJ} rt)
target_link_libraries(${PRJ} crypto)
target_link_libraries(${PRJ} ssl)
target_link_libraries(${PRJ} dl)

