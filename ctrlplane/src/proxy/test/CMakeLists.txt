
SET(CMAKE_C_FLAGS_DEBUG "-Wall -g -O0 -D__TESTING__ -DDHOST=${DB_HOST} -DDUSER=${DB_USER} -DDPSWD=${DB_PSWD} -DDINST=${DB_INST}")
SET(CMAKE_C_FLAGS "-Wall -g -O0 -D__TESTING__ -DDHOST=${DB_HOST} -DDUSER=${DB_USER} -DDPSWD=${DB_PSWD} -DDINST=${DB_INST}")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O0 -g -D__TESTING__ -DDHOST=${DB_HOST} -DDUSER=${DB_USER} -DDPSWD=${DB_PSWD} -DDINST=${DB_INST}")

if (UNIX AND NOT APPLE)                                                          
    SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D__LINUX__")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__LINUX__")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__LINUX__")
endif()

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

INCLUDE_DIRECTORIES("../")
INCLUDE_DIRECTORIES("../../inc/")

### common source.
SET(SOURCE_FILES
        ../../cmn/panic.c
        ../../pkt/gtpc_validate_header.c
        ../../pkt/gtpc_header.c
        ../../pkt/gtpc_item_func.c
        ../../pkt/gtpc_item_func_v1.c
        ../../pkt/gtpu_validate_header.c
        ../../pkt/gtpu_header.c
        ../../pkt/gtp_alloc_packet.c
        ../../pkt/gtp_free_packet.c
        ../../pkt/gtp_memory_status.c

        ../../pgw/event/event_gtpc_v1_any.c
        ../../pgw/event/event_gtpc_create_session.c
        ../../pgw/event/event_gtpc_delete_bearer.c
        ../../pgw/event/event_gtpc_delete_session.c
        ../../pgw/event/event_gtpc_echo.c
        ../../pgw/event/event_gtpc_modify_bearer.c
        ../../pgw/event/event_gtpc_other.c
        ../../pgw/event/event_gtpc_resume_notification.c
        ../../pgw/event/event_gtpc_suspend_notification.c

        ../../db/mysql_bind_init.c
        ../../db/mysql_execute.c
        ../../db/parse_sql.c

        ../../cmn/sgw_peers.c
        ../../cmn/send.c
        ../../cmn/deq_burst.c
        ../../cmn/enq.c
        ../../cmn/panic.c
        ../../cmn/free_packet.c
        ../../cmn/alloc_packet.c
        )
### project source.
SET(SOURCE_FILES
        ${SOURCE_FILES}
        ../config.cc
        ../database_config.cc
        ../general_config.cc
        ../database_connection.cc
        ../routes.cc
        ../route.cc
        ../proxy_utils.cc
        ../proxy_event.cc
        ../proxy_stats.cc
    )

#### basicaly test
SET(BASIC_SOURCE_FILES
  ${SOURCE_FILES}
  ./basic_test.cc
  )

ADD_EXECUTABLE(t_proxy ${BASIC_SOURCE_FILES})
TARGET_LINK_LIBRARIES(t_proxy pthread)
TARGET_LINK_LIBRARIES(t_proxy mysqlclient)
TARGET_LINK_LIBRARIES(t_proxy event)
TARGET_LINK_LIBRARIES(t_proxy event_pthreads)
TARGET_LINK_LIBRARIES(t_proxy gtest)
TARGET_LINK_LIBRARIES(t_proxy gtest_main)
if (UNIX AND NOT APPLE)
    TARGET_LINK_LIBRARIES(t_proxy rt)
else()
    TARGET_LINK_LIBRARIES(t_proxy ncurses)
endif()

#### ingress test
SET(INGRESS_SOURCE_FILES
        ${SOURCE_FILES}
        ./ingress_test.cc
        )

ADD_EXECUTABLE(t_ingress ${INGRESS_SOURCE_FILES})
TARGET_LINK_LIBRARIES(t_ingress pthread)
TARGET_LINK_LIBRARIES(t_ingress mysqlclient)
TARGET_LINK_LIBRARIES(t_ingress event)
TARGET_LINK_LIBRARIES(t_ingress event_pthreads)
TARGET_LINK_LIBRARIES(t_ingress gtest)
TARGET_LINK_LIBRARIES(t_ingress gtest_main)
# TARGET_LINK_LIBRARIES(t_ingress mixi_pgw_ctrl_planeutl)
if (UNIX AND NOT APPLE)
    TARGET_LINK_LIBRARIES(t_ingress rt)
else()
    TARGET_LINK_LIBRARIES(t_ingress ncurses)
endif()

#### egress test
SET(EGRESS_SOURCE_FILES
        ${SOURCE_FILES}
        ./egress_test.cc
        )

ADD_EXECUTABLE(t_egress ${EGRESS_SOURCE_FILES})
TARGET_LINK_LIBRARIES(t_egress pthread)
TARGET_LINK_LIBRARIES(t_egress mysqlclient)
TARGET_LINK_LIBRARIES(t_egress event)
TARGET_LINK_LIBRARIES(t_egress event_pthreads)
TARGET_LINK_LIBRARIES(t_egress gtest)
TARGET_LINK_LIBRARIES(t_egress gtest_main)
# TARGET_LINK_LIBRARIES(t_egress mixi_pgw_ctrl_planeutl)
if (UNIX AND NOT APPLE)
    TARGET_LINK_LIBRARIES(t_egress rt)
else()
    TARGET_LINK_LIBRARIES(t_egress ncurses)
endif()


