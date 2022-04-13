/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       main.c
    @brief      PGW main entry
*******************************************************************************
*******************************************************************************
    @date       created(28/nov/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 28/nov/2017 
      -# Initial Version
******************************************************************************/
#include "pgw.h"


// instanciate
pthread_mutex_t __mysql_mutex;
int PGW_LOG_LEVEL = PGW_LOG_LEVEL_ALL;
int PGW_RECOVERY_COUNT = GTPC_RECOVERY_1;


// static.
static RETCD event_cb(EVENT , handle_ptr , evutil_socket_t , short , const U8* , const INT ,  struct sockaddr_in*, ssize_t , struct sockaddr_in* ,ssize_t , void* );
static void read_args(int argc, char *argv[], char* ipv4, unsigned ipv4_len, char* nic, unsigned nic_len ,char* sid, unsigned sid_len);
static void usage(void);
static RETCD makepid(const char* path);
static INT __halt_process = 0;
static INT __daemon = 0;
static char __pid[128] = {0};
static U8   __ext_set_number = 0;
static U8   __pgw_unit_nubmer = 0;
static U32  __sgw_peer_expire_elapse = SGWPEER_EXPIRE_ELAPSE;

static void cleanTasks(int signo){
    __halt_process++;
    PGW_LOG(PGW_LOG_LEVEL_INF, "cleanTasks(%u)\n", (unsigned)__halt_process);
}



/**
  PGW : main entry\n
 *******************************************************************************
 GTPC echo response\n
 GTPC echo request\n
 + manage GTPC echo response\n
 GTPU echo response\n
 GTPU echo request\n
 + manage GTPU echo response\n
 *******************************************************************************
 @param[in]     argc   count of arguments
 @param[in]     argv   arguments
 @return        0/success , !=0/error
 */
int main(int argc, char *argv[]){
    handle_ptr  inst = NULL;
    server_ptr  srvr = NULL;
    U32         port = 3306,opt = 0,nodecnt = PGW_NODE_COUNT;
    char        bind_ipv4[MAX_ADDR] = {0};
    char        bind_nic[32] = {0};
    char        server_id[32] = {0};

    // read arugments
    read_args(argc, argv, bind_ipv4, sizeof(bind_ipv4)-1,
              bind_nic, sizeof(bind_nic)-1,
              server_id, sizeof(server_id)-1);
    //
    if (__daemon){
        if (daemon(1,0) != 0){ exit(1); }
    }
    if (!__pid[0]){
        pgw_panic("must -p or -i <pid file or ipv4>");
    }
    if (makepid(__pid) != OK){
        pgw_panic("can't make pidfile.");
    }
    //
    signal(SIGINT,		cleanTasks);
    signal(SIGTERM,		cleanTasks);
    signal(SIGHUP,		cleanTasks);

    openlog(argv[0], LOG_PERROR|LOG_PID,LOG_LOCAL2);
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
            pgw_set_property(inst, DB_FLAG, (PTR)&opt, sizeof(opt)) ||
            pgw_set_property(inst, EXT_SET_NUMBER, (PTR)&__ext_set_number, sizeof(__ext_set_number)) ||
            pgw_set_property(inst, PGW_UNIT_NUMBER, (PTR)&__pgw_unit_nubmer, sizeof(__pgw_unit_nubmer)) != OK){
        pgw_panic("failed. pgw_set_property");
    }
    inst->ext_set_num = (U8)__ext_set_number;
    inst->pgw_unit_num = (U8)__pgw_unit_nubmer;
    inst->sgw_peer_expire_elapse = __sgw_peer_expire_elapse;

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
    // stop(=wait for quit)
    if (pgw_stop(inst, NULL, NULL) != OK){
        pgw_panic("failed. pgw_stop");
    }
    if (pgw_release_instance(&inst) != OK){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. pgw_release_instance\n");
    }
    closelog();
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
 @param[in]   datalen receive buffer length(=packet length)
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
    //
    switch(event){
        case EVENT_POST_INIT:
            ret = event_node_init(pinst, ext);
            break;
        case EVENT_UNINIT:
            ret = event_node_uninit(pinst, ext);
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
                case PGW_NODE_TX:               ret = event_tx(pinst, ext); break;
                case PGW_NODE_GTPU_ECHO_REQ:    ret = event_gtpu_echo_req(pinst, ext); break;
                case PGW_NODE_GTPC_ECHO_REQ:    ret = event_gtpc_echo_req(pinst, ext); break;
                case PGW_NODE_GTPC_CREATE_SESSION_REQ:  ret = event_gtpc_create_session_req(pinst, ext); break;
#ifndef SINGLE_CREATE_SESSION
                case PGW_NODE_GTPC_CREATE_SESSION_REQ_EXT0:  ret = event_gtpc_create_session_req(pinst, ext); break;
#endif
                case PGW_NODE_GTPC_MODIFY_BEARER_REQ:   ret = event_gtpc_modify_bearer_req(pinst, ext); break;
                case PGW_NODE_GTPC_OTHER_REQ:           ret = event_gtpc_other_req(pinst, ext); break;
                case PGW_NODE_GTPU_OTHER_REQ:           ret = event_gtpu_other_req(pinst, ext); break;
                case PGW_NODE_TIMER:
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
        // no-operation, halt a little
        // send keepalive when a long rest state has occurred
        if (event == EVENT_ENTRY){
            pnode = (node_ptr)ext;
            pnode->tocounter += INTERACTIVE_HALT;
            //
            switch(pnode->index){
                case PGW_NODE_TX:
                case PGW_NODE_GTPU_ECHO_REQ:
                case PGW_NODE_GTPC_ECHO_REQ:
                case PGW_NODE_GTPC_CREATE_SESSION_REQ:
#ifndef SINGLE_CREATE_SESSION
                case PGW_NODE_GTPC_CREATE_SESSION_REQ_EXT0:
#endif
                case PGW_NODE_GTPC_MODIFY_BEARER_REQ:
                case PGW_NODE_GTPC_OTHER_REQ:
                case PGW_NODE_GTPU_OTHER_REQ:
                //
                // timeout on specific CPU(thread)
                if (pnode->tocounter > INTERACTIVE_TIMEOUT){
                    event_keepalive_timeout(pinst, ext);
                    ((node_ptr)ext)->tocounter = 0;
                }
                break;
            }
        }
        usleep(INTERACTIVE_HALT);
    }
    return(OK);
}


/**
  read program arguments\n
 *******************************************************************************
  \n
  \n
 *******************************************************************************
 @param[in]       argc      count of arguments
 @param[in]       argv      arguments
 @param[in/out]   ipv4      ipv4 address
 @param[in]       ipv4_len  address buffer length
 @param[in/out]   nic       nic name(reserved)
 @param[in]       nic_len   nic name length
 @param[in/out]   sid       server identifier
 @param[in]       sid_len   server identifier length
 */
void read_args(int argc, char *argv[], char* ipv4, unsigned ipv4_len, char* nic, unsigned nic_len ,char* sid, unsigned sid_len){
    int         ch,have_option = 0;
    // log level
    PGW_LOG_LEVEL = PGW_LOG_LEVEL_ALL;

    // default values
    strncpy(sid,  SERVERID, MIN(strlen(SERVERID), sid_len));
    strncpy(nic,  SRCNIC,   MIN(strlen(SRCNIC), nic_len));
    strncpy(ipv4, SRCADDRESS, MIN(strlen(SRCADDRESS), ipv4_len));
    //
    while ( (ch = getopt(argc, argv, "p:dhb:i:s:l:X:Y:r:e:")) != -1) {
        switch(ch){
            case 's': /* server id */
                if (optarg != NULL){
                    bzero(sid, sid_len);
                    strncpy(sid, optarg, MIN(strlen(optarg), sid_len));
                    sid[strlen(sid)] = '\0';
                }
                break;
            case 'b': /* bind nic  */
                if (optarg != NULL){
                    bzero(nic, nic_len);
                    strncpy(nic, optarg, MIN(strlen(optarg), nic_len));
                    nic[strlen(nic)] = '\0';
                }
                break;
            case 'i': /* ip address  */
                if (optarg != NULL){
                    bzero(ipv4, ipv4_len);
                    strncpy(ipv4, optarg, MIN(strlen(optarg), ipv4_len));
                    ipv4[strlen(ipv4)] = '\0';
                }
                break;
            case 'l': /* log level */
                if (optarg != NULL){
                    PGW_LOG_LEVEL = atoi(optarg);
                    if (PGW_LOG_LEVEL < 0 || PGW_LOG_LEVEL > PGW_LOG_LEVEL_MAX){
                        PGW_LOG_LEVEL = PGW_LOG_LEVEL_ALL;
                    }
                }
                break;
            case 'h':/* help */
                usage();
                exit(0);
                break;
            case 'd':/* daemon? */
                __daemon++;
                break;
            case 'p': /* pid */
                if (optarg != NULL){
                    bzero(__pid, sizeof(__pid));
                    strncpy(__pid, optarg, MIN(strlen(optarg), sizeof(__pid)-1));
                }
                break;
            case 'X': /* ext set number */
                if (optarg != NULL){
                    __ext_set_number = atoi(optarg);
                    have_option |= HAVE_X_OPTION;
                }
                break;
            case 'Y': /* pgw unit number */
                if (optarg != NULL){
                    __pgw_unit_nubmer = atoi(optarg);
                }
                break;
            case 'r':/* restart counter */
                if (optarg != NULL){
                    PGW_RECOVERY_COUNT = atoi(optarg);
                }
                break;
            case 'e':   /* sgw peer cache expire elapse */
                if (optarg != NULL){
                    __sgw_peer_expire_elapse = atoi(optarg);
                }
                break;
            default: /* unknown option */
                printf("Option %c is not defined.", ch);
                usage();
                exit(0);
        }
    }

    if (!__pid[0] && ipv4[0]){
        snprintf(__pid, sizeof(__pid)-1, "./pid/%s.pid", ipv4);
    }
    if (!(have_option & HAVE_X_OPTION)){
        printf("missing option.(X)\n");
        usage();
        exit(0);
    }
}


/**
 display usage\n
 *******************************************************************************
 \n
 *******************************************************************************
 */
void usage(void){
#ifdef VERSION
    printf("version %s\n", (VERSION));
#endif
    printf("-s server_id ex `hostname`\n");
    printf("-b bind nic (not use at linux)\n");
    printf("-i bind ip address \n");
    printf("-l log level(0:low - %d:max) \n",PGW_LOG_LEVEL_ALL);
    printf("-h help\n");
    printf("-d daemon\n");
    printf("-p pid file\n");
    printf("-X ext set number(must)\n");
    printf("-Y pgw unit number\n");
    printf("-r pgw recovery count(default:1)\n");
    printf("-e sgw peer cache expire elapse (default:30) sec\n");
}
/**
  generate pid file\n
 *******************************************************************************
  \n
 *******************************************************************************
 @param[in]       path      file path
 @return RETCD  0==OK,0!=error
 */
RETCD makepid(const char* path){
    int     fd;
    char    bf[32] = {0};
    //
    sprintf(bf, "%lld\n", (long long)getpid());

    if ((fd = open(path, O_CREAT | O_WRONLY | O_TRUNC,S_IREAD | S_IWRITE)) == -1){
        return(ERR);
    }
    // write row data
    if (lseek(fd,0,SEEK_SET) == -1){
        close(fd);
        unlink(path);
        return(ERR);
    }
    if (write(fd,bf,strlen(bf)) == -1){
        close(fd);
        unlink(path);
        return(ERR);
    }
    return(close(fd));
}