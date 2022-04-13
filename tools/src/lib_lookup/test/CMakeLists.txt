CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
SET(PRJ test_lookup_table)
PROJECT(${PRJ})
SET(BOOST_ROOT "/usr/local/share/")
FIND_PACKAGE(Boost COMPONENTS thread regex program_options system REQUIRED)
FIND_LIBRARY(LOG4CPP_LIBRARY_PATH NAMES log4cpp PATHS "/usr/local/lib/")

if (UNIX AND NOT APPLE)
  INCLUDE(FindPkgConfig)
  PKG_CHECK_MODULES(LIBMYSQLCLIENT REQUIRED mysqlclient)
  INCLUDE_DIRECTORIES(${LIBMYSQLCLIENT_INCLUDE_DIRS})
  LINK_DIRECTORIES(${LIBMYSQLCLIENT_LIBRARY_DIRS})
endif()

INCLUDE_DIRECTORIES("/usr/local/share/include/")
INCLUDE_DIRECTORIES("/usr/local/include/")
INCLUDE_DIRECTORIES(("../../../../.."))
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -std=c++11 -O0 -g -DTEST_PRINT")

##
SET(TEST_SOURCE_FILES
        single_thread_test.cc
        multi_thread_test.cc
        basic_test.cc
        )

ADD_EXECUTABLE(${PRJ} ${TEST_SOURCE_FILES})
TARGET_LINK_LIBRARIES(${PRJ} gtest gtest_main)
TARGET_LINK_LIBRARIES(${PRJ} pthread)
TARGET_LINK_LIBRARIES(${PRJ} ${Boost_LIBRARIES})
TARGET_LINK_LIBRARIES(${PRJ} ${LOG4CPP_LIBRARY_PATH})
TARGET_LINK_LIBRARIES(${PRJ} mixipgw_tools_lookup)
TARGET_LINK_LIBRARIES(${PRJ} mixipgw_tools_misc)

ADD_TEST(lookup ${PRJ})
if (UNIX AND NOT APPLE)
    target_link_libraries(${PRJ} rt)
endif()
