#include "pgw_ext.h"
#include "node_ext.h"
#include "gtpc_ext.h"
#include "gtest/gtest.h"
#include "../src/pgw/pgw.h"

#include <typeinfo>
#include <string>
#include <map>
#include <mysql/mysql.h>

#define _TESTDB    ("test_event")
#include "test_cmn_inc.inl"

// instance
int PGW_LOG_LEVEL = PGW_LOG_LEVEL_ALL;
static unsigned __thread_cnt = 1;
static pthread_mutex_t  __mtx__;
static inline void atomic_u64_add(U64* u, U32 a){
    pthread_mutex_lock(&__mtx__);
    (*u) += (U64)a;
    pthread_mutex_unlock(&__mtx__);
}
static inline U64 atomic_u64_read(U64* u){
    U64 read = (U64)-1;
    pthread_mutex_lock(&__mtx__);
    read = (*u);
    pthread_mutex_unlock(&__mtx__);
    return(read);
}

pthread_mutex_t __mysql_mutex;


/*
 * test entry point
 */
int main(int argc, char* argv[]){
    testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new CustomEnvironment());

    if (argc > 1){ __thread_cnt  = (unsigned)atoi(argv[1]); }
    if (__thread_cnt == 0){
        __thread_cnt = 1;
    }
    pthread_mutex_init(&__mtx__, NULL);
    return RUN_ALL_TESTS();
}
/*
 * generate PGWinstance , release
 */
TEST(PgwInstenciateTest, MallocFree){
    handle_ptr    inst = NULL;

    EXPECT_EQ(pgw_create_instance(SERVER, &inst), OK);
    EXPECT_EQ(inst!=NULL,true);

    EXPECT_EQ(pgw_release_instance(&inst), OK);
    EXPECT_EQ(inst==NULL,true);
}

/*
 * set / get attribute , PGWinstance
 */
TEST(PgwInstanceOperationTest, SetGetProperty){
    handle_ptr    inst = NULL;

    EXPECT_EQ(pgw_create_instance(SERVER, &inst), OK);
    EXPECT_EQ(inst!=NULL,true);
    const char* TEST_TXT = "hogegege-%08d";


    for(int n = 0;n < 100;n++){
        PTR     getp = NULL;
        INT     getplen = 0;
        char    bf[128] = {0};

        snprintf(bf,sizeof(bf)-1,TEST_TXT, n);
        //
        EXPECT_EQ(pgw_set_property(inst, (n%10)+1, (void*)bf, strlen(bf)), OK);
        EXPECT_EQ(pgw_get_property(inst, (n%10)+1, &getp, &getplen), OK);

        EXPECT_EQ(getp!=NULL, true);
        if (getp != NULL){
            EXPECT_EQ(getplen, strlen(bf));
            EXPECT_EQ(memcmp(getp, bf, strlen(bf)), 0);
        }
    }
    EXPECT_EQ(pgw_release_instance(&inst), OK);
    EXPECT_EQ(inst==NULL,true);
}

static U64  error_count = 0;
static U64  deqburst_count = 0;

static const U32 DEBUG_DATALEN = 1024;
static const U32 TEST_COUNT = (128*88);

static RETCD __cb(
    EVENT event, handle_ptr pinst, evutil_socket_t sock ,
    short what, const U8* data, const INT datalen,
    struct sockaddr_in* saddr, ssize_t saddrlen,
    struct sockaddr_in* caddr, ssize_t caddrlen, void* ext){

    // count of burst
    packet_ptr  pkt[128] = {NULL};

    switch(event){
        case EVENT_ENTRY:
        {
            // since receive socket
            // until first software queue processing.
            RETCD nburst = pgw_deq_burst((node_ptr)ext, INGRESS, pkt, 32);
            if (nburst > 0){
                for(INT n = 0;n < nburst;n++){
                    //
                    if (pkt[n]->datalen != DEBUG_DATALEN){ atomic_u64_add(&error_count, 1); }
                    for(INT m = 0;m < pkt[n]->datalen;m++){
                        if (pkt[n]->data[m] != (U8)m){
                            atomic_u64_add(&error_count, 1);
                        }
                    }
                    // this test is evaluation of
                    // + compare payload
                    // + receive all packet
                    if (pgw_free_packet(pkt[n]) != OK){
                        PGW_LOG(PGW_LOG_LEVEL_ERR, "failed.pgw_free_packet(%p)\n", (void*)pthread_self());
                    }else{
                        atomic_u64_add(&deqburst_count, 1);
                    }
                }
                if ((atomic_u64_read(&deqburst_count)%7)== 0){
                    PGW_LOG(PGW_LOG_LEVEL_ERR, "node ingress.[%p].(%p)-- deq(%u) -- err(%u).\n", ext, (void*)pthread_self(), (unsigned)atomic_u64_read(&deqburst_count), (unsigned)atomic_u64_read(&error_count));
                }
            }
        }
        break;
        case EVENT_TIMER:
//          fprintf(stderr, "timer ...(%p)\n", (void*)pthread_self());
            break;
        case EVENT_POST_INIT:
            PGW_LOG(PGW_LOG_LEVEL_ERR, "node ipc post init...(%p)\n", (void*)pthread_self());
            break;
        case EVENT_GTPRX:
            PGW_LOG(PGW_LOG_LEVEL_ERR, "gtp[c/u]...(%p)\n", (void*)pthread_self());
            usleep(100000);
            break;
        default:
            PGW_LOG(PGW_LOG_LEVEL_ERR, "default...(%u:%p)\n", event, (void*)pthread_self());
            break;
    }
    usleep(10000);
    return(OK);
}

/*
 * start PGW
 * enqueue packet ->  dequeue
 * data sequence
 */
TEST(PgwPacketDataSeqTest, Enq2Deq){
    handle_ptr    inst = NULL;
    node_ptr    pnode = NULL;
    node_ptr    nodes[32] = {NULL};
    void* hoge = NULL;
    INT id = 0;
    U64 scounter = 0;

    EXPECT_EQ(pgw_create_instance(SERVER, &inst), OK);
    EXPECT_EQ(inst!=NULL,true);

    U32 val = __thread_cnt;
    EXPECT_EQ(pgw_set_property(inst, NODE_CNT, (PTR)&val, sizeof(val)), OK);
    EXPECT_EQ(pgw_set_property(inst, DB_HOST, (PTR)HOST, strlen(HOST)), OK);
    EXPECT_EQ(pgw_set_property(inst, DB_USER, (PTR)USER, strlen(USER)), OK);
    EXPECT_EQ(pgw_set_property(inst, DB_PSWD, (PTR)PSWD, strlen(PSWD)), OK);
    EXPECT_EQ(pgw_set_property(inst, DB_INST, (PTR)TESTDB, strlen(TESTDB)), OK);
    val = 3306;
    EXPECT_EQ(pgw_set_property(inst, DB_PORT, (PTR)&val, sizeof(val)), OK);
    val = 0;
    EXPECT_EQ(pgw_set_property(inst, DB_FLAG, (PTR)&val, sizeof(val)), OK);

    inst->server_gtpc = NULL;
    inst->server_gtpu = NULL;
    //
    EXPECT_EQ(pgw_start(inst, __cb, hoge), OK);
    // wait for all node started
    sleep(2);
    //
    for (id = 0,pnode = inst->node; pnode!=NULL; pnode = pnode->next) {
        printf("%u : %p\n", id, pnode);
        nodes[id++] = pnode;
    }

    U32 n;
    // identical processing with gtpc packet receive
    // distribute count([ids]) nodes.
    for(n = 0;n < TEST_COUNT;n++){
        packet_ptr  pkt = NULL;
        EXPECT_EQ(pgw_alloc_packet(&pkt, DEBUG_DATALEN), OK);
        EXPECT_EQ(pkt!=NULL,true);
        for(U32 m = 0; m < DEBUG_DATALEN;m++){
            pkt->data[m] = (U8)m;
        }
        if (n%239==0){
            printf("node[%p : %u] %u\n", (void*)pthread_self(), (n%id),n);
        }
        usleep(100);
        EXPECT_EQ(pgw_enq(nodes[(n%id)], INGRESS, pkt), OK);
    }

    PGW_LOG(PGW_LOG_LEVEL_ERR, "node enqueue finished.[%u] %u\n", (n%id),n);

    sleep(1);
    while(1){
        usleep(10000);
        printf(".");
        scounter++;
        if (scounter%64 == 0){
            printf("waiting ..(%u - %u)\n", (unsigned)atomic_u64_read(&error_count), (unsigned)atomic_u64_read(&deqburst_count));
            if (atomic_u64_read(&error_count)){
                printf("error_counted. (%u)\n", (unsigned)atomic_u64_read(&error_count));
                break;
            }
            if (atomic_u64_read(&deqburst_count) >= TEST_COUNT){ break; }
        }
    }
    PGW_LOG(PGW_LOG_LEVEL_ERR, "main context...(%p)-- deq(%u) -- err(%u).\n", (void*)pthread_self(), (unsigned)atomic_u64_read(&deqburst_count), (unsigned)atomic_u64_read(&error_count));

    // wait for all node stopped.
    EXPECT_EQ(pgw_stop(inst, NULL, NULL), OK);
    EXPECT_EQ(pgw_release_instance(&inst), OK);
    EXPECT_EQ(inst==NULL,true);
    EXPECT_EQ(error_count, 0);
}
/*
 * no memory leaks
 */
TEST(MemoryLeakTest, IsLeakMemory_0){
    EXPECT_EQ(gtpc_memory_print(), OK);
}

/*
 * validate GTPC header
 */
TEST(GtpcHeaderTest, Validate){
    packet_ptr  pkt = NULL;
    EXPECT_EQ(pgw_alloc_packet(&pkt, 2048), OK);

    //
    gtpc_header_ptr gtpch = gtpc_header(pkt);
    EXPECT_EQ(gtpch!=NULL, true);
    // valid header
    gtpch->length = htons(512);
    gtpch->v2_flags.version = GTPC_VERSION_2;
    gtpch->v2_flags.piggy = GTPC_PIGGY_OFF;
    gtpch->type = GTPC_ECHO_REQ;
    EXPECT_EQ(gtpc_validate_header(pkt), OK);

    // invalid version
    gtpch->length = htons(256);
    gtpch->v2_flags.version = GTPC_VERSION_1;
    gtpch->v2_flags.piggy = GTPC_PIGGY_OFF;
    gtpch->type = GTPC_SUSPEND_NOTIFICATION;
    EXPECT_EQ(gtpc_validate_header(pkt), OK);

    // invalid flags
    gtpch->length = htons(512);
    gtpch->v2_flags.version = GTPC_VERSION_2;
    gtpch->v2_flags.piggy = GTPC_PIGGY_ON;
    gtpch->type = GTPC_ECHO_REQ;
    EXPECT_NE(gtpc_validate_header(pkt), OK);

    // invalid type
    gtpch->length = htons(128);
    gtpch->v2_flags.version = GTPC_VERSION_2;
    gtpch->v2_flags.piggy = GTPC_PIGGY_OFF;
    gtpch->type = (GTPC_RESUME_ACK+10);
    EXPECT_NE(gtpc_validate_header(pkt), OK);

    // invalid length of headerß
    gtpch->length = htons(1);
    gtpch->v2_flags.version = GTPC_VERSION_2;
    gtpch->v2_flags.piggy = GTPC_PIGGY_OFF;
    gtpch->type = GTPC_RESUME_ACK;
    EXPECT_NE(gtpc_validate_header(pkt), OK);


    EXPECT_EQ(pgw_free_packet(pkt), OK);
}
/*
 * no memory leaks
 */
TEST(MemoryLeakTest, IsLeakMemory){
    EXPECT_EQ(gtpc_memory_print(), OK);
}


static RETCD on_rec__(U32 counter, U32 clmncnt, colmn_ptr col, record_ptr rec, void* udat);
TEST(DatabaseOperationTest, Select){
    my_bool     reconnect = 1;

    DBPROVIDER_THREAD_INIT();
    auto dbh = DBPROVIDER_INIT(NULL);
    ASSERT_EQ(dbh==NULL,false);
    if (dbh == NULL){
        return;
    }
    DBPROVIDER_OPTIONS(dbh, MYSQL_OPT_RECONNECT, &reconnect);

    ASSERT_EQ(DBPROVIDER_REAL_CONNECT(dbh,
                           (TXT)HOST,
                           (TXT)USER,
                           (TXT)PSWD,
                           (TXT)TESTDB,
                           3306,
                           NULL,
                           0)==0,false);

    PGW_LOG(PGW_LOG_LEVEL_ERR, "...Select ... %s(%p : %p).", TESTDB,  dbh, (void*)pthread_self());

    char        sql[2048] = {0};
    DBPROVIDER_ST_BIND_I    bind;

    std::string ssql = "INSERT INTO keepalive (dst_ip, dst_port,src_ip, src_port, proto, stat, active, server_id, server_type, updated_at)VALUES";

    for(int n = 0;n < 1000; n++){
        bzero(sql,sizeof(sql));
        snprintf(sql, sizeof(sql)-1,
                 "%s('%u.%u.%u.%u',1234,'127.0.0.1',4321,1,0,1,'test',1,%s)",
            n==0?" ":",",
            (n&0xff), ((n+1)&0xff),((n+2)&0xff), ((n+3)&0xff),
#ifndef __USESQLITE3_ON_TEST__
            "DATE_ADD(NOW(),INTERVAL -100 SECOND)"
#else
            "STRFTIME('%Y-%m-%d %H:%M:%S',CURRENT_TIMESTAMP,'-100 seconds')"
#endif
        );
        ssql += sql;
    }

    EXPECT_EQ(DBPROVIDER_QUERY(dbh, ssql.c_str()), 0);
    PGW_LOG(PGW_LOG_LEVEL_ERR, "%s(%p)", ssql.c_str(),  (void*)pthread_self());


    snprintf(sql, sizeof(sql)-1, KEEPALIVE_SQL, "127.0.0.1", "test");
    auto stmt = DBPROVIDER_STMT_INIT(dbh);
    EXPECT_EQ(stmt == NULL, false);
    if (!stmt){
        return;
    }
    EXPECT_EQ(DBPROVIDER_STMT_PREPARE(stmt, sql, strlen(sql)), 0);
    EXPECT_EQ(DBPROVIDER_BIND_INIT(&bind), 0);

    for(int n = 0;n < 100;n++){
        // 100K
        pthread_mutex_lock(&__mysql_mutex);
        EXPECT_EQ(DBPROVIDER_EXECUTE(stmt, bind.bind, on_rec__, NULL), 0);
        pthread_mutex_unlock(&__mysql_mutex);
    }
    PGW_LOG(PGW_LOG_LEVEL_ERR, "%s(%p)", sql,  (void*)pthread_self());


    PGW_LOG(PGW_LOG_LEVEL_ERR, "...stmt ..... %s(%p : %p).", TESTDB,  dbh, (void*)pthread_self());

    pthread_mutex_lock(&__mysql_mutex);
    EXPECT_EQ(DBPROVIDER_STMT_CLOSE(stmt), 0);
    DBPROVIDER_CLOSE(dbh);
    pthread_mutex_unlock(&__mysql_mutex);
}


RETCD on_rec__(U32 counter, U32 clmncnt, colmn_ptr col, record_ptr rec, void* udat){
    if (counter % 23 == 0){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "%u(%p)", counter,  (void*)pthread_self());
    }
    return(OK);
}


static int __ext_num = 0;
static int __pgw_num = 0;

void read_args(int argc, char *argv[], char* ipv4, unsigned ipv4_len, char* nic, unsigned nic_len ,char* sid, unsigned sid_len){
    int         ch;
    // log level
    PGW_LOG_LEVEL = PGW_LOG_LEVEL_ALL;

    // bind default value
    strncpy(sid,  SERVERID, MIN(strlen(SERVERID), sid_len));
    strncpy(nic,  SRCNIC,   MIN(strlen(SRCNIC), nic_len));
    strncpy(ipv4, SRCADDRESS, MIN(strlen(SRCADDRESS), ipv4_len));
    // read options , bind ip, NIC, server_id
    while ( (ch = getopt(argc, argv, "p:dhb:i:s:l:X:Y:")) != -1) {
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
                break;
            case 'd':/* daemon? */
                break;
            case 'p': /* pid */
                if (optarg != NULL){
                }
                break;
            case 'X': /* ext set number */
                if (optarg != NULL){
                    printf("ext set number(%d)\n", atoi(optarg));
                    __ext_num = atoi(optarg);
                }
                break;
            case 'Y': /* pgw unit number */
                if (optarg != NULL){
                    printf("pgw unit number(%d)\n", atoi(optarg));
                    __pgw_num = atoi(optarg);
                }
                break;
            default: /* unknown option */
                printf("Option %c is not defined.", ch);
        }
    }
}

#define ARG_CNT (16)
TEST(ReadArgumentTest, Argument){
    char        bind_ipv4[MAX_ADDR] = {0};
    char        bind_nic[32] = {0};
    char        server_id[32] = {0};
    char        *argv[ARG_CNT];
    //
    for(auto n = 0;n < ARG_CNT;n++){
        argv[n] = (char*)malloc(128);
    }

    strcpy(argv[0], "test");
    strcpy(argv[1], "-X");
    strcpy(argv[2], "1");
    strcpy(argv[3], "-i");
    strcpy(argv[4], "12.13.14.15");
    //
    read_args(5, (char**)argv, bind_ipv4, sizeof(bind_ipv4)-1, bind_nic, sizeof(bind_nic)-1,server_id, sizeof(server_id)-1);
    //
    EXPECT_EQ(strcmp(bind_ipv4, "12.13.14.15"),0);
    EXPECT_EQ(__ext_num, 1);
    EXPECT_EQ(__pgw_num, 0);
    for(auto n = 0;n < ARG_CNT;n++){
        free(argv[n]);
    }
}
static void _check_version(DBPROVIDER_HANDLE dbh, const char* sql, U32 version){
    auto ret = DBPROVIDER_QUERY(dbh, sql);
    EXPECT_EQ(ret,0);
    auto res = DBPROVIDER_STORE_RESULT(dbh);
    EXPECT_EQ(res != NULL,true);
    if (res){
        auto rown = DBPROVIDER_NUM_ROWS(res);
        EXPECT_EQ(rown, 1);
        if (rown == 1){
            auto row =  DBPROVIDER_FETCH_ROW(res);
            EXPECT_EQ(row!=NULL,true);
            if (row){
                EXPECT_EQ((U32)atoi(row[0]), version);
            }
        }
        DBPROVIDER_FREE_RESULT(res);
    }

}
TEST(SqlDefinedTest, ChangeDdlLatestGtpVersion){
    auto dbh = DBPROVIDER_INIT(NULL);
    char sql[1024] = {0};
    EXPECT_EQ(dbh==NULL,false);
    if (!dbh){
        return;
    }
    EXPECT_EQ(DBPROVIDER_REAL_CONNECT(dbh,
                                 (TXT)HOST,
                                 (TXT)USER,
                                 (TXT)PSWD,
                                 (TXT)"",
                                 3306,
                                 NULL,
                                 0)==0,false);
    DBPROVIDER_SET_CHAR(dbh, "utf8");
    snprintf(sql, sizeof(sql)-1,"USE %s", TESTDB);
    EXPECT_EQ(DBPROVIDER_QUERY(dbh, sql), 0);
    EXPECT_EQ(DBPROVIDER_QUERY(dbh, "DELETE FROM tunnel WHERE msisdn = 819033333333 OR pgw_teid = 0xdeadbeaf"), 0);
    auto ret = DBPROVIDER_QUERY(dbh,"INSERT INTO tunnel(imsi,msisdn,ueipv4,pgw_teid,sgw_gtpu_teid,sgw_gtpu_ipv,active)VALUES "\
                               "(240679033333333,819033333333,'5.6.7.8',0xdeadbeaf,0xdeadbeaf,'1.2.3.4',1)");
    EXPECT_EQ(ret, 0);

#define GO_VER1()   {\
    snprintf(sql, sizeof(sql)-1, CREATE_PDP_CONTEXT_UPD_SQL, 1,"1.1.1.1",2,"2.2.2.2",3,4,0xdeadbeaf,240679033333333ULL,240679033333333ULL);\
    ret = DBPROVIDER_QUERY(dbh, sql);\
    EXPECT_EQ(ret, 0);\
}

    // must be version 1
    GO_VER1();
    _check_version(dbh, "SELECT latest_gtp_version FROM tunnel WHERE imsi = 240679033333333", 1);

    // change to version 2
    snprintf(sql, sizeof(sql)-1, CREATE_SESSION_UPD_SQL, 1,"1.1.1.1",2,"2.2.2.2",3,4,0xdeadbeaf,240679033333333ULL);
    ret = DBPROVIDER_QUERY(dbh, sql);
    EXPECT_EQ(ret, 0);
    _check_version(dbh, "SELECT latest_gtp_version FROM tunnel WHERE imsi = 240679033333333", 2);

    // must be version 1
    snprintf(sql, sizeof(sql)-1, UPDATE_PDP_CONTEXT_UPD_SQL, 1,"1.1.1.1",2,"2.2.2.2",240679033333333ULL);
    ret = DBPROVIDER_QUERY(dbh, sql);
    EXPECT_EQ(ret, 0);
    _check_version(dbh, "SELECT latest_gtp_version FROM tunnel WHERE imsi = 240679033333333", 1);

    // change to version 2
    snprintf(sql, sizeof(sql)-1, MODIFY_BEARER_RATONLY_UPD_SQL, 6,240679033333333ULL);
    ret = DBPROVIDER_QUERY(dbh, sql);
    EXPECT_EQ(ret, 0);
    _check_version(dbh, "SELECT latest_gtp_version FROM tunnel WHERE imsi = 240679033333333", 2);

    // must be version 1
    GO_VER1();

    // change to version 2
    snprintf(sql, sizeof(sql)-1, MODIFY_BEARER_RAT_UPD_SQL, 1,"",2,"",0xdeadbeaf,0,240679033333333ULL);
    ret = DBPROVIDER_QUERY(dbh, sql);
    EXPECT_EQ(ret, 0);
    _check_version(dbh, "SELECT latest_gtp_version FROM tunnel WHERE imsi = 240679033333333", 2);

    // must be version 1
    GO_VER1();
    // change to version 2
    snprintf(sql, sizeof(sql)-1, MODIFY_BEARER_UPD_SQL, 1,"",2,"",0xdeadbeaf,240679033333333ULL);
    ret = DBPROVIDER_QUERY(dbh, sql);
    EXPECT_EQ(ret, 0);
    _check_version(dbh, "SELECT latest_gtp_version FROM tunnel WHERE imsi = 240679033333333", 2);

    // must be version 1
    GO_VER1();
    // change to version 2
    snprintf(sql, sizeof(sql)-1, SUSPEND_NOTIFY_UPD_SQL, 0xdeadbeaf);
    ret = DBPROVIDER_QUERY(dbh, sql);
    EXPECT_EQ(ret, 0);
    _check_version(dbh, "SELECT latest_gtp_version FROM tunnel WHERE imsi = 240679033333333", 2);

    // must be version 1
    GO_VER1();
    // change to version 2
    snprintf(sql, sizeof(sql)-1, RESUME_NOTIFY_UPD_SQL, 240679033333333ULL);
    ret = DBPROVIDER_QUERY(dbh, sql);
    EXPECT_EQ(ret, 0);
    _check_version(dbh, "SELECT latest_gtp_version FROM tunnel WHERE imsi = 240679033333333", 2);


    DBPROVIDER_CLOSE(dbh);
}
