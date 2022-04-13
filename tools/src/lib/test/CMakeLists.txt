CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
SET(PRJ test_basic)
PROJECT(${PRJ})
SET(BOOST_ROOT "/usr/local/share/")
FIND_PACKAGE(Boost COMPONENTS thread regex program_options system REQUIRED)
FIND_LIBRARY(LOG4CPP_LIBRARY_PATH NAMES log4cpp PATHS "/usr/local/lib/")

LINK_DIRECTORIES("../../../../deps/mysql-binlog-events/static_link")


INCLUDE_DIRECTORIES("./deps/picojson/")
INCLUDE_DIRECTORIES("/usr/local/share/include/")
INCLUDE_DIRECTORIES("../../../deps/netmap/sys/")
INCLUDE_DIRECTORIES("/usr/local/include/")
INCLUDE_DIRECTORIES(("../../../../.."))
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2 -DTEST_PRINT")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -std=c++11 -O0 -g -DTEST_PRINT")
##
SET(TEST_SOURCE_FILES
        module_test.cc
        logger_test.cc
        basic_test.cc
        filter_test.cc
        counter_test.cc
        vlan_extract_test.cc
        policer_test.cc
        filter_policy_test.cc
##      mysql_proc_test.cc
        )

ADD_EXECUTABLE(${PRJ} ${TEST_SOURCE_FILES})
TARGET_LINK_LIBRARIES(${PRJ} gtest gtest_main)
TARGET_LINK_LIBRARIES(${PRJ} pthread)
TARGET_LINK_LIBRARIES(${PRJ} ${Boost_LIBRARIES})
TARGET_LINK_LIBRARIES(${PRJ} ${LOG4CPP_LIBRARY_PATH})
TARGET_LINK_LIBRARIES(${PRJ} mysqlclient)
TARGET_LINK_LIBRARIES(${PRJ} mixi-pgw-misc)
TARGET_LINK_LIBRARIES(${PRJ} mixi-pgw-lookup)
#TARGET_LINK_LIBRARIES(${PRJ} mixipgw_tools_db)


ADD_TEST(misc ${PRJ})
target_link_libraries(${PRJ} rt)
