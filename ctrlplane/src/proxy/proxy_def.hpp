/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane_proxy
    @file       proxy_def.hpp
    @brief      mixi_pgw_ctrl_plane_proxy common include header
*******************************************************************************
*******************************************************************************
    @date       created(09/apr/2018)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 NetworkTeam. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 09/apr/2018 
      -# Initial Version
******************************************************************************/


#ifndef XPGW_PROXY_DEF_HPP
#define XPGW_PROXY_DEF_HPP

#include <map>
#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>

#include "pgw_ext.h"
#include "gtpc_ext.h"
#include "db_ext.h"
#include "mysql.h"

/*! @enum GROUP
  @brief
   group 
*/
enum GROUP{
    GROUP_DEVELOPER = 0,    /*!< developer team */
    GROUP_QA,               /*!< qa team */
    GROUP_COMPANY,          /*!< in company */
    GROUP_USER_MNO_00,      /*!< user(MNO_00) */
    GROUP_USER_MNO_01,      /*!< user(MNO_01) */
    GROUP_USER_MNO_02,      /*!< user(MNO_02) */
    GROUP_APL_PGW,          /*!< appliance pgw */
    GROUP_MAX
};

/*! @name mixi_pgw_ctrl_plane_proxy typedef */
/* @{ */
typedef std::string SSTR;                           /*!< string */
typedef std::unordered_map<SSTR, SSTR>  CFGMAP;     /*!< map config  */
typedef std::unordered_map<U64, GROUP>  IMSI_GROUP; /*!< map imsi/group */
typedef struct sockaddr_in SOCKADDRIN;              /*!< sockaddr_in typedef */
typedef struct in_addr     INADDR;                  /*!< in_addr typedef */
/* @} */

#ifndef MIN
#define MIN(a,b)    (a<b?a:b)
#endif

#define ASSERT(b,...)  {if (b){ void(0); }else{ pgw_panic(__VA_ARGS__); }}
#define TIMEOUTMS   (25)



/*! @enum PSTAT_COUNT
  @brief
  PSTAT_COUNT \n
*/
enum PSTAT_COUNT{
    PSTAT_COUNT_DC = 0,
    PSTAT_PID,
    PSTAT_UPTIME,
    PSTAT_TIME,
    PSTAT_VERSION,
    PSTAT_BYTES_REQUEST,
    PSTAT_BYTES_RESPONSE,
    PSTAT_COUNT_REQUEST,
    PSTAT_COUNT_RESPONSE,
    PSTAT_COUNT_DELAY,
    PSTAT_REJECTED_REQUEST,
    PSTAT_REJECTED_RESPONSE,
    PSTAT_COUNT_SESSION,
    PSTAT_COUNT_LIMIT = 127,

    PSTAT_COUNT_MAX
};

#define PSTAT_RTYPE_MASK(a)     (a&0xFF000000)
#define PSTAT_RTYPE_GROUPID     (1<<31)
#define PSTAT_RTYPE_IP          (1<<30)


#ifndef ATTRIBUTE_PACKED
#ifdef __clang__
#define ATTRIBUTE_PACKED
#else
#define ATTRIBUTE_PACKED        __attribute__ ((packed))
#endif
#endif

#define PSTAT_MAGIC     (0xdeadbeaf)
#define PSTAT_REQUEST   (0x80)
#define PSTAT_RESULT    (0x00)
#define PSTAT_ALL       (0x00)
#define PSTAT_GROUP     (0x01)
#define PSTAT_IP        (0x02)
#define PSTAT_KEYLEN    (64)
#define PSTAT_DVERSION  (1000000)


/*
  +---------------+---------------+---------------+---------------+
  |      0        |       1       |       2       |       3       |
  +---------------+---------------+---------------+---------------+
  |0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7|
  +---------------+---------------+---------------+---------------+
0 |                      magic(0xdeadbeaf)                        |
  +---------------+---------------+---------------+---------------+
4 | type          |      length                   |   extra       |
  +---------------+---------------+---------------+---------------+
8 |                         callback data                         |
  +---------------+---------------+---------------+---------------+
12|                         time                                  |
  +---------------+---------------+---------------+---------------+
Total 16 bytes
*/
/** *****************************************************
 * @brief
 * proxy status general header\n
 * \n
 */
typedef struct proxy_stats_header{
    U32    magic;           /*!< magic number */
    U8     type;            /*!< request type */
    U16    length;          /*!< total length(include header) */
    U8     extlength;       /*!< extra length (dword) */
    U32    callbackdata;    /*!< callback data */
    U32    time;            /*!< current time */
}ATTRIBUTE_PACKED proxy_stats_header_t,*proxy_stats_header_ptr;

/** *****************************************************
 * @brief
 * proxy status request data packet\n
 * \n
 */
typedef struct proxy_stats_request_data{
    union{
        char key[PSTAT_KEYLEN];
        struct {
            U32  type;
            U32  groupid;
            U32  ip;
        }param;
    };
}ATTRIBUTE_PACKED proxy_stats_request_data_t,*proxy_stats_request_data_ptr;

/** *****************************************************
 * @brief
 * proxy status result data packet\n
 * \n
 */
typedef struct proxy_stats_result_counter{
    U64    val[PSTAT_COUNT_MAX];     /*!< count[enum] */
}ATTRIBUTE_PACKED proxy_stats_result_counter_t,*proxy_stats_result_counter_ptr;



#endif //XPGW_PROXY_DEF_HPP
