CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
SET(PRJ mixi-pgw-radius)
PROJECT(${PRJ})

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -o0 -DDEBUG=true -DNDEBUG=1")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -ggdb -DDEBUG=true -DNDEBUG=1")



SET(BOOST_ROOT "/usr/local/share/")
FIND_PACKAGE(Boost COMPONENTS thread regex program_options system REQUIRED)
FIND_LIBRARY(LOG4CPP_LIBRARY_PATH NAMES log4cpp PATHS "/usr/local/lib/")

INCLUDE_DIRECTORIES("/usr/local/include/")
INCLUDE_DIRECTORIES(("../../../inc/"))
INCLUDE_DIRECTORIES(("../../../"))
INCLUDE_DIRECTORIES("/usr/local/share/include/")

## for openssl
INCLUDE_DIRECTORIES("/usr/local/opt/openssl/include/")
LINK_DIRECTORIES(/usr/local/opt/openssl/lib/)

## for event
if (UNIX AND NOT APPLE)
    INCLUDE_DIRECTORIES("/usr/include/")
endif()


ADD_LIBRARY(binlog_static STATIC IMPORTED)
SET_PROPERTY(TARGET binlog_static PROPERTY IMPORTED_LOCATION "/usr/local/lib/libbinlog.a")

#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O2")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O0 -g")

SET(SOURCE_FILES
    ${BINLOG_FILES}
    mixipgw_mod_radius.cc
    func/mixipgw_radius_client.cc
    func/mixipgw_radius_server.cc
    func/mixipgw_radius_server_on_recv.cc
    func/mixipgw_radius_server_on_timeout.cc
    func/mixipgw_radius_server_binlog_loop.cc
    func/mixipgw_radius_req_access.cc
    func/mixipgw_radius_req_account.cc
)
ADD_EXECUTABLE(${PRJ} ${SOURCE_FILES})
TARGET_LINK_LIBRARIES(${PRJ} pthread)
TARGET_LINK_LIBRARIES(${PRJ} ${Boost_LIBRARIES})
TARGET_LINK_LIBRARIES(${PRJ} ${LOG4CPP_LIBRARY_PATH})
TARGET_LINK_LIBRARIES(${PRJ} mixi-pgw-misc)
TARGET_LINK_LIBRARIES(${PRJ} mixi-pgw-db)
TARGET_LINK_LIBRARIES(${PRJ} mixi-pgw-server)
TARGET_LINK_LIBRARIES(${PRJ} binlog_static)
TARGET_LINK_LIBRARIES(${PRJ} event)
TARGET_LINK_LIBRARIES(${PRJ} event_pthreads)
TARGET_LINK_LIBRARIES(${PRJ} ssl)
TARGET_LINK_LIBRARIES(${PRJ} crypto)


TARGET_COMPILE_DEFINITIONS(${PRJ} PRIVATE DISABLE_PSI_MUTEX)
TARGET_INCLUDE_DIRECTORIES(${PRJ} PRIVATE ${MYSQL_SRC_DIR}/sql)

if (UNIX AND NOT APPLE)
    target_link_libraries(${PRJ} z)
    target_link_libraries(${PRJ} rt)
    target_link_libraries(${PRJ} dl)
else()
    target_link_libraries(${PRJ} ncurses)
endif()



