#include "pgw_ext.h"
#include "node_ext.h"
#include "gtpc_ext.h"
#include "../src/pgw/pgw.h"
#include "gtest/gtest.h"

// instance
int PGW_LOG_LEVEL = PGW_LOG_LEVEL_ALL;
pthread_mutex_t __mysql_mutex;

#define _TESTDB    ("test_fix_any")

#include "test_cmn_inc.inl"

static handle_ptr  __inst = NULL;
static server_ptr  __srvr = NULL;

static RETCD event_cb(EVENT , handle_ptr , evutil_socket_t , short , const U8* , const INT ,  struct sockaddr_in*, ssize_t , struct sockaddr_in* ,ssize_t , void* );

//
int main(int argc, char* argv[]){
    testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new CustomEnvironment());
    return RUN_ALL_TESTS();
}
//
TEST(IntegrationTestBugfix, PgwInstanceStart){
    U32 nodecnt = PGW_NODE_COUNT;
    U32 port = 3306;
    U32 opt = 0;
    EXPECT_EQ(pgw_create_instance(SERVER, &__inst), OK);
    EXPECT_EQ(
            pgw_set_property(__inst, NODE_CNT, (PTR)&nodecnt, sizeof(nodecnt)) ||
            pgw_set_property(__inst, DB_HOST, (PTR)HOST, strlen(HOST)) ||
            pgw_set_property(__inst, DB_USER, (PTR)USER, strlen(USER)) ||
            pgw_set_property(__inst, DB_PSWD, (PTR)PSWD, strlen(PSWD)) ||
            pgw_set_property(__inst, DB_INST, (PTR)TESTDB, strlen(TESTDB)) ||
            pgw_set_property(__inst, DB_PORT, (PTR)&port, sizeof(port)) ||
            pgw_set_property(__inst, DB_FLAG, (PTR)&opt, sizeof(opt)), OK);

    __inst->server_gtpc = NULL;
    __inst->server_gtpu = NULL;

    __inst->ext_set_num = (U8)0x0f;
    __inst->pgw_unit_num = (U8)0x0e;


    EXPECT_EQ(pgw_start(__inst, event_cb, NULL), OK);
    // wait for all threas started.
    while(1){
        usleep(100000);
        if ((__inst->flags&ON)==ON){
            break;
        }
    }
    // inject tunnel record for debug
    DBPROVIDER_THREAD_INIT();
    auto dbh = DBPROVIDER_INIT(NULL);
    EXPECT_EQ(dbh==NULL,false);
    if (!dbh){
        return;
    }
    EXPECT_EQ(DBPROVIDER_REAL_CONNECT(dbh,
                           (TXT)HOST,
                           (TXT)USER,
                           (TXT)PSWD,
                           (TXT)TESTDB,
                           port,
                           NULL,
                           opt)==0,false);
    DBPROVIDER_SET_CHAR(dbh, "utf8");
    auto ret = DBPROVIDER_QUERY(dbh, "DELETE FROM tunnel WHERE msisdn = 819099999999");
    EXPECT_EQ(ret, 0);

#ifndef __USESQLITE3_ON_TEST__
    ret = DBPROVIDER_QUERY(dbh,
        "INSERT INTO tunnel(imsi,msisdn,ueipv4,pgw_teid,sgw_gtpu_teid,sgw_gtpu_ipv,active,sgw_gtpc_teid)VALUES "\
        "(240679099999999,819099999999,'0.6.7.8',0x00000006,0xdeadc0de,'0.2.3.4',1,91827364) ON DUPLICATE KEY UPDATE active=VALUES(active)");
#else
    ret = DBPROVIDER_QUERY(dbh,
        "INSERT OR IGNORE INTO tunnel(imsi,msisdn,ueipv4,pgw_teid,sgw_gtpu_teid,sgw_gtpu_ipv,active,sgw_gtpc_teid)VALUES "\
        "(240679099999999,819099999999,'0.6.7.8',0x00000006,0xdeadc0de,'0.2.3.4',1,91827364) ");
#endif
    EXPECT_EQ(ret, 0);
    DBPROVIDER_CLOSE(dbh);
}

TEST(IntegrationTestBugfix, ModifyBeare){
    U8  tpkt[] = {
        0x48,0x22,0x00,0x3E,
        0x00,0x00,0x00,0x06,0x00,0x00,0x01,0x00,
        0x53,0x00,0x03,0x00,0x42,0xf0,0x01,
        0x52,0x00,0x01,0x00,0x06,
        0x4d,0x00,0x03,0x00,0x00,0x00,0x00,
        0x57,0x00,0x09,0x00,0x86,0x00,0x0f,0x42,0x40,0x0a,0x04,0x02,0x0c,
        0x5d,0x00,0x12,0x00,
        0x49,0x00,0x01,0x00,0x08,
        0x57,0x00,0x09,0x01,0x84,0x00,0x1e,0x84,0x80,0x0a,0x04,0x02,0x0c,
    };
    struct sockaddr_in  saddr, caddr;
    server_t    srv;
    bzero(&saddr, sizeof(saddr));
    bzero(&caddr, sizeof(caddr));
    bzero(&srv,   sizeof(srv));
    srv.port = GTPC_PORT;

    packet_ptr  pkts[32] = {NULL};
    auto nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPC_MODIFY_BEARER_REQ], INGRESS, pkts, 32);
    //
    EXPECT_EQ(nburst, 0);
    EXPECT_EQ(event_rx(__inst, tpkt, sizeof(tpkt),  &saddr, sizeof(saddr), &caddr, sizeof(caddr), &srv), OK);

    EXPECT_EQ(event_gtpc_modify_bearer_req(__inst, __inst->nodes[PGW_NODE_GTPC_MODIFY_BEARER_REQ]), OK);
    nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPC_MODIFY_BEARER_REQ], EGRESS, pkts, 32);
    EXPECT_EQ(nburst, 1);
    struct gtpc_parse_state    parse_state;
    bzero(&parse_state, sizeof(parse_state));
    gtp_packet_ptr  parsed_pkt = NULL;

    EXPECT_EQ(gtpc_alloc_packet_from_payload(&parsed_pkt,(U8*)pkts[0]->data, pkts[0]->datalen), OK);
    // debugging: setup response-side parse flags
    parse_state.flag |= HAVE_RES_ORDER;
    gtpc_iterate_item(parsed_pkt, on_gtpc_parse_root, &parse_state);
    EXPECT_EQ((parse_state.flag&HAVE_CAUSE), HAVE_CAUSE);
    EXPECT_EQ(parse_state.cause.cause, GTPC_CAUSE_REQUEST_ACCEPTED);
    EXPECT_EQ(gtpc_free_packet(&parsed_pkt), OK);

    std::string debug_log = "\">>>>> xdpdk";

    for(auto n = 0;n < pkts[0]->datalen;n++){
        char bf[16] = {0};
        if (n%16 == 0){
            snprintf(bf, sizeof(bf) - 1,"\n%04u  ", (n/16));
            debug_log += bf;
        }
        bzero(bf,sizeof(bf));
        snprintf(bf, sizeof(bf)-1, " %02x", pkts[0]->data[n]);
        debug_log += bf;
    }

    PGW_LOG(PGW_LOG_LEVEL_ERR, "%s", debug_log.c_str());

    PGW_LOG(PGW_LOG_LEVEL_ERR, "%s",
     ">>>>> appliance pgw\n"\
     "0000   48 23 00 40 00 0f 42 40 00 00 01 00 02 00 02 00\n"\
     "0010   10 00 4c 00 06 00 18 09 21 43 65 87 4e 00 08 00\n"\
     "0020   00 00 0d 04 08 08 08 08 5d 00 13 00 49 00 01 00\n"\
     "0030   08 02 00 02 00 10 00 5e 00 04 00 00 00 00 07 03\n"\
     "0040   00 01 00 01\n");

    // DB Rat type must be 6=EUTRAN.
    // comfirm ebi in database
    char sql[1024] = {0};
    U32  rec_isexists = 0;
    dbbind_t    bind;
    snprintf(sql, sizeof(sql) -1 ,"SELECT rat,ebi FROM tunnel WHERE msisdn=819099999999");
    auto dbh = DBPROVIDER_INIT(NULL);
    EXPECT_EQ(dbh==NULL,false);
    if (!dbh){ return; }
    EXPECT_EQ(DBPROVIDER_REAL_CONNECT(dbh,
                                 (TXT)HOST,
                                 (TXT)USER,
                                 (TXT)PSWD,
                                 (TXT)TESTDB,
                                 3306,
                                 NULL,
                                 0)==0,false);

    EXPECT_EQ(DBPROVIDER_QUERY(dbh, sql), 0);
    auto res = DBPROVIDER_STORE_RESULT(dbh);
    EXPECT_EQ(res!=NULL,true);
    auto rown = DBPROVIDER_NUM_ROWS(res);
    EXPECT_EQ(rown, 1);
    if (rown == 1){
        auto row = DBPROVIDER_FETCH_ROW(res);
        EXPECT_EQ(row!=NULL,true);
        EXPECT_EQ(atoi(row[0]), int(6));
        EXPECT_EQ(atoi(row[1]), int(8));
    }
    DBPROVIDER_FREE_RESULT(res);
    DBPROVIDER_CLOSE(dbh);

#if 1
    memset(&caddr, 0, sizeof(caddr));
    caddr.sin_family = AF_INET;
    caddr.sin_port = htons(2123);
    inet_pton(AF_INET, "8.8.8.8", &caddr.sin_addr.s_addr);

    auto sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    auto ret = pgw_send_sock(sock, &caddr, sizeof(struct sockaddr_in), tpkt, sizeof(tpkt));
    ret = pgw_send_sock(sock, &caddr, sizeof(struct sockaddr_in), pkts[0]->data, pkts[0]->datalen);
    close(sock);
#endif
    //
    for(auto n = 0;n < nburst;n++){
        pgw_free_packet(pkts[n]);
    }
}


TEST(IntegrationTestBugfix, CreateSession){
    U8  tpkt[] = {
        0x48,0x20,0x01,0x1a,0x00,0x00,0x00,0x00,0x17,0x6d,0x05,0x00,
        0x01,0x00,0x08,0x00,0x42,0x60,0x97,0x70,    0x77,0x77,0x77,0xf7,    // imsi
        0x03,0x00,0x01,0x00,0x03,                                           // recovery

        0x47,0x00,0x1d,0x00,0x05,0x72,0x61,0x74,0x65,0x6c,0x03,0x63,0x6f,0x6d,0x06,0x6d,
        0x6e,0x63,0x30,0x31,0x30,0x06,0x6d,0x63,0x63,0x34,0x34,0x30,0x04,0x67,0x70,0x72,
        0x73,                                                               // apn

        0x48,0x00,0x08,0x00,0x00,0x41,0x89,0x37,0x00,0x41,0x89,0x37,        // ambr
        0x4b,0x00,0x08,0x00,0x53,0x72,0x22,0x70,0x96,0x74,0x16,0x10,        // mei
        0x4c,0x00,0x06,0x00,0x88,0x88,0x88,0x88,0x88,0x88,                  // msisdn => 888888888888
        0x4d,0x00,0x04,0x00,0x00,0x00,0x00,0x00,                            // indication

        0x4e,0x00,0x3a,0x00,0x80,0x80,0x21,0x10,0x01,0x00,0x00,0x10,0x81,0x06,0x00,0x00,
        0x00,0x00,0x83,0x06,0x00,0x00,0x00,0x00,0x00,0x0d,0x00,0x00,0x0a,0x00,0x00,0x05,
        0x00,0xc0,0x23,0x1a,0x01,0x00,0x00,0x1a,0x0f,0x72,0x61,0x74,0x65,0x6c,0x40,0x72,
        0x61,0x74,0x65,0x6c,0x2e,0x63,0x6f,0x6d,0x05,0x72,0x61,0x74,0x65,0x6c,
        // pco
        0x4f,0x00,0x05,0x00,0x01,0x00,0x00,0x00,0x00,                       // paa
        0x52,0x00,0x01,0x00,0x06,                                           // rat
        0x53,0x00,0x03,0x00,0x42,0xf0,0x01,                                 // saving network
        0x56,0x00,0x0d,0x00,0x18,0x42,0xf0,0x01,0x14,0x0c,0x42,0xf0,0x01,0x02,0x15,0x2c,
        0x00,                                                               // user location info
        0x57,0x00,0x09,0x00,0x86,0x00,0x12,0xf5,0x5a,0x31,0x67,0x0a,0x34,   // fteid
        0x5d,0x00,0x2c,0x00,0x49,0x00,0x01,0x00,0x05,0x50,0x00,0x16,0x00,0x7c,0x09,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x57,0x00,0x09,0x02,0x84,0x40,0x76,0x41,0x20,0x31,0x67,0x0a,0x0c,
        // bearer context
        0x63,0x00,0x01,0x00,0x01,                                            // pdn type
        0x7f,0x00,0x01,0x00,0x00,                                            // apn restriction
        0x80,0x00,0x01,0x00,0x01,                                            // selection mode
        0x72,0x00,0x02,0x00,0x63,0x00                                        // ue time zone
    };
    struct sockaddr_in  saddr, caddr;
    server_t    srv;
    packet_ptr  pkts[32] = {NULL};
    gtp_packet_ptr  parsed_pkt = NULL;
    struct gtpc_parse_state    parse_state;
    //
    bzero(&saddr, sizeof(saddr));
    bzero(&caddr, sizeof(caddr));
    bzero(&srv,   sizeof(srv));
    srv.port = GTPC_PORT;
    strncpy((char*)srv.ip, "254.254.254.254", MIN(sizeof(srv.ip)-1,15));
    //
    auto dbh = DBPROVIDER_INIT(NULL);
    EXPECT_EQ(dbh==NULL,false);
    if (!dbh){
        return;
    }
    EXPECT_EQ(DBPROVIDER_REAL_CONNECT(dbh,
                                 (TXT)HOST,
                                 (TXT)USER,
                                 (TXT)PSWD,
                                 (TXT)TESTDB,
                                 3306,
                                 NULL,
                                 0)==0,false);
    DBPROVIDER_SET_CHAR(dbh, "utf8");
    auto ret = DBPROVIDER_QUERY(dbh, "DELETE FROM tunnel WHERE imsi = 240679077777777");
    EXPECT_EQ(ret, 0);
    ret = DBPROVIDER_QUERY(dbh,
                      "INSERT INTO tunnel(imsi, msisdn, ueipv4, pgw_teid, sgw_gtpu_teid, sgw_gtpu_ipv, active,pgw_gtpc_ipv,pgw_gtpu_ipv)VALUES "\
                      "(240679077777777,0,'0.6.7.8',0xdead2345,0xdead3456,'192.168.71.69',1,'','')");
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(event_rx(__inst, tpkt, sizeof(tpkt),  &saddr, sizeof(saddr), &caddr, sizeof(caddr), &srv), OK);
    __inst->server_gtpc = &srv;
    EXPECT_EQ(event_gtpc_create_session_req(__inst, __inst->nodes[PGW_NODE_GTPC_CREATE_SESSION_REQ_EXT0]), OK);
    __inst->server_gtpc = NULL;
    auto nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPC_CREATE_SESSION_REQ_EXT0], EGRESS, pkts, 32);
    EXPECT_EQ(nburst, 1);
    if (nburst != 1){
        return;
    }
    //
    bzero(&parse_state, sizeof(parse_state));

    EXPECT_EQ(gtpc_alloc_packet_from_payload(&parsed_pkt,(U8*)pkts[0]->data, pkts[0]->datalen), OK);
    // debugging: setup response-side parse flags
    parse_state.flag |= HAVE_RES_ORDER;
    gtpc_iterate_item(parsed_pkt, on_gtpc_parse_root, &parse_state);
    EXPECT_EQ((parse_state.flag&HAVE_CAUSE), HAVE_CAUSE);
    EXPECT_EQ((parse_state.flag&HAVE_PCO), HAVE_PCO);
    EXPECT_EQ(parse_state.cause.cause, GTPC_CAUSE_REQUEST_ACCEPTED);
    EXPECT_EQ((parse_state.flag&HAVE_RECOVERY), HAVE_RECOVERY);
    EXPECT_EQ(parse_state.recovery.recovery_restart_counter, PGW_RECOVERY_COUNT);
    EXPECT_EQ(gtpc_free_packet(&parsed_pkt), OK);

    // pgw_gtp[c/u]_ipv must be updated.
    ret = DBPROVIDER_QUERY(dbh, "SELECT pgw_gtpc_ipv, pgw_gtpu_ipv FROM tunnel WHERE imsi = 240679077777777");
    auto res = DBPROVIDER_STORE_RESULT(dbh);
    EXPECT_EQ(res != NULL,true);
    if (res){
        auto rown = DBPROVIDER_NUM_ROWS(res);
        EXPECT_EQ(rown, 1);
        if (rown == 1){
            auto row = DBPROVIDER_FETCH_ROW(res);
            EXPECT_EQ(row!=NULL,true);
            if (row){
                EXPECT_EQ(strcmp(row[1], "254.254.254.254"), 0);
                EXPECT_EQ(strcmp(row[0], "254.254.254.254"), 0);
            }
        }
        DBPROVIDER_FREE_RESULT(res);
    }
    for(auto n = 0;n < nburst;n++){
        pgw_free_packet(pkts[n]);
    }
    DBPROVIDER_CLOSE(dbh);
}

TEST(IntegrationTestBugfix, ModifyBearer){
    U8  tpkt[] = {
        0x48,0x22,0x00,0x6b,    // version 2
        0xde,0xad,0x23,0x45,    // teid 0xdead2345
        0x09,0xcb,0xe5,0x00,    // sequence
        0x56,0x00,0x0d,0x00,    // ULI
        0x18,0x42,0xf0,0x02,0x18,0x57,0x42,0xf0,0x02,0x00,0x9d,0x1c,0x03,
        0x53,0x00,0x03,0x00,0x42,0xf0,0x02, // Serving Network
        0x52,0x00,0x01,0x00,0x06,           // RAT Type
        0x57,0x00,0x09,0x00,0x86,
        0xbe,0x5b,0xa1,0xd0,    // F-TEID(gtp-c) new TEID 0xbe5ba1d0
        0xca,0xb3,0xcd,0x71,    // F-TEID(gpt-c)
        0x48,0x00,0x08,0x00,0x00,0x00,0x01,0x80,0x00,0x00,0xa4,0x10,        // AMBR
        0x4b,0x00,0x08,0x00,0x53,0x75,0x03,0x70,0x16,0x56,0x13,0x22,        // MEI
        0x72,0x00,0x02,0x00,0x63,0x00,  // UE Time Zone
        0x5d,0x00,0x12,0x00,
        0x49,0x00,0x01,0x00,0x05,       // Bearer Context - EBI 5
        0x57,0x00,0x09,
        0x01,0x84,
        0xbe,0x50,0x8b,0xe7,            // Bearer Context - F-TEID(gtp-u) new TEID 0xbe508be7
        0xca,0xb3,0xcd,0x81,                  // Bearer Context
        0x03,0x00,0x01,0x00,0x08,       // Recovery
    };
    struct sockaddr_in  saddr, caddr;
    server_t    srv;
    packet_ptr  pkts[32] = {NULL};
    gtp_packet_ptr  parsed_pkt = NULL;
    struct gtpc_parse_state    parse_state;
    //
    bzero(&saddr, sizeof(saddr));
    bzero(&caddr, sizeof(caddr));
    bzero(&srv,   sizeof(srv));
    srv.port = GTPC_PORT;
    strncpy((char*)srv.ip, "254.254.254.254", MIN(sizeof(srv.ip)-1,15));
    //
    auto dbh = DBPROVIDER_INIT(NULL);
    EXPECT_EQ(dbh==NULL,false);
    if (!dbh){
        return;
    }
    EXPECT_EQ(DBPROVIDER_REAL_CONNECT(dbh,
                                 (TXT)HOST,
                                 (TXT)USER,
                                 (TXT)PSWD,
                                 (TXT)TESTDB,
                                 3306,
                                 NULL,
                                 0)==0,false);
    DBPROVIDER_SET_CHAR(dbh, "utf8");
    auto ret = DBPROVIDER_QUERY(dbh, "DELETE FROM tunnel WHERE imsi = 240679077777777");
    EXPECT_EQ(ret, 0);
    ret = DBPROVIDER_QUERY(dbh,
                      "INSERT INTO tunnel(imsi, msisdn, ueipv4, pgw_teid, sgw_gtpu_teid, sgw_gtpu_ipv, active,pgw_gtpc_ipv,pgw_gtpu_ipv)VALUES "\
                      "(240679077777777,811026279438,'0.6.7.8',0xdead2345,0xdead3456,'192.168.71.69',1,'','')");
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(event_rx(__inst, tpkt, sizeof(tpkt),  &saddr, sizeof(saddr), &caddr, sizeof(caddr), &srv), OK);
    EXPECT_EQ(event_gtpc_modify_bearer_req(__inst, __inst->nodes[PGW_NODE_GTPC_MODIFY_BEARER_REQ]), OK);
    auto nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPC_MODIFY_BEARER_REQ], EGRESS, pkts, 32);
    EXPECT_EQ(nburst, 1);
    if (nburst != 1){
        return;
    }
    //
    bzero(&parse_state, sizeof(parse_state));

    EXPECT_EQ(gtpc_alloc_packet_from_payload(&parsed_pkt,(U8*)pkts[0]->data, pkts[0]->datalen), OK);
    // debugging: setup response-side parse flags
    parse_state.flag |= HAVE_RES_ORDER;
    gtpc_iterate_item(parsed_pkt, on_gtpc_parse_root, &parse_state);
    EXPECT_EQ((parse_state.flag&HAVE_CAUSE), HAVE_CAUSE);
    EXPECT_EQ((parse_state.flag&HAVE_EBI_B), HAVE_EBI_B);
    EXPECT_EQ(parse_state.cause.cause, GTPC_CAUSE_REQUEST_ACCEPTED);
    EXPECT_EQ((parse_state.flag&HAVE_RECOVERY), HAVE_RECOVERY);
    EXPECT_EQ(parse_state.recovery.recovery_restart_counter, PGW_RECOVERY_COUNT);
    // new EBI must be returned
    EXPECT_EQ(parse_state.ebi_b.ebi.low, 0x05);
    // must be new teid
    EXPECT_EQ(((gtpc_header_ptr)pkts[0]->data)->t.teid, (U32)htonl(0xbe5ba1d0));


    EXPECT_EQ(gtpc_free_packet(&parsed_pkt), OK);

#if 1
    memset(&caddr, 0, sizeof(caddr));
    caddr.sin_family = AF_INET;
    caddr.sin_port = htons(2123);
    inet_pton(AF_INET, "8.8.8.8", &caddr.sin_addr.s_addr);

    auto sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    ret = pgw_send_sock(sock, &caddr, sizeof(struct sockaddr_in), tpkt, sizeof(tpkt));
    ret = pgw_send_sock(sock, &caddr, sizeof(struct sockaddr_in), pkts[0]->data, pkts[0]->datalen);
    close(sock);
#endif


    // sgw_gtp[c/u]_ipv,sgw_gtp[c/u]_teid must be updated.
    ret = DBPROVIDER_QUERY(dbh, "SELECT sgw_gtpc_ipv, sgw_gtpu_ipv , sgw_gtpc_teid, sgw_gtpu_teid, restart_counter FROM tunnel WHERE imsi = 240679077777777");
    auto res = DBPROVIDER_STORE_RESULT(dbh);
    EXPECT_EQ(res != NULL,true);
    if (res){
        auto rown = DBPROVIDER_NUM_ROWS(res);
        EXPECT_EQ(rown, 1);
        if (rown == 1){
            auto row = DBPROVIDER_FETCH_ROW(res);
            EXPECT_EQ(row!=NULL,true);
            if (row){
                EXPECT_EQ(strcmp(row[0], "202.179.205.113"), 0);
                EXPECT_EQ(strcmp(row[1], "202.179.205.129"), 0);
                EXPECT_EQ((U32)atoi(row[2]), (U32)htonl(0xbe5ba1d0));
                EXPECT_EQ((U32)atoi(row[3]), (U32)htonl(0xbe508be7));
                EXPECT_EQ((U32)atoi(row[4]), (U32)8);
            }
        }
        DBPROVIDER_FREE_RESULT(res);
    }
    for(auto n = 0;n < nburst;n++){
        pgw_free_packet(pkts[n]);
    }
    DBPROVIDER_CLOSE(dbh);
}

TEST(IntegrationTestBugfix, ErrorIndication){
    U8  tpkt[] = {
        0x32 ,0x1a ,0x00 ,0x10 ,                    // version 1
        0x00 ,0x00 ,0x00 ,0x00 ,                    // teid
        0x00 ,0x00 ,0x00 ,0x00 ,
        0x10 ,0x8b ,0x60 ,0x78 ,0x53 ,              // teid data-i
        0x85 ,0x00 ,0x04 ,0xc0 ,0xa8 ,0xc0 ,0xc9    // GSN address
    };
    struct sockaddr_in  saddr, caddr;
    server_t    srv;
    packet_ptr  pkts[32] = {NULL};
    gtp_packet_ptr  parsed_pkt = NULL;
    struct gtpc_parse_state    parse_state;
    //
    bzero(&saddr, sizeof(saddr));
    bzero(&caddr, sizeof(caddr));
    bzero(&srv,   sizeof(srv));
    srv.port = GTPU1_PORT;
    strncpy((char*)srv.ip, "254.254.254.254", MIN(sizeof(srv.ip)-1,15));
    //
    auto dbh = DBPROVIDER_INIT(NULL);
    EXPECT_EQ(dbh==NULL,false);
    if (!dbh){
        return;
    }
    EXPECT_EQ(DBPROVIDER_REAL_CONNECT(dbh,
                                 (TXT)HOST,
                                 (TXT)USER,
                                 (TXT)PSWD,
                                 (TXT)TESTDB,
                                 3306,
                                 NULL,
                                 0)==0,false);
    DBPROVIDER_SET_CHAR(dbh, "utf8");
    auto ret = DBPROVIDER_QUERY(dbh, "DELETE FROM tunnel WHERE imsi = 240679077777777");
    EXPECT_EQ(ret, 0);
    ret = DBPROVIDER_QUERY(dbh,
                      "INSERT INTO tunnel(imsi, msisdn, ueipv4, pgw_teid, sgw_gtpu_teid, sgw_gtpu_ipv, active,pgw_gtpc_ipv,pgw_gtpu_ipv)VALUES "\
                      "(240679077777777,811026279438,'0.6.7.8',0xdead2345,0x8b607853,'192.168.192.201',1,'','')");
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(event_rx(__inst, tpkt, sizeof(tpkt),  &saddr, sizeof(saddr), &caddr, sizeof(caddr), &srv), OK);
    EXPECT_EQ(event_gtpu_other_req(__inst, __inst->nodes[PGW_NODE_GTPU_OTHER_REQ]), OK);

    // status must be updated to active = 0 
    ret = DBPROVIDER_QUERY(dbh, "SELECT active FROM tunnel WHERE imsi = 240679077777777");
    auto res = DBPROVIDER_STORE_RESULT(dbh);
    EXPECT_EQ(res != NULL,true);
    if (res){
        auto rown = DBPROVIDER_NUM_ROWS(res);
        EXPECT_EQ(rown, 1);
        if (rown == 1){
            auto row = DBPROVIDER_FETCH_ROW(res);
            EXPECT_EQ(row!=NULL,true);
            if (row){
                EXPECT_EQ((U32)atoi(row[0]), (U32)0);
            }
        }
        DBPROVIDER_FREE_RESULT(res);
    }
    DBPROVIDER_CLOSE(dbh);
}

#include <chrono>


TEST(IntegrationTestBugfix, SgwPeers){
    U32 n;
    sgw_peers_ptr   inst;
    // generate, clean up
    for(n = 0;n < 1000;n++){
        EXPECT_EQ(pgw_create_sgw_peers(0, &inst), OK);
        EXPECT_EQ(pgw_free_sgw_peers(&inst), OK);
    }
#define MAX_PEER    (64)
    EXPECT_EQ(pgw_create_sgw_peers(0, &inst), OK);
    if (inst){
        // set appropriate peers at about 1K, 
        for(n = 1;n < MAX_PEER;n++){
            EXPECT_EQ(pgw_set_sgw_peer(inst, n, (n*10), (U32)time(NULL) + n), OK);
        }
        // loop , and , check correct order
        n = 1;
        pgw_get_sgw_peer(inst, -1,[](void* userdat, sgw_peer_ptr peer){
            auto nn = (U32*)userdat;
            EXPECT_EQ(peer->ip, (*nn));
            EXPECT_EQ(peer->counter, ((*nn)*10));
            (*nn)++;
            return(0);
        }, &n);
        EXPECT_EQ(n>1,true);

        auto stime = std::chrono::high_resolution_clock::now();
        // search processing must be fast.
        n = MAX_PEER/2;
        pgw_get_sgw_peer(inst, n,[](void* userdat, sgw_peer_ptr peer){
            auto nn = (U32*)userdat;
            EXPECT_EQ(peer->ip, (*nn));
            EXPECT_EQ(peer->counter, ((*nn)*10));
            (*nn) = 0;
            return(0);
        }, &n);
        EXPECT_EQ(n,0);

        auto duration = std::chrono::high_resolution_clock::now() - stime;
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();

        // less than 4us, -> 250K tps
        printf( ">> %llu ns\n", (U64)ns);
#ifdef __USE_SQLITE3__
        EXPECT_EQ((U64)ns < 40000, true);
#else
        EXPECT_EQ((U64)ns < 4000, true);
#endif

        // remove 1 item
        EXPECT_EQ(pgw_delete_sgw_peer(inst, MAX_PEER-1), 0);

        // deleted item must not exist.
        n = MAX_PEER-1;
        pgw_get_sgw_peer(inst, n,[](void* userdat, sgw_peer_ptr peer){
            auto nn = (U32*)userdat;
            (*nn) = 0;
            return(0);
        }, &n);
        EXPECT_EQ(n, MAX_PEER-1 );
        EXPECT_EQ(pgw_free_sgw_peers(&inst), OK);
    }
}

TEST(IntegrationTestBugfix, CreateSessionSqlRecoveryCounter){
    auto dbh = DBPROVIDER_INIT(NULL);
    EXPECT_EQ(dbh==NULL,false);
    if (!dbh){
        return;
    }
    EXPECT_EQ(DBPROVIDER_REAL_CONNECT(dbh,
                                 (TXT)HOST,
                                 (TXT)USER,
                                 (TXT)PSWD,
                                 (TXT)TESTDB,
                                 3306,
                                 NULL,
                                 0)==0,false);
    DBPROVIDER_SET_CHAR(dbh, "utf8");

#ifndef __USESQLITE3_ON_TEST__
    auto ret = DBPROVIDER_QUERY(dbh, "INSERT INTO tunnel(imsi,msisdn,ueipv4,pgw_teid,sgw_gtpu_teid,sgw_gtpu_ipv,active,sgw_gtpc_teid)VALUES "\
                                "(7,8,'0.6.7.8',0x00000006,0xdeadc0de,'0.2.3.4',1,91827364) ON DUPLICATE KEY UPDATE active=VALUES(active)");
#else
    auto ret = DBPROVIDER_QUERY(dbh, "INSERT OR IGNORE INTO tunnel(imsi,msisdn,ueipv4,pgw_teid,sgw_gtpu_teid,sgw_gtpu_ipv,active,sgw_gtpc_teid)VALUES "\
                                "(7,8,'0.6.7.8',0x00000006,0xdeadc0de,'0.2.3.4',1,91827364) ");
#endif
    EXPECT_EQ(ret, 0);


    char sql[1024];
    snprintf(sql, sizeof(sql)-1, CREATE_SESSION_UPD_R_SQL, 1, "0.1.2.3", 2, "4.5.6.7", 3, 4, 5, 6, 7ULL);

    ret = DBPROVIDER_QUERY(dbh, sql);
    EXPECT_EQ(ret, 0);

    // restart counter must be registererd.
    ret = DBPROVIDER_QUERY(dbh, "SELECT restart_counter FROM tunnel WHERE imsi = 7");
    auto res = DBPROVIDER_STORE_RESULT(dbh);
    EXPECT_EQ(res != NULL,true);
    if (res){
        auto rown = DBPROVIDER_NUM_ROWS(res);
        EXPECT_EQ(rown, 1);
        if (rown == 1){
            auto row = DBPROVIDER_FETCH_ROW(res);
            EXPECT_EQ(row!=NULL,true);
            if (row){
                EXPECT_EQ((U32)atoi(row[0]), (U32)6);
            }
        }
        DBPROVIDER_FREE_RESULT(res);
    }
    DBPROVIDER_CLOSE(dbh);
}

TEST(IntegrationTestBugfix, ModifyBearerReqSqlRecoveryCounter){
    auto dbh = DBPROVIDER_INIT(NULL);
    EXPECT_EQ(dbh==NULL,false);
    if (!dbh){
        return;
    }
    EXPECT_EQ(DBPROVIDER_REAL_CONNECT(dbh,
                                 (TXT)HOST,
                                 (TXT)USER,
                                 (TXT)PSWD,
                                 (TXT)TESTDB,
                                 3306,
                                 NULL,
                                 0)==0,false);
    DBPROVIDER_SET_CHAR(dbh, "utf8");


#ifndef __USESQLITE3_ON_TEST__
    auto ret = DBPROVIDER_QUERY(dbh, "INSERT INTO tunnel(imsi,msisdn,ueipv4,pgw_teid,sgw_gtpu_teid,sgw_gtpu_ipv,active,sgw_gtpc_teid)VALUES "\
                                "(987654321,987654321,'0.6.7.8',987654321,987654321,'0.2.3.4',9,987654321) ON DUPLICATE KEY UPDATE active=VALUES(active)");
#else
    auto ret = DBPROVIDER_QUERY(dbh, "INSERT OR IGNORE INTO tunnel(imsi,msisdn,ueipv4,pgw_teid,sgw_gtpu_teid,sgw_gtpu_ipv,active,sgw_gtpc_teid)VALUES "\
                                "(987654321,987654321,'0.6.7.8',987654321,987654321,'0.2.3.4',9,987654321) ");
#endif
    EXPECT_EQ(ret, 0);

#define CHECK_RESTART_COUNTER(c)  {\
    ret = DBPROVIDER_QUERY(dbh, "SELECT restart_counter FROM tunnel WHERE imsi = 987654321");\
    auto res = DBPROVIDER_STORE_RESULT(dbh);\
    EXPECT_EQ(res != NULL,true);\
    if (res){\
        auto rown = DBPROVIDER_NUM_ROWS(res);\
        EXPECT_EQ(rown, 1);\
        if (rown == 1){\
            auto row = DBPROVIDER_FETCH_ROW(res);\
            EXPECT_EQ(row!=NULL,true);\
            if (row){\
                EXPECT_EQ((U32)atoi(row[0]), (U32)c);\
            }\
        }\
        DBPROVIDER_FREE_RESULT(res);\
    }\
    }
    char sql[1024];
    snprintf(sql, sizeof(sql)-1, MODIFY_BEARER_RAT_UPD_R_SQL, 1, "1.1.1.1", 2, "2.2.2.2", 3, 4, 5, 987654321ULL);
    ret = DBPROVIDER_QUERY(dbh, sql);
    EXPECT_EQ(ret, 0);
    CHECK_RESTART_COUNTER(5);

    snprintf(sql, sizeof(sql)-1, MODIFY_BEARER_UPD_R_SQL,1, "1.1.1.1",2, "2.2.2.2",3,6,987654321ULL);
    ret = DBPROVIDER_QUERY(dbh, sql);
    EXPECT_EQ(ret, 0);
    CHECK_RESTART_COUNTER(6);

    snprintf(sql, sizeof(sql)-1, MODIFY_BEARER_RATONLY_UPD_R_SQL, 1, 7, 987654321ULL);
    ret = DBPROVIDER_QUERY(dbh, sql);
    EXPECT_EQ(ret, 0);
    CHECK_RESTART_COUNTER(7);

    DBPROVIDER_CLOSE(dbh);
}






TEST(IntegrationTestBugfix, PgwInstanceStop){
    EXPECT_EQ(pgw_stop(__inst, NULL, NULL), OK);
    EXPECT_EQ(pgw_release_instance(&__inst), OK);
}
RETCD event_cb(
        EVENT event, handle_ptr pinst, evutil_socket_t sock ,
        short what, const U8* data, const INT datalen,
        struct sockaddr_in* saddr, ssize_t saddrlen,
        struct sockaddr_in* caddr, ssize_t caddrlen, void* ext){
    RETCD       ret = ERR;
    node_ptr    pnode = NULL;
    //
    switch(event){
        case EVENT_POST_INIT:   ret = event_node_init(pinst, ext); break;
        case EVENT_UNINIT:      ret = event_node_uninit(pinst, ext); break;
        case EVENT_TIMER:       ret = event_timer(pinst, ext); break;
        case EVENT_GTPRX:
        case EVENT_ENTRY:
            // test target is above 2 events.
            break;
        default:
            pgw_panic("not implemented event.(%u)", event);
            break;
    }
    usleep(100000);
    return(OK);
}
