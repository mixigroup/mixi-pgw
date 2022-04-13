CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
SET(PRJ mixi-pgw-misc)
PROJECT(${PRJ})

FIND_LIBRARY(LOG4CPP_LIBRARY_PATH NAMES log4cpp PATHS "/usr/local/lib/")
INCLUDE_DIRECTORIES("../../deps/netmap/sys")
INCLUDE_DIRECTORIES("/usr/local/include/")
INCLUDE_DIRECTORIES(("../../inc"))
INCLUDE_DIRECTORIES(("../.."))
if (UNIX AND NOT APPLE)
  if (__DEBUG__)
    MESSAGE("debug mode. on")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O0 -g ")
  else()
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2 ")
  endif()

else()
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O0 -g ")
endif()

## databases
SET(SOURCE_FILES
        filter/bfd.cc
        filter/arp.cc
        filter/icmp.cc
        filter/xepc_ctrl.cc
        filter/gtpu_counter_egress.cc
        filter/gtpu_counter_ingress.cc
        filter/gtpu_echo.cc
        filter/gtpu_decap.cc
        filter/gtpu_encap.cc
        filter/policy.cc
        arch/netmap.cc
        arch/osxsim.cc
        misc.cc
        module.cc
        logger.cc
        process.cc
        pktheader.cc
        buffer.cc
        filter_container.cc
        arch_interface.cc

        pkt/gtpc.cc
        pkt/v2_ambr.cc
        pkt/v2_any.cc
        pkt/v2_apn.cc
        pkt/v2_apnrestriction.cc
        pkt/v2_bearercontext.cc
        pkt/v2_bqos.cc
        pkt/v2_cause.cc
        pkt/v2_chargingid.cc
        pkt/v2_ebi.cc
        pkt/v2_fteid.cc
        pkt/v2_gtpcpkt.hh
        pkt/v2_imsi.cc
        pkt/v2_indication.cc
        pkt/v2_mei.cc
        pkt/v2_msisdn.cc
        pkt/v2_paa.cc
        pkt/v2_pco.cc
        pkt/v2_pdn.cc
        pkt/v2_rat.cc
        pkt/v2_recovery.cc
        pkt/v2_selectionmode.cc
        pkt/v2_servingnetwork.cc
        pkt/v2_uli.cc

        )

ADD_LIBRARY(${PRJ} SHARED ${SOURCE_FILES})
TARGET_LINK_LIBRARIES(${PRJ} pthread)
TARGET_LINK_LIBRARIES(${PRJ} ${LOG4CPP_LIBRARY_PATH})
if (UNIX AND NOT APPLE)
    TARGET_LINK_LIBRARIES(${PRJ} rt)
endif()

INSTALL(TARGETS ${PRJ} LIBRARY DESTINATION lib)

