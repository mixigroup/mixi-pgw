/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       pgw_ext.h
    @brief      mixi_pgw_ctrl_plane c function define, common header
*******************************************************************************
*******************************************************************************
    @date       created(07/nov/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 07/nov/2017 
      -# Initial Version
******************************************************************************/
#ifndef MIXI_PGW_PGW_MODULE_H
#define MIXI_PGW_PGW_MODULE_H

#include "pgw_ext.h"
#include "node_ext.h"
#include "gtpc_ext.h"
#include "gtpu_ext.h"
#include "db_ext.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)


#ifndef DHOST
#define HOST                    ("192.168.56.20")
#else
#define HOST                    STR(DHOST)
#endif
#ifndef DSRCADDRESS
#define SRCADDRESS              ("10.192.211.64")
#else
#define SRCADDRESS              STR(DSRCADDRESS)
#endif
#ifndef DSRCNIC
#define SRCNIC                  ("en0")
#else
#define SRCNIC                  STR(DSRCNIC)
#endif
#ifndef DSERVERID
#define SERVERID                ("a00000")
#else
#define SERVERID                STR(DSERVERID)
#endif
#ifndef DUSER
#define USER                    ("mixipgw")
#else
#define USER                    STR(DUSER)
#endif
#ifndef DPSWD
#define PSWD                    ("password")
#else
#define PSWD                    STR(DPSWD)
#endif
#ifndef DINST
#define INST                    ("mixipgw")
#else
#define INST                    STR(DINST)
#endif

#define GTP_ECHO_SEQNUM_MIN     (0x0001)
#define GTP_ECHO_SEQNUM_MAX     (0x7fff)

enum {
    PGW_NODE_GTPU_ECHO_REQ = 0,
    PGW_NODE_GTPC_ECHO_REQ,
    PGW_NODE_TIMER,
    PGW_NODE_GTPC_CREATE_SESSION_REQ,
#ifndef SINGLE_CREATE_SESSION
    PGW_NODE_GTPC_CREATE_SESSION_REQ_EXT0,
#endif
    PGW_NODE_GTPC_CREATE_SESSION_REQ_MAX,
    PGW_NODE_GTPC_MODIFY_BEARER_REQ = PGW_NODE_GTPC_CREATE_SESSION_REQ_MAX,
    PGW_NODE_GTPC_OTHER_REQ,
    PGW_NODE_GTPU_OTHER_REQ,
    PGW_NODE_TX,
    PGW_NODE_COUNT
};

#define HAVE_S_OPTION       (1<<31)
#define HAVE_B_OPTION       (1<<30)
#define HAVE_I_OPTION       (1<<29)
#define HAVE_L_OPTION       (1<<28)
#define HAVE_D_OPTION       (1<<27)
#define HAVE_P_OPTION       (1<<26)
#define HAVE_X_OPTION       (1<<25)
#define HAVE_Y_OPTION       (1<<24)
#define HAVE_E_OPTION       (1<<23)

#define	LOCALPORT_BASE		(0)

#ifndef __USESQLITE3_ON_TEST__
#define KEEPALIVE_SQL               ("SELECT dst_ip,dst_port,src_ip,src_port,proto,stat, "\
                                    " active,server_id,server_type "\
                                    "FROM keepalive "\
                                    "WHERE updated_at < DATE_ADD(NOW(),INTERVAL -60 SECOND) AND active=1 AND stat >= 0 AND "\
                                    " src_ip='%s' AND server_id='%s' LIMIT 10")
#else
#define KEEPALIVE_SQL               ("SELECT dst_ip,dst_port,src_ip,src_port,proto,stat, "\
                                    " active,server_id,server_type "\
                                    "FROM keepalive "\
                                    "WHERE updated_at < STRFTIME('%%Y-%%m-%%d %%H:%%M:%%S',CURRENT_TIMESTAMP,'-60 seconds') AND active=1 AND stat >= 0 AND "\
                                    " src_ip='%s' AND server_id='%s' LIMIT 10")
#endif /* __USESQLITE3_ON_TEST__ */
//

#define INVALID_STATUS_SQL          ("UPDATE  keepalive SET stat = %d, updated_at = NOW() "\
                                    "WHERE src_ip='%s' AND src_port= %u AND server_id='%s' ")
#define CHANGE_STATUS_SQL           ("UPDATE  keepalive SET stat = %d, updated_at = NOW() "\
                                    "WHERE src_ip='%s' AND src_port= %u AND "\
                                    " dst_ip='%s' AND dst_port= %u AND server_id='%s' ")

#define CREATE_SESSION_IMSI_LOOKUP_SQL   ("SELECT `pgw_teid`, `ueipv4`, `bitrate_s5`, `bitrate_sgi` , "\
                                    " `pgw_gtpu_ipv`, `pgw_gtpc_ipv`, `dns`, `ebi`, `imsi`, `msisdn` , `sgw_gtpc_teid` "\
                                    " FROM tunnel "\
                                    " WHERE imsi = ? AND active IN(0,1) ")
#define CREATE_SESSION_MSISDN_LOOKUP_SQL   ("SELECT `pgw_teid`, `ueipv4`, `bitrate_s5`, `bitrate_sgi` , "\
                                    " `pgw_gtpu_ipv`, `pgw_gtpc_ipv`, `dns`, `ebi`, `imsi`, `msisdn` , `sgw_gtpc_teid` "\
                                    " FROM tunnel "\
                                    " WHERE msisdn = ? AND active IN(0,1) ")


#define CREATE_SESSION_UPD_SQL      ("UPDATE tunnel SET `sgw_gtpc_teid`=%u, `sgw_gtpc_ipv`='%s', `sgw_gtpu_teid`=%u, `sgw_gtpu_ipv`='%s', "\
                                    " `qci`=%u, `active` = 1, `ebi`=%u, `pgw_teid`=%u, `latest_gtp_version`=2 "\
                                    " WHERE imsi = %llu ")
#define CREATE_SESSION_UPD_R_SQL    ("UPDATE tunnel SET `sgw_gtpc_teid`=%u, `sgw_gtpc_ipv`='%s', `sgw_gtpu_teid`=%u, `sgw_gtpu_ipv`='%s', "\
                                    " `qci`=%u, `active` = 1, `ebi`=%u, `pgw_teid`=%u, `latest_gtp_version`=2, `restart_counter`=%u "\
                                    " WHERE imsi = %llu ")

#define MODIFY_BEARER_LOOKUP_SQL   ("SELECT `pgw_teid`, `ueipv4`, `bitrate_s5`, `bitrate_sgi` , "\
                                    " `pgw_gtpu_ipv`, `pgw_gtpc_ipv`, `dns`, `ebi`, `imsi`, `msisdn`, `sgw_gtpc_teid` "\
                                    " FROM tunnel "\
                                    " WHERE pgw_teid = ? AND active = 1 ")
#define MODIFY_BEARER_UPD_SQL      ("UPDATE tunnel SET `sgw_gtpc_teid`=%u, `sgw_gtpc_ipv`='%s', "\
                                    " `sgw_gtpu_teid`=%u, `sgw_gtpu_ipv`='%s', `pgw_teid`=%u, "\
                                    " `active` = 1, `latest_gtp_version`=2 "\
                                    " WHERE imsi = %llu ")
#define MODIFY_BEARER_UPD_R_SQL      ("UPDATE tunnel SET `sgw_gtpc_teid`=%u, `sgw_gtpc_ipv`='%s', "\
                                    " `sgw_gtpu_teid`=%u, `sgw_gtpu_ipv`='%s', `pgw_teid`=%u, "\
                                    " `active` = 1, `latest_gtp_version`=2, `restart_counter`=%u "\
                                    " WHERE imsi = %llu ")
#define MODIFY_BEARER_RAT_UPD_SQL  ("UPDATE tunnel SET `sgw_gtpc_teid`=%u, `sgw_gtpc_ipv`='%s', "\
                                    " `sgw_gtpu_teid`=%u, `sgw_gtpu_ipv`='%s', `pgw_teid`=%u, `rat`=%u,"\
                                    " `active` = 1, `latest_gtp_version`=2 "\
                                    " WHERE imsi = %llu ")
#define MODIFY_BEARER_RAT_UPD_R_SQL  ("UPDATE tunnel SET `sgw_gtpc_teid`=%u, `sgw_gtpc_ipv`='%s', "\
                                    " `sgw_gtpu_teid`=%u, `sgw_gtpu_ipv`='%s', `pgw_teid`=%u, `rat`=%u,"\
                                    " `active` = 1, `latest_gtp_version`=2, `restart_counter`=%u "\
                                    " WHERE imsi = %llu ")

#define MODIFY_BEARER_RATONLY_UPD_SQL  ("UPDATE tunnel SET `rat`=%u,`active` = 1, `latest_gtp_version`=2 WHERE imsi = %llu ")
#define MODIFY_BEARER_RATONLY_UPD_R_SQL  ("UPDATE tunnel SET `rat`=%u,`active` = 1, `latest_gtp_version`=2, `restart_counter`=%u  WHERE imsi = %llu ")

#define MODIFY_BEARER_EBI_UPD_SQL   ("UPDATE tunnel SET `ebi`=%u WHERE imsi = %llu ")


#define DELETE_SESSION_LOOKUP_SQL   ("SELECT `pgw_teid`, `ueipv4`, `bitrate_s5`, `bitrate_sgi` , "\
                                    " `pgw_gtpu_ipv`, `pgw_gtpc_ipv`, `dns`, `ebi`, `imsi`, `msisdn`, `sgw_gtpc_teid` "\
                                    " FROM tunnel "\
                                    " WHERE pgw_teid = ? ")
#define DELETE_SESSION_UPD_SQL      ("UPDATE tunnel SET `active` = 0 WHERE pgw_teid = %u AND ebi = %u ")
#define ERROR_IND_UPD_SQL           ("UPDATE tunnel SET `active` = 0 WHERE sgw_gtpu_teid = %u AND sgw_gtpu_ipv = '%s' ")

#define SUSPEND_NOTIFY_UPD_SQL      ("UPDATE tunnel SET active=0, `latest_gtp_version`=2 WHERE pgw_teid = %u")
#define RESUME_NOTIFY_UPD_SQL       ("UPDATE tunnel SET active=1, `latest_gtp_version`=2 WHERE imsi = %llu")
#define ICMPV6_RA_LOOKUP_SQL        ("SELECT `pgw_teid`, `ueipv4`, `bitrate_s5`, `bitrate_sgi` , "\
                                    " `pgw_gtpu_ipv`, `pgw_gtpc_ipv`, `dns`, `ebi`, `imsi`, `msisdn` , `sgw_gtpu_teid` "\
                                    " FROM tunnel "\
                                    " WHERE pgw_teid = ? AND active IN(0,1) ")
#define CREATE_PDP_CONTEXT_UPD_SQL  ("UPDATE tunnel SET `sgw_gtpc_teid`=%u, `sgw_gtpc_ipv`='%s', `sgw_gtpu_teid`=%u, `sgw_gtpu_ipv`='%s', "\
                                    " `qci`=%u, `active` = 1, `ebi`=%u, `pgw_teid`=%u, `imsi`=%llu, `rat`=1 ,`latest_gtp_version`=1 "\
                                    " WHERE imsi = %llu ")
#define UPDATE_PDP_CONTEXT_UPD_SQL  ("UPDATE tunnel SET `sgw_gtpc_teid`=%u, `sgw_gtpc_ipv`='%s', `sgw_gtpu_teid`=%u, `sgw_gtpu_ipv`='%s', "\
                                    " `rat`=1, `active` = 1, `latest_gtp_version`=1 "\
                                    " WHERE imsi = %llu ")


#define CREATE_SESSION_IMSI_MSISDN_UPD_SQL  ("UPDATE tunnel SET msisdn=%llu WHERE imsi = %llu")
#define CREATE_SESSION_PGWIP_UPD_SQL    ("UPDATE tunnel SET pgw_gtpc_ipv='%s',pgw_gtpu_ipv='%s' WHERE imsi= %llu")

#define SGW_PEER_INSERT_SQL         ("INSERT INTO sgw_peer(ip,counter)VALUES(%u,%u) ON DUPLICATE KEY UPDATE `counter`=VALUES(`counter`),updated_at=NOW()")

#define ECHO_RESTART_UPD_SQL        ("UPDATE tunnel SET active=0, `latest_gtp_version`=2 WHERE sgw_gtpc_ipv = '%s' AND `restart_counter` < %u")

/**
  PGW event : Rx\n
 *******************************************************************************
 \n
 *******************************************************************************
 @parma[in]   pinst   application instance handle
 @param[in]   data    receive buffer
 @param[in]   datalen receive buffer length(=packet length)
 @param[in]   saddr   server address
 @param[in]   saddrlen address length
 @param[in]   caddr   client address
 @param[in]   caddrlen address length
 @param[in]   ext     extend data , event code
 @return int  0==OK,0!=error
 */
RETCD event_rx(handle_ptr pinst, const U8* data, const INT datalen,  struct sockaddr_in* saddr, ssize_t saddrlen, struct sockaddr_in* caddr, ssize_t caddrlen, void* ext);

/**
  event : TX core\n
 *******************************************************************************
 \n
 *******************************************************************************
 @parma[in]   pinst   application instance handle
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_tx(handle_ptr , void* );

/**
  event : init node\n
 *******************************************************************************
 initialize it every node\n
 *******************************************************************************
 @parma[in]   pinst   application instance handle
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_node_init(handle_ptr , void* );
/**
  event : cleanup node\n
 *******************************************************************************
 cleanup it every node\n
 *******************************************************************************
 @parma[in]   pinst   application instance handle
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_node_uninit(handle_ptr , void* );

/**
  event : GTPU Echo Request Core\n
 *******************************************************************************
 Rx -> gtpu echo [request/ response]\n
 - request:\n
 convert to response , send to tx via software ring\n
 - response:\n
 update status by dst[ip/port],src[ip/port]\n
 *******************************************************************************
 @parma[in]   pinst   application instance handle
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_gtpu_echo_req(handle_ptr , void* );

/**
  event : GTPC Echo Request Core\n
 *******************************************************************************
 Rx -> gtpc echo [request/ response]\n
 - request:\n
 convert to response , send to tx via software ring\n
 - response:\n
 update status on dst[ip/port],src[ip/port]\n
 *******************************************************************************
 @parma[in]   pinst   application instance handle
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_gtpc_echo_req(handle_ptr , void* );

/**
  event : timer Core\n
 *******************************************************************************
 do not modify node object in timer context\n
 \n
 - start life monitoring for GTP[u/c]-directed servers configured in database\n
 *******************************************************************************
 @parma[in]   pinst   application instance handle
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_timer(handle_ptr , void* );

/**
  event : GTPC Create Session Request\n
 *******************************************************************************
 Rx -> gtpc create session\n
 *******************************************************************************
 @parma[in]   pinst   application instance handle
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_gtpc_create_session_req(handle_ptr , void* );

/**
  event : GTPC Modify Bearer Request\n
 *******************************************************************************
 Rx -> gtpc modify bearer\n
 *******************************************************************************
 @parma[in]   pinst   application instance handle
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_gtpc_modify_bearer_req(handle_ptr , void* );


/**
  event : GTPC Other Request\n
 *******************************************************************************
 Rx -> gtpc delete bearer / delete session/ suspend notification/ resume notification \n
 *******************************************************************************
 @parma[in]   pinst   application instance handle
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_gtpc_other_req(handle_ptr , void* );


/**
  event : GTPU Other Request\n
 *******************************************************************************
 Rx -> internal / error indicator \n
 *******************************************************************************
 @parma[in]   pinst   application instance handle
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_gtpu_other_req(handle_ptr , void* );


/**
  event : GTPC Delete Bearer Request\n
 *******************************************************************************
 Rx -> gtpc delete bearer\n
 *******************************************************************************
 @parma[in]   pckt    packet 
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_gtpc_delete_bearer_req(packet_ptr , void* );

/**
  event : GTPC Delete Session Request\n
 *******************************************************************************
 Rx -> gtpc delete session\n
 *******************************************************************************
 @parma[in]   pckt    packet 
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_gtpc_delete_session_req(packet_ptr , void* );


/**
  event : GTPC Suspend Notification Request\n
 *******************************************************************************
 Rx -> gtpc Suspend Notification \n
 *******************************************************************************
 @parma[in]   pckt    packet 
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_gtpc_suspend_notification_req(packet_ptr , void* );

/**
  event : GTPC Resume Notification Request\n
 *******************************************************************************
 Rx -> gtpc Resume Notification \n
 *******************************************************************************
 @parma[in]   pckt    packet 
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_gtpc_resume_notification_req(packet_ptr , void* );

/**
  event : GTPC Version1 Any Request\n
 *******************************************************************************
 Rx -> gtpc version1 request \n
 *******************************************************************************
 @parma[in]   pckt    packet 
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_gtpc_v1_any_req(packet_ptr , void* );

/**
  SELECT callback about tunnel table \n
 *******************************************************************************
 duplicate records are not considered\n
 *******************************************************************************
 @parma[in]   counter   count of records
 @param[in]   clmncnt   count of columns
 @param[in]   col       columns
 @param[in]   rec       record
 @param[in]   arg       callback , user data
 @return int  0==OK,0!=error
 */
RETCD on_tunnel_record(U32 counter, U32 clmncnt, colmn_ptr col, record_ptr rec, void* arg);

/**
  Setup PCO response\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   pco   pco struct
 @param[in]   cs    parse status
 @return int  0==OK,0!=error
 */
RETCD setup_pco(gtpc_pco_ptr pco, struct gtpc_parse_state* cs);

/**
  Setup PCO response\n
 *******************************************************************************
 gtpc version 1\n
 *******************************************************************************
 @param[in]   pco   pco-v1 struct
 @param[in]   cs    parse status
 @return int  0==OK,0!=error
 */
RETCD setup_pco_v1(gtpc_pco_v1_ptr pco, struct gtpc_parse_state* cs);


/**
  event : timeout Keepalive Fire\n
 *******************************************************************************
 \n
 *******************************************************************************
 @parma[in]   pinst   application instance handle
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_keepalive_timeout(handle_ptr , void* );


static inline uint32_t BE24(uint32_t len){
    uint32_t ptr = len;
    uint32_t ret = ((uint8_t)(((char*)&ptr)[0]&0xff) << 16) +
                   ((uint8_t)(((char*)&ptr)[1]&0xff) << 8) +
                   ((uint8_t)(((char*)&ptr)[2]&0xff));
    return(ret);
}

enum {
    KEEPALIVE_CLMN_DST_IP = 0,
    KEEPALIVE_CLMN_DST_PORT,
    KEEPALIVE_CLMN_SRC_IP,
    KEEPALIVE_CLMN_SRC_PORT,
    KEEPALIVE_CLMN_PROTO,
    KEEPALIVE_CLMN_STAT,
    KEEPALIVE_CLMN_ACTIVE,
    KEEPALIVE_CLMN_SERVER_ID,
    KEEPALIVE_CLMN_SERVER_TYPE,
    KEEPALIVE_CLMN_MAX
};

enum {
    KEEPALIVE_PROTO_GTPU = 0,
    KEEPALIVE_PROTO_GTPC,
    KEEPALIVE_PROTO_MAX
};
enum {
    KEEPALIVE_STAT_ERR = -1,
    KEEPALIVE_STAT_DC = 0,
    KEEPALIVE_STAT_WAIT,
    KEEPALIVE_STAT_OK,
    KEEPALIVE_STAT_MAX
};
enum {
    KEEPALIVE_SERVER_TYPE_MASTER = 0,
    KEEPALIVE_SERVER_TYPE_SLAVE,
    KEEPALIVE_SERVER_TYPE_MAX
};


enum {
    CRSES_LOOKUP_CLMN_PGW_TEID = 0,
    CRSES_LOOKUP_CLMN_UEIPV,
    CRSES_LOOKUP_CLMN_BITRT_S5,
    CRSES_LOOKUP_CLMN_BITRT_SGI,
    CRSES_LOOKUP_CLMN_PGW_U_IPV,
    CRSES_LOOKUP_CLMN_PGW_C_IPV,
    CRSES_LOOKUP_CLMN_DNS,
    CRSES_LOOKUP_CLMN_EBI,
    CRSES_LOOKUP_CLMN_IMSI,
    CRSES_LOOKUP_CLMN_MSISDN,
    CRSES_LOOKUP_CLMN_SGW_C_TEID,
    CRSES_LOOKUP_CLMN_MAX
};


/*! @struct txnode_ext
  @brief
  tx node handle\n
  \n
 */
typedef struct txnode_ext{
    int         sock_gtpc;
    int         sock_gtpu;
    struct sockaddr_in  addr_gtpc;
    struct sockaddr_in  addr_gtpu;
}txnode_ext_t,*txnode_ext_ptr;


/*! @struct updatekey
  @brief
  update key\n
  \n
 */
typedef struct upkey {
    TAILQ_ENTRY(upkey) link;
    char    src_ip[64];
    U32     src_port;
    char    dst_ip[64];
    U32     dst_port;
    U32     next_status;
}upkey_t,*upkey_ptr;

/*! @struct timernode_ext
  @brief
  timer node handle\n
  \n
 */
typedef struct timernode_ext{
    DBPROVIDER_STMT_HANDLE stmt;
    DBPROVIDER_ST_BIND_I    bind;
    //
    TAILQ_HEAD(upkeys, upkey)  upkeys;
}timernode_ext_t,*timernode_ext_ptr;


/*! @struct create_session_node_ext
  @brief
  Create Session node handle\n
  \n
 */
typedef struct create_session_node_ext{
    DBPROVIDER_STMT_HANDLE stmt;
    DBPROVIDER_STMT_HANDLE   stmt_msisdn;
    DBPROVIDER_ST_BIND_I    bind;
}create_session_node_ext_t,*create_session_node_ext_ptr;

/*! @struct modify_bearer_node_ext
  @brief
  Modify Bearer node handle\n
  \n
 */
typedef struct modify_bearer_node_ext{
    DBPROVIDER_STMT_HANDLE stmt;
    DBPROVIDER_ST_BIND_I    bind;
}modify_bearer_node_ext_t,*modify_bearer_node_ext_ptr;

/*! @struct delete_session_node_ext
  @brief
  Delete Session node handle\n
  \n
 */
typedef struct delete_session_node_ext{
    DBPROVIDER_STMT_HANDLE stmt;
    DBPROVIDER_STMT_HANDLE stmt_v1;
    DBPROVIDER_STMT_HANDLE stmt_v1_mb;
    DBPROVIDER_ST_BIND_I    bind;
}delete_session_node_ext_t,*delete_session_node_ext_ptr;


/*! @struct gtpu_other_node_ext
  @brief
  gtpu other request node handle\n
  \n
 */
typedef struct gtpu_other_node_ext{
    DBPROVIDER_STMT_HANDLE stmt;
    DBPROVIDER_ST_BIND_I    bind;
}gtpu_other_node_ext_t,*gtpu_other_node_ext_ptr;



extern pthread_mutex_t __mysql_mutex;



#ifdef __cplusplus
}
#endif
#endif //MIXI_PGW_PGW_MODULE_H
