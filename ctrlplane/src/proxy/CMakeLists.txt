cmake_minimum_required(VERSION 3.1)

add_definitions(-DVERSION="1.00")
SET(PRJ mixi_pgw_ctrl_plane_proxy)
PROJECT(${PRJ})


SET(CMAKE_C_FLAGS_DEBUG "-Wall -D__DEBUG_PACKET__")
SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")


SET(CMAKE_C_FLAGS "-Wall -D__DEBUG_PACKET__")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")


SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -D__DEBUG_PACKET__")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")


if (UNIX AND NOT APPLE)
    SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D__LINUX__")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__LINUX__")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__LINUX__")
endif()

SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DSINGLE_CREATE_SESSION")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DSINGLE_CREATE_SESSION")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSINGLE_CREATE_SESSION")

IF (NOT DEBUG)
    SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O2 ")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 ")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 ")
ELSE()
    SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -g ")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g ")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g ")
ENDIF()


LINK_DIRECTORIES("../lib/")
INCLUDE_DIRECTORIES("../inc/")
INCLUDE_DIRECTORIES("../../inc/")
INCLUDE_DIRECTORIES("../proxy/")
INCLUDE_DIRECTORIES("/usr/local/include/")

SET(MYSQL_SRC_DIR "${CMAKE_SOURCE_DIR}/../../../deps/mysql-8.0.28/")
LINK_DIRECTORIES("../../../deps/mysql-8.0.28/_build/client/base/")
LINK_DIRECTORIES("../../../deps/mysql-8.0.28/_build/libbinlogevents/lib/")
LINK_DIRECTORIES("../../../deps/mysql-8.0.28/_build/library_output_directory/")
LINK_DIRECTORIES("../../../deps/mysql-8.0.28/_build/archive_output_directory/")
LINK_DIRECTORIES("../../../deps/mysql-8.0.28/_build/sql/")
LINK_DIRECTORIES("../../../deps/mysql-8.0.28/_build/components/library_mysys/")
INCLUDE_DIRECTORIES("../../../deps/mysql-8.0.28/_build/include/")
INCLUDE_DIRECTORIES("../../../deps/mysql-8.0.28/include/")
INCLUDE_DIRECTORIES("../../../deps/mysql-8.0.28/libbinlogevents/include/")
INCLUDE_DIRECTORIES("../../../deps/mysql-8.0.28/sql/")
INCLUDE_DIRECTORIES("../../../deps/mysql-8.0.28/")

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

        ../pgw/event/event_gtpc_v1_any.c
        ../pgw/event/event_gtpc_create_session.c
        ../pgw/event/event_gtpc_delete_bearer.c
        ../pgw/event/event_gtpc_delete_session.c
        ../pgw/event/event_gtpc_echo.c
        ../pgw/event/event_gtpc_modify_bearer.c
        ../pgw/event/event_gtpc_other.c
        ../pgw/event/event_gtpc_resume_notification.c
        ../pgw/event/event_gtpc_suspend_notification.c
       
        ../db/mysql_bind_init.c
        ../db/mysql_execute.c
        ../db/parse_sql.c

        ../cmn/sgw_peers.c
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
        proxy_entry.cc
        proxy_event.cc
        proxy_utils.cc
        proxy_stats.cc
        config.cc
        database_config.cc
        general_config.cc
        routes.cc
        route.cc
        database_connection.cc
        )
ADD_EXECUTABLE(${PRJ} ${SOURCE_FILES})
TARGET_LINK_LIBRARIES(${PRJ} pthread)
TARGET_LINK_LIBRARIES(${PRJ} mysqlclient)
TARGET_LINK_LIBRARIES(${PRJ} event)
TARGET_LINK_LIBRARIES(${PRJ} event_pthreads)

ADD_SUBDIRECTORY(test)
ENABLE_TESTING()
ADD_TEST(NAME proxy COMMAND t_proxy)
ADD_TEST(NAME proxy_ingress COMMAND t_ingress)
ADD_TEST(NAME proxy_egress COMMAND t_egress)


if (UNIX AND NOT APPLE)
    target_link_libraries(${PRJ} rt)
else()
    target_link_libraries(${PRJ} ncurses)
endif()
