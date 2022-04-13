PROJECT("mixipgw_tools")
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

## LIST( APPEND CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS} -g -Wno-deprecated-declarations -D__WITHOUT_ECM__")
LIST( APPEND CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS} -g -Wno-deprecated-declarations ")
SET(CMAKE_C_FLAGS_DEBUG "-Wall -g -O0")
SET(BOOST_ROOT "/usr/local/share/")
SET(STANDALONE_BINLOG TRUE)


MESSAGE("SRC : ${CMAKE_SOURCE_DIR}")

SET(MYSQL_SRC_DIR "${CMAKE_SOURCE_DIR}/../deps/mysql-8.0.28/")
INCLUDE_DIRECTORIES("/usr/local/share/include/")
INCLUDE_DIRECTORIES("inc")
FIND_PACKAGE(PkgConfig REQUIRED)


LINK_DIRECTORIES("../deps/mysql-8.0.28/_build/library_output_directory/")
#LINK_DIRECTORIES("../deps/mysql-8.0.28/_build/archive_output_directory/")
INCLUDE_DIRECTORIES("../deps/mysql-8.0.28/_build/include/")
INCLUDE_DIRECTORIES("../deps/mysql-8.0.28/include/")

SET(__DATAPLANE_DEBUG_, ON)

if (__DEBUG__)
  MESSAGE("debug on")
  SET(__DEBUG__, ON)
else()
  MESSAGE("debug off")
endif()

OPTION(TEST "Build all tests." ON)

SUBDIRS(src/lib)
SUBDIRS(src/lib_db)
SUBDIRS(src/lib_lookup)
SUBDIRS(src/lib_srv)
SUBDIRS(src/mod/mod_sgw)
SUBDIRS(src/mod/mod_radius)
SUBDIRS(src/mod/mod_diameter)

ENABLE_TESTING()
