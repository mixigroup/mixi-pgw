/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       pgw_def.h
    @brief      mixi_pgw_ctrl_plane c struc, common header
*******************************************************************************
*******************************************************************************
    @date       created(07/nov/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license
*******************************************************************************
    @par        History
    - 07/nov/2017 
      -# Initial Version
******************************************************************************/

#ifndef MIXI_PGW_PGW_DEF_H
#define MIXI_PGW_PGW_DEF_H

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdarg.h>
#include <err.h>
#include <ctype.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <event2/event.h>
#include <event2/thread.h>
#ifdef __linux__
#include "queue.h"
#include <errno.h>
#else
#include <sys/queue.h>
#endif
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef __USE_SQLITE3__
#include <sqlite3.h>
#endif


/*! @name pragma */
/* @{ */
#define PGW_VERSION     (0xdeadbeaf)    /*!< version */
#define ON              (0x80000000)    /*!< flag,ON */
#define OK              (0)             /*!< success */
#define ERR             (-1)            /*!< failed  */
#define NOTFOUND        (-2)            /*!< not found */
#define MAX_DSTCNT      (16)            /*!< limit number of destination */
#define MAX_ADDR        (64)            /*!< address */
#define SERVER_TO       (1000)          /*!< server time out  */
#define PCKTLEN_UDP     (2048)          /*!< packet size */
#ifndef ETHER_MAX_LEN
#define ETHER_MAX_LEN   (1514)          /*!< ether packet limit */
#endif
#ifndef MIN
#define MIN(a,b) (a>b?b:a)              /*!< min */
#endif

#define UDPSERVER_SUB    (0)            /*!< sub udp server */
#define UDPSERVER_MAIN   (1)            /*!< main udp server */
#define PGW_IP8          (0x0a)         /*!< ip address first octet */

#ifndef XHTONLL
#ifndef htonll
#define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) ((1==ntohl(1)) ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))
#endif
#define	XHTONLL(x)  (htonll((__uint64_t)x))
#define	XNTOHLL(x)  (ntohll((__uint64_t)x))
#endif

/*! @name RING_SIZE_MAX
  @brief
  max ring size = 20K

  This value calculated in reverse from delay time of Create Session Request,
  which is processing bolleneck.

  Create Session Request has a timeout value = 3 sec set by SGW.

  It means that it is not necessary to taek more time than
  3 sec to reply to Create Session Response.

  And from this 3 sec = 6Ktps(Prepared SELCT x1, UPDATEx1 = total 2 queries)
  actual value 3x6=18Ktps almost 20 ring size.
*/
#ifndef RING_SIZE_MAX
#define RING_SIZE_MAX   (20*1024)       /*!< ring size : 20K */
#endif

#ifndef BURST_SELECT_THRESHOLD
#define BURST_SELECT_THRESHOLD  (4)     /*!< burst SELECT threshold */
#endif

#ifndef BURST_SIZE
#define BURST_SIZE              (32)    /*!< burst size */
#endif

#ifndef INTERACTIVE_TIMEOUT
#define INTERACTIVE_TIMEOUT     (60000000)  /*!< no operation timeout usec */
#endif

#ifndef INTERACTIVE_HALT
#define INTERACTIVE_HALT        (1000)   /*!< no operation sleep usec */
#endif

#ifndef SGWPEER_EXPIRE_ELAPSE
#define SGWPEER_EXPIRE_ELAPSE   (30)    /*!< sgw peer cache expire elapse sec */
#endif



/* @} */

/*! @name mixi_pgw_ctrl_plane typedef */
/* @{ */
typedef int RETCD;                      /*!< result code */
typedef int EVENT;                      /*!< event code */
typedef int INT;                        /*!< signed : 32bit numeric */
typedef unsigned long long U64;         /*!< unsigned : 64bit numeric */
typedef unsigned int U32;               /*!< unsigned : 32bit numeric */
typedef unsigned short U16;             /*!< unsigned : 16bit numeric */
typedef unsigned char U8;               /*!< unsigned : 8bit */
typedef const char* TXT;                /*!< generic string */
typedef void* PTR;                      /*!< pointer */
/* @} */




/*! @enum TYPE
  @brief
  module type
*/
enum TYPE{
    SERVER = 0,                         /*!< server */
    CLIENT,                             /*!< client */
    UNKNOWN
};


/*! @enum PROPERTY
  @brief
  propertyid
*/
enum PROPERTY{
    SRC_ADDR = 0,                       /*!< input interface(ip address) */
    SRC_PORT,                           /*!< input interface(udp port) */
    DST_CNT,                            /*!< count of output */
    DST_ADDR_MIN,
    DST_ADDR_00 = DST_ADDR_MIN,         /*!< output address[MIN] */
    DST_ADDR_MAX= (DST_ADDR_MIN+MAX_DSTCNT),
                                        /*!< output address[MAX] */
    DST_PORT_MIN,
    DST_PORT_00 = (DST_PORT_MIN),       /*!< output port[MIN] */
    DST_PORT_MAX= (DST_PORT_MIN+MAX_DSTCNT),
                                        /*!< output port[MAX]   */
    DB_HOST,                            /*!< DB : host */
    DB_USER,                            /*!< DB : user */
    DB_PSWD,                            /*!< DB : password  */
    DB_INST,                            /*!< DB : instance */
    DB_PORT,                            /*!< DB : port */
    DB_FLAG,                            /*!< DB : flags */
    NODE_CNT,                           /*!< count of nodes */
    EXT_SET_NUMBER,                     /*!< EXT Set Number */
    PGW_UNIT_NUMBER,                    /*!< PGW unit number */
};

/*! @enum EVENTCD
  @brief
  event code
*/
enum EVENTCD{
    EVENT_GTPRX = 0,                    /*!< gtp[c/u]packet received */
    EVENT_ENTRY,                        /*!< proxy -> node ipc */
    EVENT_POST_INIT,                    /*!< node post init. */
    EVENT_UNINIT,                       /*!< node uninit. */
    EVENT_TIMER,                        /*!< timer on fire. */
    EVENT_NOTSUPPORT
};

/*! @enum ORDERBY
  @brief
  direction
*/
enum ORDERBY{
    INGRESS = 0,                        /*!< direction:ingress */
    EGRESS                              /*!< direction:egress */
};



/*! @enum PGW_LOG_LEVEL
  @brief
  log level
*/
enum PGW_LOG_LEVEL{
    PGW_LOG_LEVEL_MIN = 0,              /*!< log level min */
    PGW_LOG_LEVEL_ERR = PGW_LOG_LEVEL_MIN,
    PGW_LOG_LEVEL_WRN,
    PGW_LOG_LEVEL_INF,
    PGW_LOG_LEVEL_DBG,
    PGW_LOG_LEVEL_ALL,
    PGW_LOG_LEVEL_MAX                   /*!< log level max */
};

#ifdef __clang__
#define ATTRIBUTE_PACKED
#else
#define ATTRIBUTE_PACKED        __attribute__ ((packed))
#endif

// adjustment by target compiler

struct server;
/*! @struct handle
    @brief
    application handle\n
    \n
*/
typedef struct handle {
    U32     version;
    U32     length;
    U32     flags;
    U32     type;
    U32     sgw_peer_expire_elapse;
    U8      ext_set_num;
    U8      pgw_unit_num;
    PTR     callback;
    PTR     callback_data;
    struct server*  server_gtpc;
    struct server*  server_gtpu;
    U32             nodecnt;
    struct node*    nodes[MAX_DSTCNT];
    struct node*    node;
    pthread_mutex_t property_mtx;
    TAILQ_HEAD(properties, property)  properties;
}ATTRIBUTE_PACKED handle_t,*handle_ptr;

/*! @struct packet
    @brief
    packet \n
    \n
*/
typedef struct packet {
    TAILQ_ENTRY(packet) link;
    U32     datalen;
    U8*     data;
    struct sockaddr_in  saddr;
    ssize_t             saddrlen;
    struct sockaddr_in  caddr;
    ssize_t             caddrlen;
}ATTRIBUTE_PACKED packet_t,*packet_ptr;


/*! @struct property
    @brief
    property\n
    \n
*/
typedef struct property {
    TAILQ_ENTRY(property) link;
    INT     id;
    PTR     data;
    INT     length;
}ATTRIBUTE_PACKED property_t,*property_ptr;



/*! @struct server
    @brief
    server\n
    \n
*/
typedef struct server {
    struct handle*      handle;
    pthread_t           thread;
    struct event_base*  event_base;
    struct event*       recv_event;
    struct event*       timeout_event;
    struct sockaddr_in  addr;
    ssize_t             addrlen;
    INT                 main_server;
    INT                 halt;
    INT                 sock;
    U16                 port;
    U8                  ip[MAX_ADDR];
    char                nic[32];
    char                server_id[32];
    U16                 server_type;
}ATTRIBUTE_PACKED server_t, *server_ptr;

/*! @struct client
    @brief
    client(=Node)\n
    \n
*/
typedef struct node {
    struct handle*  handle;
    struct node*    next;
    void*           dbhandle;
    void*           node_opt;
    U32             index;
    INT             halt;
    U64             tocounter;
    void*           peers_table;
    pthread_t       thread;
    pthread_mutex_t ingress_queue_mtx;
    TAILQ_HEAD(ingress_queue, packet) ingress_queue;
    pthread_mutex_t egress_queue_mtx;
    TAILQ_HEAD(egress_queue, packet) egress_queue;
    size_t          ingress_queue_drops;
    size_t          ingress_queue_count;
    size_t          egress_queue_drops;
    size_t          egress_queue_count;
}ATTRIBUTE_PACKED node_t, *node_ptr;

/*! @struct sgw_peer
  @brief
  gtpu other request SGW Peer\n
  \n
 */
typedef struct sgw_peer {
#ifdef __USE_SQLITE3__
    void*   link;
#else
    TAILQ_ENTRY(sgw_peer) link;
#endif
    U32     ip;
    U32     counter;
    U32     expire;
}ATTRIBUTE_PACKED sgw_peer_t,*sgw_peer_ptr;


/*! @struct sgw_peers
  @brief
  gtpu other request SGW Peer\n
  \n
 */
typedef struct sgw_peers{
#ifdef __USE_SQLITE3__
    sqlite3* peers;
    void* udata;
#else
    TAILQ_HEAD(peers, sgw_peer)  peers;
#endif
    U32     count;
    U32     updated;
}sgw_peers_t,*sgw_peers_ptr;



#endif //MIXI_PGW_PGW_DEF_H
