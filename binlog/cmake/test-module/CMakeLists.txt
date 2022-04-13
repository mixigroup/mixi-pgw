CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

SET(PRJ binlog-read)
PROJECT(${PRJ})

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -o0 -DDEBUG=true -DNDEBUG=1")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -ggdb -DDEBUG=true -DNDEBUG=1")
SET(STANDALONE_BINLOG TRUE)
SET(MYSQL_SRC_DIR "${CMAKE_SOURCE_DIR}/../../../deps/mysql-8.0.28/")

LINK_DIRECTORIES("${MYSQL_SRC_DIR}/_build/client/base/")
LINK_DIRECTORIES("${MYSQL_SRC_DIR}_build/libbinlogevents/lib/")
LINK_DIRECTORIES("${MYSQL_SRC_DIR}_build/library_output_directory/")
LINK_DIRECTORIES("${MYSQL_SRC_DIR}_build/archive_output_directory/")
LINK_DIRECTORIES("${MYSQL_SRC_DIR}_build/sql/")
LINK_DIRECTORIES("${MYSQL_SRC_DIR}_build/components/library_mysys/")
INCLUDE_DIRECTORIES("${MYSQL_SRC_DIR}_build/include/")
INCLUDE_DIRECTORIES("${MYSQL_SRC_DIR}include/")
INCLUDE_DIRECTORIES("${MYSQL_SRC_DIR}libbinlogevents/include/")
INCLUDE_DIRECTORIES("${MYSQL_SRC_DIR}sql/")
INCLUDE_DIRECTORIES("${MYSQL_SRC_DIR}")

INCLUDE_DIRECTORIES("/usr/local/include/")
INCLUDE_DIRECTORIES(("../../../inc/"))
INCLUDE_DIRECTORIES(("../../../"))
INCLUDE_DIRECTORIES("/usr/local/share/include/")

## for openssl
INCLUDE_DIRECTORIES("/usr/local/opt/openssl@1.1/include/")
LINK_DIRECTORIES(/usr/local/opt/openssl@1.1/lib/)

ADD_LIBRARY(binlogevents_static STATIC IMPORTED)
SET_PROPERTY(TARGET binlogevents_static PROPERTY IMPORTED_LOCATION "${MYSQL_SRC_DIR}/_build/libbinlogevents/lib/libbinlogevents.a")
ADD_LIBRARY(json_binlog_static STATIC IMPORTED)
SET_PROPERTY(TARGET json_binlog_static PROPERTY IMPORTED_LOCATION "${MYSQL_SRC_DIR}/_build/archive_output_directory/libjson_binlog_static.a")
ADD_LIBRARY(mysqlclient_static STATIC IMPORTED)
SET_PROPERTY(TARGET mysqlclient_static PROPERTY IMPORTED_LOCATION "${MYSQL_SRC_DIR}/_build/archive_output_directory/libmysqlclient.a")
ADD_LIBRARY(mysys_static STATIC IMPORTED)
SET_PROPERTY(TARGET mysys_static PROPERTY IMPORTED_LOCATION "${MYSQL_SRC_DIR}/_build/archive_output_directory/libmysys.a")
ADD_LIBRARY(keyring_common_static STATIC IMPORTED)
SET_PROPERTY(TARGET keyring_common_static PROPERTY IMPORTED_LOCATION "${MYSQL_SRC_DIR}/_build/archive_output_directory/libkeyring_common.a")


#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O2")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O0 -g")

SET(BINLOG_FILES
    ${MYSQL_SRC_DIR}/sql/log_event.cc
    ${MYSQL_SRC_DIR}/sql/rpl_utility.cc
    ${MYSQL_SRC_DIR}/sql/rpl_gtid_sid_map.cc
    ${MYSQL_SRC_DIR}/sql/rpl_gtid_misc.cc
    ${MYSQL_SRC_DIR}/sql/rpl_gtid_set.cc
    ${MYSQL_SRC_DIR}/sql/rpl_gtid_specification.cc
    ${MYSQL_SRC_DIR}/sql/rpl_tblmap.cc
    ${MYSQL_SRC_DIR}/sql/basic_istream.cc
    ${MYSQL_SRC_DIR}/sql/binlog_istream.cc
    ${MYSQL_SRC_DIR}/sql/binlog_reader.cc
    ${MYSQL_SRC_DIR}/sql/stream_cipher.cc
    ${MYSQL_SRC_DIR}/sql/rpl_log_encryption.cc
    ${MYSQL_SRC_DIR}/libbinlogevents/src/trx_boundary_parser.cpp
)
SET(SOURCE_FILES
    ${BINLOG_FILES}
    ${CMAKE_SOURCE_DIR}/../../main.cc
)
ADD_EXECUTABLE(${PRJ} ${SOURCE_FILES})

TARGET_LINK_LIBRARIES(${PRJ} pthread)
TARGET_LINK_LIBRARIES(${PRJ} mysqlclient_static)
TARGET_LINK_LIBRARIES(${PRJ} binlogevents_static)
TARGET_LINK_LIBRARIES(${PRJ} json_binlog_static)
TARGET_LINK_LIBRARIES(${PRJ} mysys_static)
TARGET_LINK_LIBRARIES(${PRJ} keyring_common_static)
TARGET_LINK_LIBRARIES(${PRJ} client_base)
TARGET_LINK_LIBRARIES(${PRJ} ssl)
TARGET_LINK_LIBRARIES(${PRJ} crypto)

TARGET_LINK_LIBRARIES(${PRJ} z)
TARGET_LINK_LIBRARIES(${PRJ} dl)


IF (UNIX AND NOT APPLE)
    TARGET_LINK_LIBRARIES(${PRJ} rt)
ENDIF()



