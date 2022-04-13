CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
SET(PRJ mixi-pgw-server)
PROJECT(${PRJ})

FIND_PACKAGE(Boost COMPONENTS thread regex program_options system REQUIRED)
FIND_LIBRARY(LOG4CPP_LIBRARY_PATH NAMES log4cpp PATHS "/usr/local/lib/")
INCLUDE_DIRECTORIES("/usr/local/include/")
INCLUDE_DIRECTORIES(("../../inc"))
if (UNIX AND NOT APPLE)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2")
else()
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O0 -g ")
endif()

## databases
SET(SOURCE_FILES
        ${SOURCE_FILES}
        conn.cc
        server.cc
        http_client.cc
        bfd.cc
        )

ADD_LIBRARY(${PRJ} SHARED ${SOURCE_FILES})
TARGET_LINK_LIBRARIES(${PRJ} pthread)
TARGET_LINK_LIBRARIES(${PRJ} ${Boost_LIBRARIES})
TARGET_LINK_LIBRARIES(${PRJ} ${LOG4CPP_LIBRARY_PATH})
TARGET_LINK_LIBRARIES(${PRJ} mixi-pgw-misc)
if (UNIX AND NOT APPLE)
    TARGET_LINK_LIBRARIES(${PRJ} rt)
endif()

INSTALL(TARGETS ${PRJ} LIBRARY DESTINATION lib)

