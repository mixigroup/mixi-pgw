/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane_delete_bearer
    @file       cmd_delete_bearer.c
    @brief      command delete bearer entry point
*******************************************************************************
*******************************************************************************
    @date       created(30/mar/2018)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 30/mar/2018 
      -# Initial Version
******************************************************************************/

#include "pgw_ext.h"
#include "gtpc_ext.h"
#include "db_ext.h"
#include <mysql/mysql.h>

#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)


#ifndef DHOST
#define HOST                    ("127.0.0.1")
#else
#define HOST                    STR(DHOST)
#endif
#ifndef DSRCADDRESS
#define SRCADDRESS              ("127.0.0.1")
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

struct tunnel_record{
    U32     sgw_gtpc_teid;
    U8      ebi;
    U8      latest_gtp_version;
    char    sgw_gtpc_ipv[32];
    char    pgw_gtpc_ipv[32];
};

int PGW_LOG_LEVEL = PGW_LOG_LEVEL_ALL;
int PGW_RECOVERY_COUNT = GTPC_RECOVERY_1;

static void read_args(int argc, char *argv[], U64* imsi, U32* timeout);
static void usage(const char* name, const char* fmt, ...);
static int  find_tunnel_by_imsi(MYSQL* dbh, U64 imsi, struct tunnel_record* rec);

/**
  Command Delete Bearer : main entry\n
 *******************************************************************************
 + \n
 *******************************************************************************
 @param[in]     argc   count of arguments
 @param[in]     argv   arguments
 @return        0/success , !=0/error
 */
int main(int argc, char *argv[]){
    int         sock = -1;
    U64         imsi = 0;
    U32         timeout = 0;
    U16         port = 3306;
    U16         seqno,len;
    my_bool     reconnect = 1;
    MYSQL*      dbh = NULL;
    struct tunnel_record    rec;
    struct sockaddr_in addr;
    gtp_packet_ptr  packet = NULL;
    gtpc_ebi_t          ebi;
    gtpc_nsapi_v1_t     nsapi;
    gtpc_teardown_ind_v1_t  teard;

    // read arguments
    read_args(argc, argv, &imsi, &timeout);
    openlog(argv[0], LOG_PERROR|LOG_PID,LOG_LOCAL2);

    PGW_LOG(PGW_LOG_LEVEL_INF, " >> start(%p)\n", (void*)pthread_self());

    // initialize database connection
    mysql_thread_init();
    if ((dbh = mysql_init(NULL)) == NULL){
        pgw_panic("failed. mysql_init (%p)\n", (void*)pthread_self());
    }
    mysql_options(dbh, MYSQL_OPT_RECONNECT, &reconnect);
    if (!mysql_real_connect(dbh, (TXT)HOST, (TXT)USER, (TXT)PSWD, (TXT)INST, (U32)port, NULL, 0)){
        pgw_panic("failed. mysql_real_connect (%p : %s/%s/%s/%s/%u/%u)\n",(void*)pthread_self(), (TXT)HOST, (TXT)USER, (TXT)PSWD, (TXT)INST, (U32)port, 0);
    }
    mysql_set_character_set(dbh, "utf8");

    // collect data read from imsi
    bzero(&rec, sizeof(rec));
    if (find_tunnel_by_imsi(dbh, imsi, &rec) != 0){
        pgw_panic("failed. find_tunnel_by_imsi (%p:%llu)\n", (void*)pthread_self(), imsi);
    }
    PGW_LOG(PGW_LOG_LEVEL_INF, "exists :[%08x, %s, %s, %02x] \n",
            rec.sgw_gtpc_teid, rec.sgw_gtpc_ipv, rec.pgw_gtpc_ipv, rec.ebi);
    // create socket
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0){
        goto clean_up;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = 0;
    inet_pton(AF_INET, (char*)rec.pgw_gtpc_ipv, &addr.sin_addr.s_addr);
    // bind
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr))) {
        PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. bind (%d: %s)\n", errno, strerror(errno));
        goto clean_up;
    }

    // gtpc-v1
    if (rec.latest_gtp_version==GTPC_VERSION_1){
        // Delete PDP Context
        if (gtpc_v1_alloc_packet(&packet, GTPC_V1_MSG_DELETE_PDP_CONTEXT_REQ) != 0){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. gtpc_v1_alloc_packet (%d: %s)\n", errno, strerror(errno));
            goto clean_up;
        }
        // header (sequence number)
        seqno = htons(time(NULL));
        if(gtpc_v1_append_item(packet, 0, (U8 *) &seqno, sizeof(seqno)) != OK) { pgw_panic("failed. gtpc_v1_append_item.(seqno)"); }
        seqno = 0;
        if(gtpc_v1_append_item(packet, 0, (U8 *) &seqno, sizeof(seqno)) != OK) { pgw_panic("failed. gtpc_v1_append_item.(extend)"); }
        // teardown ind
        teard.type = GTPC_V1_IE_TEARDOWN_IND;
        teard.flags = 0xff;
        if(gtpc_v1_append_item(packet, 0, (U8 *) &teard, sizeof(teard)) != OK) { pgw_panic("failed. gtpc_v1_append_item.(teard)"); }

        // nsapi
        nsapi.type = GTPC_V1_IE_NSAPI;
        nsapi.nsapi_value = rec.ebi;
        if(gtpc_v1_append_item(packet, 0, (U8 *) &nsapi, sizeof(nsapi)) != OK) { pgw_panic("failed. gtpc_v1_append_item.(nsapi)"); }

        gtpc_v1_header_(packet)->tid = rec.sgw_gtpc_teid;

    }else{
        // generate Delete Bearer packet
        if (gtpc_alloc_packet(&packet, GTPC_DELETE_BEARER_REQ) != 0){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. gtpc_alloc_packet (%d: %s)\n", errno, strerror(errno));
            goto clean_up;
        }
        if (gtpc_ebi_set(&ebi, GTPC_INSTANCE_ORDER_0, rec.ebi) != OK){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. gtpc_ebi_set (%d: %s)\n", errno, strerror(errno));
            goto clean_up;
        }
        if (gtpc_append_item(packet, GTPC_TYPE_EBI, (U8*)&ebi, gtpc_ebi_length(&ebi)) != OK){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. gtpc_append_item (%d: %s)\n", errno, strerror(errno));
            goto clean_up;
        }
        gtpc_header_(packet)->t.teid         = rec.sgw_gtpc_teid;
        gtpc_header_(packet)->q.sq_t.seqno   = time(NULL);
    }
    // send
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(GTPC_PORT);
    inet_pton(AF_INET, (char*)rec.sgw_gtpc_ipv, &addr.sin_addr.s_addr);
    //
    if (pgw_send_sock(sock, &addr, sizeof(struct sockaddr_in), packet->packet, packet->offset) != OK){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. pgw_send_sock (%d: %s)\n", packet->length, strerror(errno));
        goto clean_up;
    }
    PGW_LOG(PGW_LOG_LEVEL_INF, "complete :[%08x, %s, %s, %02x] \n",
            rec.sgw_gtpc_teid, rec.sgw_gtpc_ipv, rec.pgw_gtpc_ipv, rec.ebi);

clean_up:
    if (packet){
        gtpc_free_packet(&packet);
    }
    if (sock != -1){
        close(sock);
    }
    mysql_close(dbh);
    closelog();
    //
    return(OK);
}
/**
  from imsi\n
 *******************************************************************************
  \n
  \n
 *******************************************************************************
 @param[in]       dph       database handle
 @param[in]       imsi      imsi number
 @param[in/out]   rec       record
 @return        0/success , !=0/error
 */
int  find_tunnel_by_imsi(MYSQL* dbh, U64 imsi, struct tunnel_record* rec) {
    int ret;
    my_ulonglong rown;
    MYSQL_RES  *res;
    MYSQL_ROW   row;
    char        sql[1024] = {0};
    // check, argument
    if (!rec){
        return(-1);
    }
    snprintf(sql, sizeof(sql) - 1, "SELECT sgw_gtpc_teid, sgw_gtpc_ipv, pgw_gtpc_ipv, ebi, latest_gtp_version FROM tunnel WHERE imsi = %llu", imsi);

    ret = mysql_query(dbh, sql);
    res = mysql_store_result(dbh);
    if (ret != 0 || !res) {
        return(-1);
    }
    rown = mysql_num_rows(res);
    if (rown != 1){
        mysql_free_result(res);
        return(-1);
    }
    row = mysql_fetch_row(res);
    if (row){
        rec->sgw_gtpc_teid = (U32)strtoul(row[0], NULL, 10);
        memcpy(rec->sgw_gtpc_ipv, row[1], MIN(sizeof(rec->sgw_gtpc_ipv), strlen(row[1])));
        memcpy(rec->pgw_gtpc_ipv, row[2], MIN(sizeof(rec->pgw_gtpc_ipv), strlen(row[2])));
        rec->ebi = (U8)atoi(row[3]);
        rec->latest_gtp_version = (U8)atoi(row[4]);
    }
    mysql_free_result(res);
    //
    return(row?0:-1);
}


/**
  read program arguments\n
 *******************************************************************************
  \n
  \n
 *******************************************************************************
 @param[in]       argc      count of arguments
 @param[in]       argv      arguments
 @param[in/out]   imsi      specific imsi
 @param[in/out]   timeout   timeout
 */
void read_args(int argc, char *argv[], U64 *imsi, U32* timeout){
    int         ch;
    // initialized log level
    PGW_LOG_LEVEL = PGW_LOG_LEVEL_ALL;
    if (!imsi || !timeout){
        usage(argv[0], "null pointer.");
        exit(0);
    }
    //
    while ( (ch = getopt(argc, argv, "i:t:h")) != -1) {
        switch(ch){
            case 'i': /* imsi */
                if (optarg != NULL){
                    (*imsi) = strtoull(optarg, NULL, 10);
                }
                break;
            case 'h':/* help */
                usage(argv[0] ,"help");
                exit(0);
                break;
            case 't':/* timeout */
                if (optarg != NULL){
                    (*timeout) = strtoul(optarg, NULL, 10);
                }
                break;
            default: /* unknown option */
                usage(argv[0] ,"Option %c is not defined.", ch);
                exit(0);
        }
    }
    if (!(*imsi) || !(*timeout)){
        usage(argv[0], "invalid args");
        exit(0);
    }
}
/**
 print usage\n
 *******************************************************************************
 \n
 *******************************************************************************
 */
void usage(const char* name, const char* fmt, ...){
    char msg[512] = {0};
    va_list ap;

    printf("============\n");
    printf("%s\n", name);
    printf("------------\n");
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg)-1,fmt, ap);
    vprintf(fmt, ap);
    va_end(ap);
    printf("\n============\n");
    printf("\t-i imsi `1234567890`\n");
    printf("\t-t timeout (sec)\n");
}
