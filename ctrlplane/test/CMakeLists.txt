OPTION(DB_HOST "Config DB_HOST" OFF)
OPTION(DB_USER "Config DB_USER" OFF)
OPTION(DB_PSWD "Config DB_PSWD" OFF)
OPTION(DB_INST "Config DB_INST" OFF)
IF(NOT DB_HOST)
    SET(DB_HOST "127.0.0.1")
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




SET(CMAKE_C_FLAGS_DEBUG "-Wall -g -O0 -D__TESTING__ -DDHOST=${DB_HOST} -DDUSER=${DB_USER} -DDPSWD=${DB_PSWD} -DDINST=${DB_INST}")
SET(CMAKE_C_FLAGS "-Wall -g -O0 -D__TESTING__ -DDHOST=${DB_HOST} -DDUSER=${DB_USER} -DDPSWD=${DB_PSWD} -DDINST=${DB_INST}")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O0 -g -D__TESTING__ -DDHOST=${DB_HOST} -DDUSER=${DB_USER} -DDPSWD=${DB_PSWD} -DDINST=${DB_INST}")

if (UNIX AND NOT APPLE)                                                          
    SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D__LINUX__")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__LINUX__")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__LINUX__")
endif()

IF (IPV64)
    MESSAGE(STATUS "IPV64=ON")
    SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D__IPV64__")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__IPV64__")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__IPV64__")
ELSE()
    MESSAGE(STATUS "IPV64=OFF")
ENDIF()
IF (SQLITE3)
    MESSAGE(STATUS "SQLITE3=ON")
    SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D__USE_SQLITE3__")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__USE_SQLITE3__")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__USE_SQLITE3__")
ELSE()
    MESSAGE(STATUS "SQLITE3=OFF")
ENDIF()


set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

INCLUDE_DIRECTORIES("../inc/")
### c source.
SET(SOURCE_FILES
        ../src/cmn/create_instance.c
        ../src/cmn/release_instance.c
        ../src/cmn/get_property.c
        ../src/cmn/set_property.c
        ../src/cmn/start.c
        ../src/cmn/stop.c
        ../src/cmn/send.c
        ../src/cmn/deq_burst.c
        ../src/cmn/enq.c
        ../src/cmn/panic.c
        ../src/cmn/free_packet.c
        ../src/cmn/alloc_packet.c
        ../src/cmn/sgw_peers.c
        ../src/db/mysql_execute.c
        ../src/db/mysql_bind_init.c
        ../src/db/parse_sql.c
        ../src/pkt/gtpc_validate_header.c
        ../src/pkt/gtpc_header.c
        ../src/pkt/gtpc_item_func.c
        ../src/pkt/gtpc_item_func_v1.c
        ../src/pkt/gtpu_validate_header.c
        ../src/pkt/gtpu_header.c
        ../src/pkt/gtp_alloc_packet.c
        ../src/pkt/gtp_free_packet.c
        ../src/pkt/gtp_memory_status.c
        ../src/node/start_parallel.c
        ../src/node/thread.c
    )

IF (false)
    MESSAGE(STATUS "Use Mariadb on Tested")
    SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")
ELSE()
    MESSAGE(STATUS "Use SQLITE3 on Tested")
    SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D__USESQLITE3_ON_TEST__")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__USESQLITE3_ON_TEST__")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__USESQLITE3_ON_TEST__")
    SET(SQLITE3 ON)
    SET(SOURCE_FILES
            ${SOURCE_FILES}
            ../src/db/driver_sqlite3.c
            )
ENDIF()



SET(GTPCPARSE_SOURCE_FILES
  ${SOURCE_FILES}
  ./gtpc_parse_test.cc
  )


SET(BASIC_SOURCE_FILES
  ${SOURCE_FILES}
  ./basic_test.cc
  )
SET(EVENT_SOURCE_FILES
  ./event_test.cc
  ${SOURCE_FILES}
  ../src/pgw/event/event_rx.c
  ../src/pgw/event/event_tx.c
  ../src/pgw/event/event_timer.c
  ../src/pgw/event/event_gtpc_echo.c
  ../src/pgw/event/event_gtpu_echo.c
  ../src/pgw/event/event_node_init.c
  ../src/pgw/event/event_node_uninit.c
  ../src/pgw/event/event_gtpc_create_session.c
  ../src/pgw/event/event_gtpc_modify_bearer.c
  ../src/pgw/event/event_gtpc_delete_bearer.c
  ../src/pgw/event/event_gtpc_delete_session.c
  ../src/pgw/event/event_gtpc_suspend_notification.c
  ../src/pgw/event/event_gtpc_resume_notification.c
  ../src/pgw/event/event_gtpc_other.c
  ../src/pgw/event/event_gtpu_other.c
  ../src/pgw/event/event_gtpc_v1_any.c
  )

SET(FIX_ANY_TEST_SOURCE_FILES
        ./fix_any_test.cc
        ${SOURCE_FILES}
        ../src/pgw/event/event_rx.c
        ../src/pgw/event/event_tx.c
        ../src/pgw/event/event_timer.c
        ../src/pgw/event/event_gtpc_echo.c
        ../src/pgw/event/event_gtpu_echo.c
        ../src/pgw/event/event_node_init.c
        ../src/pgw/event/event_node_uninit.c
        ../src/pgw/event/event_gtpc_create_session.c
        ../src/pgw/event/event_gtpc_modify_bearer.c
        ../src/pgw/event/event_gtpc_delete_bearer.c
        ../src/pgw/event/event_gtpc_delete_session.c
        ../src/pgw/event/event_gtpc_suspend_notification.c
        ../src/pgw/event/event_gtpc_resume_notification.c
        ../src/pgw/event/event_gtpc_other.c
        ../src/pgw/event/event_gtpu_other.c
        ../src/pgw/event/event_gtpc_v1_any.c
        )
SET(GTPC_V1_TEST_SOURCE_FILES
        gtpc_v1_test.cc
        ${SOURCE_FILES}
        ../src/pgw/event/event_rx.c
        ../src/pgw/event/event_tx.c
        ../src/pgw/event/event_timer.c
        ../src/pgw/event/event_gtpc_echo.c
        ../src/pgw/event/event_gtpu_echo.c
        ../src/pgw/event/event_node_init.c
        ../src/pgw/event/event_node_uninit.c
        ../src/pgw/event/event_gtpc_create_session.c
        ../src/pgw/event/event_gtpc_modify_bearer.c
        ../src/pgw/event/event_gtpc_delete_bearer.c
        ../src/pgw/event/event_gtpc_delete_session.c
        ../src/pgw/event/event_gtpc_suspend_notification.c
        ../src/pgw/event/event_gtpc_resume_notification.c
        ../src/pgw/event/event_gtpc_other.c
        ../src/pgw/event/event_gtpu_other.c
        ../src/pgw/event/event_gtpc_v1_any.c
        )


ADD_EXECUTABLE(t_basic ${BASIC_SOURCE_FILES})
TARGET_LINK_LIBRARIES(t_basic pthread)
TARGET_LINK_LIBRARIES(t_basic mysqlclient)
TARGET_LINK_LIBRARIES(t_basic event)
TARGET_LINK_LIBRARIES(t_basic event_pthreads)
TARGET_LINK_LIBRARIES(t_basic gtest)
TARGET_LINK_LIBRARIES(t_basic gtest_main)
IF (SQLITE3)
    TARGET_LINK_LIBRARIES(t_basic sqlite3)
ENDIF()
if (UNIX AND NOT APPLE)
    target_link_libraries(t_basic rt)
else()
    target_link_libraries(t_basic ncurses)
endif()


ADD_EXECUTABLE(t_gtpcparse ${GTPCPARSE_SOURCE_FILES})
TARGET_LINK_LIBRARIES(t_gtpcparse pthread)
TARGET_LINK_LIBRARIES(t_gtpcparse mysqlclient)
TARGET_LINK_LIBRARIES(t_gtpcparse event)
TARGET_LINK_LIBRARIES(t_gtpcparse event_pthreads)
TARGET_LINK_LIBRARIES(t_gtpcparse gtest)
TARGET_LINK_LIBRARIES(t_gtpcparse gtest_main)
IF (SQLITE3)
    TARGET_LINK_LIBRARIES(t_gtpcparse sqlite3)
ENDIF()
if (UNIX AND NOT APPLE)
    target_link_libraries(t_gtpcparse rt)
else()
    target_link_libraries(t_gtpcparse ncurses)
endif()


ADD_EXECUTABLE(t_event ${EVENT_SOURCE_FILES})
TARGET_LINK_LIBRARIES(t_event pthread)
TARGET_LINK_LIBRARIES(t_event mysqlclient)
TARGET_LINK_LIBRARIES(t_event event)
TARGET_LINK_LIBRARIES(t_event event_pthreads)
TARGET_LINK_LIBRARIES(t_event gtest)
TARGET_LINK_LIBRARIES(t_event gtest_main)
IF (SQLITE3)
    TARGET_LINK_LIBRARIES(t_event sqlite3)
ENDIF()
if (UNIX AND NOT APPLE)
    target_link_libraries(t_event rt)
else()
    target_link_libraries(t_event ncurses)
endif()


ADD_EXECUTABLE(t_fix_any ${FIX_ANY_TEST_SOURCE_FILES})
TARGET_LINK_LIBRARIES(t_fix_any pthread)
TARGET_LINK_LIBRARIES(t_fix_any mysqlclient)
TARGET_LINK_LIBRARIES(t_fix_any event)
TARGET_LINK_LIBRARIES(t_fix_any event_pthreads)
TARGET_LINK_LIBRARIES(t_fix_any gtest)
TARGET_LINK_LIBRARIES(t_fix_any gtest_main)
IF (SQLITE3)
    TARGET_LINK_LIBRARIES(t_fix_any sqlite3)
ENDIF()
if (UNIX AND NOT APPLE)
    target_link_libraries(t_fix_any rt)
else()
    target_link_libraries(t_fix_any ncurses)
endif()


ADD_EXECUTABLE(t_gtpcv1 ${GTPC_V1_TEST_SOURCE_FILES})
TARGET_LINK_LIBRARIES(t_gtpcv1 pthread)
TARGET_LINK_LIBRARIES(t_gtpcv1 mysqlclient)
TARGET_LINK_LIBRARIES(t_gtpcv1 event)
TARGET_LINK_LIBRARIES(t_gtpcv1 event_pthreads)
TARGET_LINK_LIBRARIES(t_gtpcv1 gtest)
TARGET_LINK_LIBRARIES(t_gtpcv1 gtest_main)
IF (SQLITE3)
    TARGET_LINK_LIBRARIES(t_gtpcv1 sqlite3)
ENDIF()

if (UNIX AND NOT APPLE)
    target_link_libraries(t_gtpcv1 rt)
else()
    target_link_libraries(t_gtpcv1 ncurses)
endif()

