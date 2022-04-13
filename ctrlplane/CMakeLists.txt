cmake_minimum_required(VERSION 3.1)
SET(PRJ mixi-pgw-cplane)
PROJECT(${PRJ})
ADD_DEFINITIONS(-DVERSION="1.00")

SET(MYSQL_SRC_DIR "${CMAKE_SOURCE_DIR}/../deps/mysql-8.0.28/")
LINK_DIRECTORIES("../deps/mysql-8.0.28/_build/client/base/")
LINK_DIRECTORIES("../deps/mysql-8.0.28/_build/libbinlogevents/lib/")
LINK_DIRECTORIES("../deps/mysql-8.0.28/_build/library_output_directory/")
LINK_DIRECTORIES("../deps/mysql-8.0.28/_build/archive_output_directory/")
LINK_DIRECTORIES("../deps/mysql-8.0.28/_build/sql/")
LINK_DIRECTORIES("../deps/mysql-8.0.28/_build/components/library_mysys/")
INCLUDE_DIRECTORIES("../deps/mysql-8.0.28/_build/include/")
INCLUDE_DIRECTORIES("../deps/mysql-8.0.28/include/")
INCLUDE_DIRECTORIES("../deps/mysql-8.0.28/libbinlogevents/include/")
INCLUDE_DIRECTORIES("../deps/mysql-8.0.28/sql/")
INCLUDE_DIRECTORIES("../deps/mysql-8.0.28/")



LINK_DIRECTORIES("/usr/local/lib/")
INCLUDE_DIRECTORIES("/usr/local/include/")
INCLUDE_DIRECTORIES("./inc/")


FILE(GLOB PGW_SOURCES ./src/pgw/*.c)
FILE(GLOB PGW_EVT_SOURCES ./src/pgw/event/*.c)
FILE(GLOB CMN_SOURCES ./src/cmn/*.c)
FILE(GLOB NODE_SOURCES ./src/node/*.c)
FILE(GLOB DB_SOURCES ./src/db/*.c)
FILE(GLOB PKT_SOURCES ./src/pkt/*.c)

LIST(REMOVE_ITEM DB_SOURCES "${CMAKE_SOURCE_DIR}/./src/db/driver_sqlite3.c")
LIST(REMOVE_ITEM CMN_SOURCES "${CMAKE_SOURCE_DIR}/./src/cmn/instances.c")


ADD_EXECUTABLE(
    ${PRJ}
    ${PGW_SOURCES}
    ${PGW_EVT_SOURCES}
    ${CMN_SOURCES}
    ${NODE_SOURCES}
    ${DB_SOURCES}
    ${PKT_SOURCES}
)
TARGET_LINK_LIBRARIES(${PRJ} z)
TARGET_LINK_LIBRARIES(${PRJ} dl)
TARGET_LINK_LIBRARIES(${PRJ} rt)
TARGET_LINK_LIBRARIES(${PRJ} pthread)
TARGET_LINK_LIBRARIES(${PRJ} event)
TARGET_LINK_LIBRARIES(${PRJ} event_pthreads)
TARGET_LINK_LIBRARIES(${PRJ} mysqlclient)
TARGET_LINK_LIBRARIES(${PRJ} ssl)
TARGET_LINK_LIBRARIES(${PRJ} crypto)

