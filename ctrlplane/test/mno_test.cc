#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

#include <pthread.h>
#include <fcntl.h>
#include <event2/event.h>
#include <event2/thread.h>
#ifdef __linux__
#include "queue.h"
#else
#include <sys/queue.h>
#endif


#include "pgw_ext.h"
#include "node_ext.h"
#include "gtpc_ext.h"
#include "gtpu_ext.h"
#include "db_ext.h"
#ifdef __LINUX__
#define HOST                    ("127.0.0.1")
#define SRCADDRESS              ("192.168.181.18")
#define SRCNIC                  ("eno1")
#define SERVERID                ("a0000")
#else
#define HOST                    ("192.168.56.20")
#define SRCADDRESS              ("1192.168.211.64")
#define SRCNIC                  ("en0")
#define SERVERID                ("a0000")
#endif
#define USER                    ("pgw")
#define PSWD                    ("pgwp")
#define INST                    ("mixipgw")

#define GTP_ECHO_SEQNUM_MIN     (0x0001)
#define GTP_ECHO_SEQNUM_MAX     (0x7fff)

#define NODE_GTPU_ECHO_REQ      (0)
#define NODE_GTPC_ECHO_REQ      (1)
#define NODE_TIMER              (2)
#define NODE_TX                 (3)
//
#define NODECNT                 (4)
// #define	LOCALPORT_BASE		(50000)
#define	LOCALPORT_BASE		(0)
//
#define KEEPALIVE_SQL           ("SELECT dst_ip,dst_port,src_ip,src_port,proto,stat, "\
        " active,server_id,server_type "\
        "FROM keepalive "\
        "WHERE updated_at < DATE_ADD(NOW(),INTERVAL -60 SECOND) AND active=1 AND stat >= 0 AND "\
        " src_ip='%s' AND server_id='%s' ")

#define INVALID_STATUS_SQL      ("UPDATE  keepalive SET stat = %d, updated_at = NOW() "\
        "WHERE src_ip='%s' AND src_port= %u AND server_id='%s' ")
#define CHANGE_STATUS_SQL       ("UPDATE  keepalive SET stat = %d, updated_at = NOW() "\
        "WHERE src_ip='%s' AND src_port= %u AND "\
        " dst_ip='%s' AND dst_port= %u AND server_id='%s' ")
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
  update keys\n
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
    MYSQL_STMT* stmt;
    dbbind_t    bind;
    //
    TAILQ_HEAD(upkeys, upkey)  upkeys;
}timernode_ext_t,*timernode_ext_ptr;





static RETCD event_cb(EVENT , handle_ptr , evutil_socket_t , short , const U8* , const INT ,  struct sockaddr_in*, ssize_t , struct sockaddr_in* ,ssize_t , void* );
//
static RETCD event_rx(handle_ptr , const U8* , const INT ,  struct sockaddr_in* , ssize_t , struct sockaddr_in* , ssize_t, void* );
static RETCD event_tx(handle_ptr , void* );
static RETCD event_gtpu_echo_req(handle_ptr , void* );
static RETCD event_gtpc_echo_req(handle_ptr , void* );
static RETCD event_timer(handle_ptr , void* );
static RETCD event_node_init(handle_ptr , void* );
static RETCD event_node_uninit(handle_ptr , void* );
//
static INT __halt_process = 0;
static pthread_mutex_t __mysql_mutex;


static void cleanTasks(int signo){
    __halt_process++;
    printf("cleanTasks(%u)\n", (unsigned)__halt_process);
}
static inline uint32_t BE24(uint32_t len){
    uint32_t ptr = len;
    uint32_t ret = ((uint8_t)(((char*)&ptr)[0]&0xff) << 16) +
        ((uint8_t)(((char*)&ptr)[1]&0xff) << 8) +
        ((uint8_t)(((char*)&ptr)[2]&0xff));
    return(ret);
}

// instance
int PGW_LOG_LEVEL = PGW_LOG_LEVEL_ALL;

/**
  MNO test : main entry\n
 *******************************************************************************
 response GTPC echo\n
 request GTPC echo\n
 + manage GTPC echo response \n
 response GTPU echo\n
 request GTPU echo \n
 + mange GTPU echo response \n
 *******************************************************************************
 @param[in]     argc   count of arguments
 @param[in]     argv   arguments
 @return        0/success , !=0/error
 */
int main(int argc, char *argv[]){
    handle_ptr  inst = NULL;
    server_ptr  srvr = NULL;
    U32         port = 3306,opt = 0,nodecnt = NODECNT;
    int         ch;
    char        bind_ipv4[MAX_ADDR] = {0};
    char        bind_nic[32] = {0};
    char        server_id[32] = {0};
    //
    signal(SIGINT,		cleanTasks);
    signal(SIGTERM,		cleanTasks);
    signal(SIGHUP,		cleanTasks);
    // initialize bind regions(defaul value)
    strncpy(server_id, SERVERID, MIN(strlen(SERVERID), sizeof(server_id)-1));
    strncpy(bind_nic,  SRCNIC, MIN(strlen(SRCNIC), sizeof(bind_nic)-1));
    strncpy(bind_ipv4, SRCADDRESS, MIN(strlen(SRCADDRESS), sizeof(bind_ipv4)-1));
    // read options , set bind ip, NIC, server_id
    while ( (ch = getopt(argc, argv, "b:i:s:")) != -1) {
        switch(ch){
        case 's': /* server id */
            if (optarg != NULL){
                bzero(server_id, sizeof(server_id));
                strncpy(server_id, optarg, MIN(strlen(optarg), sizeof(server_id)-1));
                server_id[strlen(server_id)] = '\0';
            }
            break;
        case 'b': /* bind nic  */
            if (optarg != NULL){
                bzero(bind_nic, sizeof(bind_nic));
                strncpy(bind_nic, optarg, MIN(strlen(optarg), sizeof(bind_nic)-1));
                bind_nic[strlen(bind_nic)] = '\0';
            }
            break;
        case 'i': /* ip address  */
            if (optarg != NULL){
                bzero(bind_ipv4, sizeof(bind_ipv4));
                strncpy(bind_ipv4, optarg, MIN(strlen(optarg), sizeof(bind_ipv4)-1));
                bind_ipv4[strlen(bind_ipv4)] = '\0';
            }
            break;
        }
    }
    //
    if (pgw_create_instance(SERVER, &inst) != OK){
        pgw_panic("failed. pgw_create_instance");
    }
    if (pgw_set_property(inst, NODE_CNT, (PTR)&nodecnt, sizeof(nodecnt)) ||
            pgw_set_property(inst, DB_HOST, (PTR)HOST, strlen(HOST)) ||
            pgw_set_property(inst, DB_USER, (PTR)USER, strlen(USER)) ||
            pgw_set_property(inst, DB_PSWD, (PTR)PSWD, strlen(PSWD)) ||
            pgw_set_property(inst, DB_INST, (PTR)INST, strlen(INST)) ||
            pgw_set_property(inst, DB_PORT, (PTR)&port, sizeof(port)) ||
            pgw_set_property(inst, DB_FLAG, (PTR)&opt, sizeof(opt)) != OK){
        pgw_panic("failed. pgw_set_property");
    }

    // GTPC
    srvr = (server_ptr)inst->server_gtpc;
    srvr->port = GTPC_PORT;
    srvr->main_server = UDPSERVER_MAIN;
    strncpy((char*)srvr->ip, bind_ipv4, MIN(strlen(bind_ipv4), sizeof(srvr->ip)-1));
    strncpy((char*)srvr->nic, bind_nic, MIN(strlen(bind_nic), sizeof(srvr->nic)-1));
    strncpy((char*)srvr->server_id, server_id, MIN(strlen(server_id), sizeof(srvr->server_id)-1));

    // GTPU
    srvr = (server_ptr)inst->server_gtpu;
    srvr->port = GTPU1_PORT;
    srvr->main_server = UDPSERVER_SUB;
    strncpy((char*)srvr->ip, bind_ipv4, MIN(strlen(bind_ipv4), sizeof(srvr->ip)-1));
    strncpy((char*)srvr->nic, bind_nic, MIN(strlen(bind_nic), sizeof(srvr->nic)-1));
    strncpy((char*)srvr->server_id, server_id, MIN(strlen(server_id), sizeof(srvr->server_id)-1));
    //
    if (pgw_start(inst, event_cb, NULL) != OK){
        pgw_panic("failed. pgw_start");
    }
    // main loop
    while(1){
        usleep(100000);
        if (__halt_process)
            break;
    }
    sleep(1);
    // wait for all thread stopped.
    if (pgw_stop(inst, NULL, NULL) != OK){
        pgw_panic("failed. pgw_stop");
    }
    if (pgw_release_instance(&inst) != OK){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. pgw_release_instance\n");
    }
    //
    return(OK);
}

/**
  eventcallback\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   event   event code
 @parma[in]   pinst   application instance handle
 @param[in]   sock    socket descriptor
 @param[in]   what    reason
 @param[in]   data    receive buffer
 @param[in]   datalen receive buffer長(=packet 長)
 @param[in]   saddr   server address
 @param[in]   saddrlen address length
 @param[in]   caddr   client address
 @param[in]   caddrlen address length
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_cb(
        EVENT event, handle_ptr pinst, evutil_socket_t sock ,
        short what, const U8* data, const INT datalen,
        struct sockaddr_in* saddr, ssize_t saddrlen,
        struct sockaddr_in* caddr, ssize_t caddrlen, void* ext){
    RETCD       ret = ERR;
    node_ptr    pnode = NULL;
    /*
     *
     *  Node = CPU Core = thread
     * +------+---------------+----------------+-------+
     * | node |    ingress    | egress         | event |
     * | num  |               |                |       |
     * +------+---------------+----------------+-------+
     * |      | nic  -> ring  | ring -> nic    | GTP   |
     * |      |               |                | INGR  |
     * |   0  |gtpu(echo) req |grpu(echo)res   | EGRS  |
     * +------+---------------+----------------+       |
     * |      | nic  -> ring  | ring -> nic    |       |
     * |      |               |                |       |
     * |   1  |gtpc(echo) req |gtpc(echo)req   |       |
     * +------+---------------+----------------+-------+
     * |      |               |                |       |
     * |      |   timer       | ring -> nic    |       |
     * |   2  |    - - -      |gtpu(echo)req   |       |
     * +------+---------------+----------------+-------+
     * |      |               |                |       |
     * |      |               | ring -> nic    |       |
     * |   3  |               |     - - -      |       |
     * +------+---------------+----------------+-------+
     *
     */
    switch(event){
        case EVENT_POST_INIT:
            pthread_mutex_lock(&__mysql_mutex);
            ret = event_node_init(pinst, ext);
            pthread_mutex_unlock(&__mysql_mutex);
            break;
        case EVENT_UNINIT:
            pthread_mutex_lock(&__mysql_mutex);
            ret = event_node_uninit(pinst, ext);
            pthread_mutex_unlock(&__mysql_mutex);
            break;
        case EVENT_TIMER:
            ret = event_timer(pinst, ext);
            break;
        case EVENT_GTPRX:
            ret = event_rx(pinst, data, datalen,  saddr, saddrlen, caddr, caddrlen, ext);
            break;
        case EVENT_ENTRY:
            pnode = (node_ptr)ext;
            switch(pnode->index){
                case NODE_TX:
                    ret = event_tx(pinst, ext);
                    break;
                case NODE_GTPU_ECHO_REQ:
                    ret = event_gtpu_echo_req(pinst, ext);
                    break;
                case NODE_GTPC_ECHO_REQ:
                    ret = event_gtpc_echo_req(pinst, ext);
                    break;
                case NODE_TIMER:
                    break;
                default:
                    pgw_panic("not implemented node.(%u)", pnode->index);
                    break;
            }
            break;
        default:
            pgw_panic("not implemented event.(%u)", event);
            break;
    }
    if (ret != OK){
        // not processed, take a little halt
        usleep(1000);
    }
    return(OK);
}
/**
  event : Rx Core\n
 *******************************************************************************
 \n
 *******************************************************************************
 @parma[in]   pinst   application instance handle
 @param[in]   data    receive buffer
 @param[in]   datalen receive buffer長(=packet 長)
 @param[in]   saddr   server address
 @param[in]   saddrlen  address length
 @param[in]   caddr   client address
 @param[in]   caddrlen  address length
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_rx(handle_ptr pinst, const U8* data, const INT datalen,  struct sockaddr_in* saddr, ssize_t saddrlen, struct sockaddr_in* caddr, ssize_t caddrlen, void* ext){
    RETCD       nburst;
    server_ptr  pserver = NULL;
    packet_ptr  pkt = NULL;
    pserver = (server_ptr)ext;

    PGW_LOG(PGW_LOG_LEVEL_ERR, "event_rx..(%p : %p)\n", pserver, (void*)pthread_self());

    // receive gtp[c/u] : 1 packet from socket descriptor 
    // callback from libevent context(thread)
    if (pgw_alloc_packet(&pkt, datalen) != OK){
        pgw_panic("failed. memory allocate.");
    }
    memcpy(pkt->data, data, datalen);
    memcpy(&pkt->saddr, saddr, sizeof(*saddr));
    memcpy(&pkt->caddr, caddr, sizeof(*caddr));
    pkt->saddrlen   = saddrlen;
    pkt->caddrlen   = caddrlen;

    // gtpu or gtpc
    if (pserver->port == GTPU1_PORT){
        if (gtpu_header(pkt)->v1_flags.version == GTPU_VERSION_1 ){
            if (gtpu_header(pkt)->type == GTPU_ECHO_REQ ||
                    gtpu_header(pkt)->type == GTPU_ECHO_RES){
                if (pgw_enq(pinst->nodes[NODE_GTPU_ECHO_REQ], INGRESS, pkt) != OK){
                    pgw_panic("failed. enq(gtpu echo req/res).");
                }
                return(OK);
            }
        }else{
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid. gtpu version\n");
        }
    }else if(pserver->port == GTPC_PORT && gtpc_header(pkt)->v2_flags.version == GTPC_VERSION_2){
        if (gtpc_header(pkt)->v2_flags.version == GTPC_VERSION_2){
            if (gtpc_header(pkt)->type == GTPC_ECHO_REQ ||
                    gtpc_header(pkt)->type == GTPC_ECHO_RES){
                if (pgw_enq(pinst->nodes[NODE_GTPC_ECHO_REQ], INGRESS, pkt) != OK){
                    pgw_panic("failed. enq(gtpc echo req/res).");
                }
                return(OK);
            }
        }else{
            PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid. gtpc version\n");

        }
    }else{
        PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid port.. or version(%u)\n", pserver->port);
    }
    // out of target packet(exclude for GTPU/GTPC ECHO)
    pgw_free_packet(pkt);

    return(ERR);
}
/**
  event : TX Core\n
 *******************************************************************************
 \n
 *******************************************************************************
 @parma[in]   pinst   application instance handle
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_tx(handle_ptr pinst, void* ext){
    node_ptr    pnode = (node_ptr)ext;
    txnode_ext_ptr tnep = (txnode_ext_ptr)pnode->node_opt;
    RETCD       nburst, sent = 0,ret;
    INT         n,m,sock;
    packet_ptr  pkts[32] = {NULL};
    char        sql[256] = {0};

    //
    int idx[NODE_TX] = { NODE_GTPU_ECHO_REQ, NODE_GTPC_ECHO_REQ, NODE_TIMER };
    for(n = 0;n < NODE_TX;n++){
        nburst = pgw_deq_burst(pinst->nodes[idx[n]], EGRESS, pkts, 32);
        if (nburst > 0){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "event_tx..(%p : %p : %u/%d)\n", pnode, (void*)pthread_self(), pnode->index, nburst);
            sent += nburst;
            for(m = 0;m < nburst;m++){
                // send from  gtpc interface or gtpu interface
                if (idx[n] == NODE_TIMER){
                    sock = -1;
                    if (pkts[m]->saddr.sin_port == htons(GTPU1_PORT)){
                        sock = tnep->sock_gtpu;
                    }else if (pkts[m]->saddr.sin_port == htons(GTPC_PORT)){
                        sock = tnep->sock_gtpc;
                    }else{
                        PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid address.(%08x/%u).\n",
                                pkts[m]->saddr.sin_addr.s_addr,
                                ntohs(pkts[m]->saddr.sin_port)
                               );
                        ret = NOTFOUND;
                    }
                    if (sock != -1){
                        ret = pgw_send_sock(sock, &pkts[m]->caddr, pkts[m]->caddrlen, pkts[m]->data, pkts[m]->datalen);
                    }
                }else{
                    ret = pgw_send(pinst, &pkts[m]->saddr, &pkts[m]->caddr, pkts[m]->caddrlen, pkts[m]->data, pkts[m]->datalen);
                }
                if (ret != OK){
                    PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. pgw_send.(%d).\n", ret);
                    // when invalid server address, set status = error
                    if (ret == NOTFOUND){
                        snprintf(sql, sizeof(sql)-1, INVALID_STATUS_SQL,
                                KEEPALIVE_STAT_ERR,
                                inet_ntoa(pkts[m]->saddr.sin_addr),
                                ntohs(pkts[m]->saddr.sin_port),
                                pnode->handle->server_gtpc->server_id);
                        if (mysql_query((MYSQL*)pnode->dbhandle, sql) != 0){
                            PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. mysql_query.(%s).\n", DBPROVIDER_ERROR(pnode->dbhandle));
                        }
                    }
                }
                // send completed, can be release paket
                pgw_free_packet(pkts[m]);
            }
        }
    }
    if (sent > 0){
        return(OK);
    }
    return(ERR);
}

/**
  event : GTPU Echo Request Core\n
 *******************************************************************************
 Rx -> gtpu echo [request/ response]\n
 - request:\n
 convert to response, send to tx via software ring\n
 - response:\n
 update status by dst[ip/port],src[ip/port]\n
 *******************************************************************************
 @parma[in]   pinst   application instance handle
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_gtpu_echo_req(handle_ptr pinst, void* ext){
    node_ptr    pnode = (node_ptr)ext;
    RETCD       nburst;
    INT         m;
    packet_ptr  pkts[32] = {NULL};
    char        sql[1024] = {0};
    char        sip[64] = {0}, dip[64] = {0};
    //
    nburst = pgw_deq_burst(pnode, INGRESS, pkts, 32);
    if (nburst > 0){
        for(m = 0;m < nburst;m++){
            if (gtpu_header(pkts[m])->type == GTPU_ECHO_REQ){
                PGW_LOG(PGW_LOG_LEVEL_ERR, "event_gtpu_echo_req..(%p : %p : %u/%d)\n", pnode, (void*)pthread_self(), pnode->index, nburst);
                // equivalent to worker core,
                // convert INGRESS -> EGRESS,
                // convert GTPU Echo Request -> GTPU Echo Response and,
                // forwarding packet to.
                gtpu_header(pkts[m])->type = GTPU_ECHO_RES;
                pgw_swap_address(pkts[m]);
                if (pgw_enq(pnode, EGRESS, pkts[m]) != OK){
                    pgw_panic("failed. enq(gtpu echo req).");
                }
            }else if (gtpu_header(pkts[m])->type == GTPU_ECHO_RES){
                PGW_LOG(PGW_LOG_LEVEL_ERR, "event_gtpu_echo_res..(%p : %p : %u/%d)\n", pnode, (void*)pthread_self(), pnode->index, nburst);
                // replace src/dst in response(keepalive table)
                strncpy(sip, inet_ntoa(pkts[m]->saddr.sin_addr), sizeof(sip)-1);
                strncpy(dip, inet_ntoa(pkts[m]->caddr.sin_addr), sizeof(dip)-1);
                snprintf(sql, sizeof(sql)-1, CHANGE_STATUS_SQL,
                        KEEPALIVE_STAT_OK,
                        dip, ntohs(pkts[m]->caddr.sin_port),
                        sip, ntohs(pkts[m]->saddr.sin_port),
                        pnode->handle->server_gtpc->server_id);

                pthread_mutex_lock(&__mysql_mutex);
                if (mysql_query((MYSQL*)pnode->dbhandle, sql) != 0){
                    PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. mysql_query.(%s).\n", DBPROVIDER_ERROR(pnode->dbhandle));
                }
                pthread_mutex_unlock(&__mysql_mutex);
                PGW_LOG(PGW_LOG_LEVEL_ERR, "== GTPU RES(%p:%p) ==\n\t%s\n========\n",pnode->dbhandle, (void*)pthread_self(),  sql);
                // release packet
                pgw_free_packet(pkts[m]);
            }else{
                PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. event_gtpu_echo_[req/res].\n");
            }
        }
        return(OK);
    }
    return(ERR);
}

/**
  event : GTPC Echo Request Core\n
 *******************************************************************************
 Rx -> gtpc echo [request/ response]\n
 - request:\n
 convert to response, send to tx via software ring
 - response:\n
 update status by dst[ip/port],src[ip/port]\n
 *******************************************************************************
 @parma[in]   pinst   application instance handle
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_gtpc_echo_req(handle_ptr pinst, void* ext){
    node_ptr    pnode = (node_ptr)ext;
    RETCD       nburst;
    INT         m;
    packet_ptr  pkts[32] = {NULL};
    char        sql[1024] = {0};
    char        sip[64] = {0},dip[64] = {0};
    //
    nburst = pgw_deq_burst(pnode, INGRESS, pkts, 32);
    if (nburst > 0){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "event_gtpc_echo_req..(%p : %p : %u/%d)\n", pnode, (void*)pthread_self(), pnode->index, nburst);
        for(m = 0;m < nburst;m++){
            if (gtpc_header(pkts[m])->type == GTPC_ECHO_REQ){
                // equivalent to worker core,
                // convert INGRESS -> EGRESS,
                // convert GTPC Echo Request -> GTPC Echo Response and,
                // forwarding packet to.
                gtpc_header(pkts[m])->type = GTPC_ECHO_RES;
                pgw_swap_address(pkts[m]);
                if (pgw_enq(pnode, EGRESS, pkts[m]) != OK){
                    pgw_panic("failed. enq(gtpc echo req).");
                }
            }else if (gtpc_header(pkts[m])->type == GTPC_ECHO_RES){
                // replace src/dst in response(keepalive table)
                strncpy(sip, inet_ntoa(pkts[m]->saddr.sin_addr), sizeof(sip)-1);
                strncpy(dip, inet_ntoa(pkts[m]->caddr.sin_addr), sizeof(dip)-1);
                snprintf(sql, sizeof(sql)-1, CHANGE_STATUS_SQL,
                        KEEPALIVE_STAT_OK,
                        dip, ntohs(pkts[m]->caddr.sin_port),
                        sip, ntohs(pkts[m]->saddr.sin_port),
                        pnode->handle->server_gtpc->server_id);
                pthread_mutex_lock(&__mysql_mutex);
                if (mysql_query((MYSQL*)pnode->dbhandle, sql) != 0){
                    PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. mysql_query.(%s).\n", DBPROVIDER_ERROR(pnode->dbhandle));
                }
                PGW_LOG(PGW_LOG_LEVEL_ERR, "== GTPC RES(%p:%p) ==\n\t%s\n========\n", (void*)pthread_self(), pnode->dbhandle, sql);
                pthread_mutex_unlock(&__mysql_mutex);

                // release packet
                pgw_free_packet(pkts[m]);
            }
        }
        return(OK);
    }
    return(ERR);
}


static inline RETCD on_rec(U32 counter, U32 clmncnt, colmn_ptr col, record_ptr rec, void* udat){
    // send gtp[c/u] echo to destination
    packet_ptr  pkt = NULL;
    U16 len = sizeof(gtpc_header_t) + sizeof(gtpc_recovery_t) - sizeof(U32);
    handle_ptr pinst = (handle_ptr)udat;
    timernode_ext_ptr tnode = NULL;
    upkey_ptr ukey = NULL;
    //
    if (pinst == NULL){
        return(ERR);
    }
    if (clmncnt != KEEPALIVE_CLMN_MAX){
        return(ERR);
    }
    // server_type==0(only master server) -> send keepalive
    // slave server -> response to keepalive
    if (rec[KEEPALIVE_CLMN_SERVER_TYPE].u.nval != KEEPALIVE_SERVER_TYPE_MASTER){
        return(ERR);
    }
    if (pinst->nodes[NODE_TIMER] && pinst->nodes[NODE_TIMER]->node_opt){
        tnode = (timernode_ext_ptr)pinst->nodes[NODE_TIMER]->node_opt;
    }
    if (tnode == NULL){
        return(ERR);
    }
    if (pgw_alloc_packet(&pkt, len) != OK){
        pgw_panic("failed. memory allocate.");
    }
    // destination address
    pkt->caddr.sin_family    = AF_INET;
    pkt->caddr.sin_port      = htons(rec[KEEPALIVE_CLMN_DST_PORT].u.nval);
    inet_pton(AF_INET, rec[KEEPALIVE_CLMN_DST_IP].u.sval ,&pkt->caddr.sin_addr.s_addr);
    pkt->caddrlen            = sizeof(struct sockaddr_in);
    // source address
    pkt->saddr.sin_family    = AF_INET;
    pkt->saddr.sin_port      = htons(rec[KEEPALIVE_CLMN_SRC_PORT].u.nval);
    inet_pton(AF_INET, rec[KEEPALIVE_CLMN_SRC_IP].u.sval ,&pkt->saddr.sin_addr.s_addr);
    pkt->saddrlen            = sizeof(struct sockaddr_in);

    // save update keys after callback completed.(temporary save)
    if ((ukey = (upkey_ptr)malloc(sizeof(upkey_t))) == NULL){
        pgw_panic("failed. memory allocate... upkey");
    }
    memcpy(ukey->src_ip, rec[KEEPALIVE_CLMN_SRC_IP].u.sval, MIN(sizeof(ukey->src_ip), sizeof(rec[KEEPALIVE_CLMN_SRC_IP].u.sval)));
    ukey->src_port = rec[KEEPALIVE_CLMN_SRC_PORT].u.nval;
    memcpy(ukey->dst_ip, rec[KEEPALIVE_CLMN_DST_IP].u.sval, MIN(sizeof(ukey->dst_ip), sizeof(rec[KEEPALIVE_CLMN_DST_IP].u.sval)));
    ukey->dst_port = rec[KEEPALIVE_CLMN_DST_PORT].u.nval;
    ukey->next_status = KEEPALIVE_STAT_WAIT;

    //
    TAILQ_INSERT_TAIL(&(tnode->upkeys), ukey, link);

    //
    if (rec[KEEPALIVE_CLMN_PROTO].u.nval == KEEPALIVE_PROTO_GTPC){
        static U32 gtpc_seqnum = GTP_ECHO_SEQNUM_MIN;
        PGW_LOG(PGW_LOG_LEVEL_ERR, ">> GTPC >>\n\tsrc[%08x:%u] dst[%08x:%u]\n\tukey src[%s:%u] - dst[%s:%u]\n<<<<<<<<\n",
                pkt->saddr.sin_addr.s_addr,
                pkt->saddr.sin_port,
                pkt->caddr.sin_addr.s_addr,
                pkt->caddr.sin_port,
                ukey->src_ip, ukey->src_port,
                ukey->dst_ip, ukey->dst_port
               );
        // GTPC echo request
        gtpc_header_ptr gtpch = (gtpc_header_ptr)pkt->data;
        gtpc_recovery_ptr gtpc_rcvr = (gtpc_recovery_ptr)(pkt->data + sizeof(gtpc_header_t) - sizeof(U32));
        //
        gtpch->v2_flags.version = GTPC_VERSION_2;
        gtpch->v2_flags.teid    = GTPC_TEID_OFF;
        gtpch->v2_flags.piggy   = GTPC_PIGGY_OFF;
        gtpch->type             = GTPC_ECHO_REQ;
        gtpch->length           = htons(sizeof(gtpc_recovery_t)+4);
        gtpch->t.sq.seqno       = BE24(gtpc_seqnum++);
        gtpc_rcvr->head.type    = GTPC_TYPE_RECOVERY;
        gtpc_rcvr->head.length  = htons(1);
        gtpc_rcvr->recovery_restart_counter = 8;
        pkt->datalen            = len;
        if (gtpc_seqnum>=GTP_ECHO_SEQNUM_MAX){ gtpc_seqnum = GTP_ECHO_SEQNUM_MIN; }
    }else if (rec[KEEPALIVE_CLMN_PROTO].u.nval == KEEPALIVE_PROTO_GTPU){
        static U32 gtpu_seqnum = GTP_ECHO_SEQNUM_MIN;
        PGW_LOG(PGW_LOG_LEVEL_ERR, ">> GTPU >>\n\tsrc[%08x:%u] dst[%08x:%u]\n\tukey src[%s:%u] - dst[%s:%u]\n<<<<<<<<\n",
                pkt->saddr.sin_addr.s_addr,
                pkt->saddr.sin_port,
                pkt->caddr.sin_addr.s_addr,
                pkt->caddr.sin_port,
                ukey->src_ip, ukey->src_port,
                ukey->dst_ip, ukey->dst_port
               );
        // GTPU echo request
        gtpu_header_ptr gtpuh = (gtpu_header_ptr)pkt->data;
        //
        gtpuh->v1_flags.npdu = GTPU_NPDU_OFF;
        gtpuh->v1_flags.sequence = GTPU_SEQ_1;
        gtpuh->v1_flags.extension = GTPU_EXTEND_0;
        gtpuh->v1_flags.proto = GTPU_PROTO_GTP;
        gtpuh->v1_flags.version = GTPU_VERSION_1;
        gtpuh->type     = GTPU_ECHO_REQ;
        gtpuh->length   = htons(4);
        gtpuh->tid      = 0;
        *(U32*)&(gtpuh[1]) = htons(gtpu_seqnum++);
        pkt->datalen    = sizeof(gtpu_header_t)+4;
        if (gtpu_seqnum>=GTP_ECHO_SEQNUM_MAX){ gtpu_seqnum = GTP_ECHO_SEQNUM_MIN; }
    }else{
        pgw_free_packet(pkt);
        pgw_panic("?????");

        return(ERR);
    }
    if (pgw_enq(pinst->nodes[NODE_TIMER], EGRESS, pkt) != OK){
        pgw_panic("failed. enq(gtpu echo req).");
    }
    return(OK);
}
/**
  event : timer Core\n
 *******************************************************************************
 do not modify node object in timer context\n
 \n
 \n
 - start life monitoring for GTP[u/c]-directed servers configured in database\n
 *******************************************************************************
 @parma[in]   pinst   application instance handle
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_timer(handle_ptr pinst, void* ext){
    server_ptr pserver = (server_ptr)ext;
    upkey_ptr  cur, curtmp, freeptr;
    char sql[1024] = {0};

    // use timer node
    node_ptr node = pinst->nodes[NODE_TIMER];
//  fprintf(stderr, "event_timer..(%p : %p : %u)\n", node, (void*)pthread_self(), node->index);
    // GTP[u/c] Echo request to peer
    // that has exceeded 60 sec since last update. 
    if (node && node->node_opt){
        timernode_ext_ptr tnep = (timernode_ext_ptr)node->node_opt;
        //
        if (DBPROVIDER_EXECUTE(tnep->stmt, tnep->bind.bind, on_rec, pinst) != OK){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. event_mysql_execute..(%p)\n", (void*)pthread_self());
        }
        pthread_mutex_lock(&__mysql_mutex);
        //
        TAILQ_FOREACH_SAFE(cur, &tnep->upkeys, link, curtmp) {
            snprintf(sql, sizeof(sql)-1, CHANGE_STATUS_SQL, KEEPALIVE_STAT_WAIT, 
                    cur->src_ip, cur->src_port,
                    cur->dst_ip, cur->dst_port,
                    node->handle->server_gtpc->server_id);
            PGW_LOG(PGW_LOG_LEVEL_ERR, "== REQ ==\n\t%s\n========\n", sql);
            if (mysql_query((MYSQL*)node->dbhandle, sql) != 0){
                PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. mysql_query.(%s).\n", DBPROVIDER_ERROR(node->dbhandle));
            }
            freeptr = cur;
            TAILQ_REMOVE(&tnep->upkeys, cur, link);
            free(freeptr);
        }
        pthread_mutex_unlock(&__mysql_mutex);
    }
    return(ERR);
}
/**
  event : initialize node\n
 *******************************************************************************
 initialize event every nodes\n
 *******************************************************************************
 @parma[in]   pinst   application instance handle
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_node_init(handle_ptr pinst, void* ext){
    node_ptr    pnode = (node_ptr)ext;
    PGW_LOG(PGW_LOG_LEVEL_ERR, "event_node_init..(%p : %p : %u)\n", pnode, (void*)pthread_self(), pnode->index);
    //
    if (pnode->index == NODE_TIMER){
        char sql[1024] = {0};
        timernode_ext_ptr   tnep;
        INT n;
        MYSQL* db = (MYSQL*)pnode->dbhandle;
        //
        tnep = (timernode_ext_ptr)malloc(sizeof(timernode_ext_t));
        if (tnep == NULL){
            pgw_panic("failed. malloc(%u)", (unsigned)sizeof(timernode_ext_t));
        }
        TAILQ_INIT(&tnep->upkeys);
        snprintf(sql, sizeof(sql)-1, KEEPALIVE_SQL, 
            pnode->handle->server_gtpc->ip,
            pnode->handle->server_gtpc->server_id);
        pnode->node_opt = tnep;
        //
        PGW_LOG(PGW_LOG_LEVEL_ERR, "event_node_init..(%p : %p : %u : %s)\n", pnode, (void*)pthread_self(), pnode->index, sql);
        if ((tnep->stmt = DBPROVIDER_STMT_INIT(db)) == NULL){
            pgw_panic("failed. mysql_stmt_init(%s)", DBPROVIDER_ERROR(db));
        }
        if (mysql_stmt_prepare(tnep->stmt, sql, strlen(sql)) != 0) {
            if (DBPROVIDER_ERRNO(db) == 2006){
                if (mysql_stmt_prepare(tnep->stmt, sql, strlen(sql)) != 0) {
                    pgw_panic("failed. mysql_stmt_prepare(%s : %s)",DBPROVIDER_STMT_ERROR(tnep->stmt),  DBPROVIDER_ERROR(db));
                }
            }else{
                pgw_panic("failed. mysql_stmt_prepare .. (%s)", DBPROVIDER_ERROR(db));
            }
        }
        // initialize bind regions
        DBPROVIDER_BIND_INIT(&tnep->bind);

        return(OK);
    }else if (pnode->index == NODE_TX){
        // preparing descriptor for send(tx node)
        txnode_ext_ptr  tnep;
        //
        tnep = (txnode_ext_ptr)malloc(sizeof(txnode_ext_t));
        if (tnep == NULL){
            pgw_panic("failed. malloc(%u)", (unsigned)sizeof(txnode_ext_t));
        }
        pnode->node_opt = tnep;
        // gtpc-side socket for send
        tnep->sock_gtpc = ((handle_ptr)(pnode->handle))->server_gtpc->sock;
        if (tnep->sock_gtpc < 0) {
            pgw_panic("failed. create socket(%d: %s)", errno, strerror(errno));
        }
        // gtpu-side socket for send
        tnep->sock_gtpu = ((handle_ptr)(pnode->handle))->server_gtpu->sock;
        if (tnep->sock_gtpu < 0) {
            pgw_panic("failed. create socket(%d: %s)", errno, strerror(errno));
        }
        return(OK);
    }
    return(ERR);
}

/**
  event : cleanup node\n
 *******************************************************************************
 clean up every nodes\n
 *******************************************************************************
 @parma[in]   pinst   application instance handle
 @param[in]   ext     extend data, every event code
 @return int  0==OK,0!=error
 */
RETCD event_node_uninit(handle_ptr pinst, void* ext){
    node_ptr    pnode = (node_ptr)ext;
    PGW_LOG(PGW_LOG_LEVEL_ERR, "event_node_uninit..(%p : %p/%u)\n", pnode, (void*)pthread_self(), pnode->index);
    //
    if (pnode->index == NODE_TIMER){
        if (pnode->node_opt){
            timernode_ext_ptr tnep = (timernode_ext_ptr)pnode->node_opt;
            if (tnep->stmt){
                DBPROVIDER_STMT_CLOSE(tnep->stmt);
            }
            free(pnode->node_opt);
        }
        pnode->node_opt = NULL;
    }else if (pnode->index == NODE_TX){
        if (pnode->node_opt){
            txnode_ext_ptr tnep = (txnode_ext_ptr)pnode->node_opt;
            close(tnep->sock_gtpc);
            close(tnep->sock_gtpu);
            //
            free(pnode->node_opt);
        }
        pnode->node_opt = NULL;
    }

    return(ERR);
}
