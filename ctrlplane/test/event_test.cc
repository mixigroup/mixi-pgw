#include "pgw_ext.h"
#include "node_ext.h"
#include "gtpc_ext.h"
#include "../src/pgw/pgw.h"
#include "gtest/gtest.h"

// instance
int PGW_LOG_LEVEL = 99;
pthread_mutex_t __mysql_mutex;

#define _TESTDB    ("test_event")
#include "test_cmn_inc.inl"


static void init_event_test_database(void){
    int ret;
    U32 port = 3306;
    U32 opt = 0;
    char    sql[1024] = {0};

    // inject tunnel data for debug.
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
                           (TXT)"",
                           port,
                           NULL,
                           opt)==0,false);

    snprintf(sql, sizeof(sql)-1,"USE %s", TESTDB);
    ASSERT_EQ(DBPROVIDER_QUERY(dbh, sql), 0);

    ret = DBPROVIDER_QUERY(dbh, "DELETE FROM tunnel WHERE msisdn = 819099999999");
    EXPECT_EQ(ret, 0);
#ifndef __USESQLITE3_ON_TEST__
    ret = DBPROVIDER_QUERY(dbh,
        "INSERT INTO tunnel(imsi,msisdn,ueipv4,pgw_teid,sgw_gtpu_teid,sgw_gtpu_ipv,active)VALUES "\
        "(240679099999999,819099999999,'0.6.7.8',0xdeadc0de,0xdeadc0de,'0.2.3.4',1) ON DUPLICATE KEY UPDATE active=VALUES(active)");
#else
    ret = DBPROVIDER_QUERY(dbh,
        "INSERT OR IGNORE INTO tunnel(imsi,msisdn,ueipv4,pgw_teid,sgw_gtpu_teid,sgw_gtpu_ipv,active)VALUES "\
        "(240679099999999,819099999999,'0.6.7.8',0xdeadc0de,0xdeadc0de,'0.2.3.4',1)");
#endif
    EXPECT_EQ(ret, 0);


#ifndef __USESQLITE3_ON_TEST__
    ret = DBPROVIDER_QUERY(dbh,
        "INSERT INTO tunnel(imsi,msisdn,ueipv4,pgw_teid,sgw_gtpu_teid,sgw_gtpu_ipv,active)VALUES "\
        "(240677026279438,811026279438,'0.6.7.9',0xdeadc0df,0xdeadc0df,'0.2.3.5',1) ON DUPLICATE KEY UPDATE active=VALUES(active)");
#else
    ret = DBPROVIDER_QUERY(dbh,
        "INSERT OR IGNORE INTO tunnel(imsi,msisdn,ueipv4,pgw_teid,sgw_gtpu_teid,sgw_gtpu_ipv,active)VALUES "\
        "(240677026279438,811026279438,'0.6.7.9',0xdeadc0df,0xdeadc0df,'0.2.3.5',1) ");
#endif
    EXPECT_EQ(ret, 0);

    DBPROVIDER_CLOSE(dbh);
}


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
TEST(IntegrateTestPacketProcessing, Start){
    U32 nodecnt = PGW_NODE_COUNT;
    U32 port = 3306;
    U32 opt = 0;
    //
    init_event_test_database();

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
    // wait for all threads started.
    while(1){
        usleep(100000);
        if ((__inst->flags&ON)==ON){
            break;
        }
    }
    // inject tunnel data for debugging.
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
        "INSERT INTO tunnel(imsi,msisdn,ueipv4,pgw_teid,sgw_gtpu_teid,sgw_gtpu_ipv,active)VALUES "\
        "(240679099999999,819099999999,'0.6.7.8',0xdeadc0de,0xdeadc0de,'0.2.3.4',1) ON DUPLICATE KEY UPDATE active=VALUES(active)");
#else
    ret = DBPROVIDER_QUERY(dbh,
        "INSERT OR IGNORE INTO tunnel(imsi,msisdn,ueipv4,pgw_teid,sgw_gtpu_teid,sgw_gtpu_ipv,active)VALUES "\
        "(240679099999999,819099999999,'0.6.7.8',0xdeadc0de,0xdeadc0de,'0.2.3.4',1)");
#endif
    EXPECT_EQ(ret, 0);
    DBPROVIDER_CLOSE(dbh);
    // validate
    EXPECT_EQ(PGW_RECOVERY_COUNT, (GTPC_RECOVERY_1 + 1));
}
TEST(IntegrateTestPacketProcessing, TransferRxResumeNotification){
    U8  tpkt[] = {
        0x48, 0xa4, 0x00, 0x14,   0x00, 0x00, 0x03, 0x44,
        0x44, 0x03, 0x00, 0x00,   0x01, 0x00, 0x08, 0x00,
        0x44, 0x10, 0x90, 0x10,   0x00, 0x20, 0x38, 0xf6,
    };
    struct sockaddr_in  saddr, caddr;
    server_t    srv;
    bzero(&saddr, sizeof(saddr));
    bzero(&caddr, sizeof(caddr));
    bzero(&srv,   sizeof(srv));
    srv.port = GTPC_PORT;

    packet_ptr  pkts[32] = {NULL};
    auto nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPC_OTHER_REQ], INGRESS, pkts, 32);
    //
    EXPECT_EQ(nburst, 0);
    EXPECT_EQ(event_rx(__inst, tpkt, sizeof(tpkt),  &saddr, sizeof(saddr), &caddr, sizeof(caddr), &srv), OK);
    // transfer to other Node
    nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPC_OTHER_REQ], INGRESS, pkts, 32);
    EXPECT_EQ(nburst, 1);
    //
    for(auto n = 0;n < nburst;n++){
        pgw_free_packet(pkts[n]);
    }
}
TEST(IntegrateTestPacketProcessing, TransferRxGtpcEcho){
    U8  tpkt[] = {
        0x40, 0x01, 0x00, 0x04,   0xa2, 0x01, 0x00, 0x00,
    };
    struct sockaddr_in  saddr, caddr;
    server_t    srv;
    packet_ptr  pkts[32] = {NULL};
    //
    bzero(&saddr, sizeof(saddr));
    bzero(&caddr, sizeof(caddr));
    bzero(&srv,   sizeof(srv));
    srv.port = GTPC_PORT;
    //
    auto nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPC_ECHO_REQ], INGRESS, pkts, 32);
    EXPECT_EQ(nburst, 0);
    EXPECT_EQ(event_rx(__inst, tpkt, sizeof(tpkt),  &saddr, sizeof(saddr), &caddr, sizeof(caddr), &srv), OK);
    // transfer to other Node
    nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPC_ECHO_REQ], INGRESS, pkts, 32);
    EXPECT_EQ(nburst, 1);
    //
    for(auto n = 0;n < nburst;n++){
        pgw_free_packet(pkts[n]);
    }
}
TEST(IntegrateTestPacketProcessing, TransferRxError){
    U8  tpkt[] = {
        0x20, 0x01, 0x00, 0x04,   0xa2, 0x01, 0x00, 0x00,
    };
    struct sockaddr_in  saddr, caddr;
    server_t    srv;
    packet_ptr  pkts[32] = {NULL};
    //
    bzero(&saddr, sizeof(saddr));
    bzero(&caddr, sizeof(caddr));
    bzero(&srv,   sizeof(srv));
    srv.port = GTPC_PORT;
    //
    auto nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPC_ECHO_REQ], INGRESS, pkts, 32);
    EXPECT_EQ(nburst, 0);
    EXPECT_EQ(event_rx(__inst, tpkt, sizeof(tpkt),  &saddr, sizeof(saddr), &caddr, sizeof(caddr), &srv), ERR);
    // not transferred
    nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPC_ECHO_REQ], INGRESS, pkts, 32);
    EXPECT_EQ(nburst, 0);
}
TEST(IntegrateTestPacketProcessing, TransferRxCreateSession){
    U8  tpkt[] = {
        0x48,0x20,0x00,0x7f,0x00,0x00,0x00,0x00,    0x90,0x20,0x00,0x00,0x01,0x00,0x08,0x00,
        0x42,0x60,0x97,0x10,0x00,0x01,0x33,0xf6,    0x4c,0x00,0x06,0x00,0x18,0x09,0x01,0x10,
        0x33,0x63,0x52,0x00,0x01,0x00,0x01,0x57,    0x00,0x09,0x00,0x86,0x00,0x00,0x20,0x90,
        0xc0,0xa8,0x38,0x14,0x47,0x00,0x0f,0x00,    0x04,0x74,0x65,0x73,0x74,0x05,0x78,0x66,
        0x6c,0x61,0x67,0x03,0x63,0x6f,0x6d,0x5d,    0x00,0x2c,0x00,0x49,0x00,0x01,0x00,0x01,
        0x57,0x00,0x09,0x00,0x84,0x00,0x00,0x20,    0x91,0xc0,0xa8,0x38,0x14,0x50,0x00,0x16,
        0x00,0x00,0x0e,0x00,0x00,0x02,0x00,0x00,    0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x02,
        0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x48,    0x00,0x08,0x00,0x00,0x02,0x00,0x00,0x00,
        0x02,0x00,0x00,
    };
    struct sockaddr_in  saddr, caddr;
    server_t    srv;
    bzero(&saddr, sizeof(saddr));
    bzero(&caddr, sizeof(caddr));
    bzero(&srv,   sizeof(srv));
    srv.port = GTPC_PORT;

    packet_ptr  pkts[32] = {NULL};
    auto nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPC_CREATE_SESSION_REQ], INGRESS, pkts, 32);
    //
    EXPECT_EQ(nburst, 0);
    EXPECT_EQ(event_rx(__inst, tpkt, sizeof(tpkt),  &saddr, sizeof(saddr), &caddr, sizeof(caddr), &srv), OK);
    // transfer to route for Create Session
    nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPC_CREATE_SESSION_REQ], INGRESS, pkts, 32);
    EXPECT_EQ(nburst, 1);
    //
    for(auto n = 0;n < nburst;n++){
        pgw_free_packet(pkts[n]);
    }
}
TEST(IntegrateTestPacketProcessing, TransferRxCreateSessionAppliancePgw){
    U8  tpkt[] = {
        0x48,0x20,0x00,0xd6,0x00,0x00,
        0x00,0x00,0x00,0x80,0x6e,0x00,0x01,0x00,0x07,0x00,0x71,0x65,0x57,0x74,0x98,0x87,
        0x10,0x4c,0x00,0x08,0x00,0x19,0x57,0x76,0x66,0x55,0x85,0x09,0xf1,0x4b,0x00,0x08,
        0x00,0x19,0x57,0x76,0x66,0x55,0x85,0x09,0xf1,0x56,0x00,0x0d,0x00,0x18,0x02,0xf8,
        0x01,0x01,0x23,0x02,0xf8,0x01,0x00,0x01,0x23,0x45,0x53,0x00,0x03,0x00,0x02,0xf8,
        0x01,0x52,0x00,0x01,0x00,0x06,0x4d,0x00,0x05,0x00,0x80,0x10,0x00,0x00,0x00,0x57,
        0x00,0x09,0x00,0x86,0x00,0x01,0xc8,0x8f,0xc0,0xa8,0x0f,0xc9,0x47,0x00,0x0e,0x00,
        0x09,0x61,0x66,0x66,0x69,0x72,0x6d,0x65,0x64,0x31,0x03,0x63,0x74,0x63,0x80,0x00,
        0x01,0x00,0x02,0x63,0x00,0x01,0x00,0x01,0x4f,0x00,0x05,0x00,0x01,0x0a,0x80,0x00,
        0x01,0x7f,0x00,0x01,0x00,0x00,0x48,0x00,0x08,0x00,0x00,0x00,0x27,0x10,0x00,0x00,
        0x27,0x10,0x5d,0x00,0x2c,0x00,0x49,0x00,0x01,0x00,0x05,0x57,0x00,0x09,0x02,0x84,
        0x00,0x12,0x84,0x71,0xc0,0xa8,0x0f,0xc9,0x50,0x00,0x16,0x00,0x04,0x09,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x72,0x00,0x02,0x00,0x40,0x00,0x91,0x00,0x08,0x00,0x02,0xf8,0x01,0x00,
        0x00,0x00,0x16,0x02,
    };
    struct sockaddr_in  saddr, caddr;
    server_t    srv;
    bzero(&saddr, sizeof(saddr));
    bzero(&caddr, sizeof(caddr));
    bzero(&srv,   sizeof(srv));
    srv.port = GTPC_PORT;

    packet_ptr  pkts[32] = {NULL};
    auto nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPC_CREATE_SESSION_REQ], INGRESS, pkts, 32);
    //
    EXPECT_EQ(nburst, 0);
    EXPECT_EQ(event_rx(__inst, tpkt, sizeof(tpkt),  &saddr, sizeof(saddr), &caddr, sizeof(caddr), &srv), OK);
    // transfer to route for Create Session
    nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPC_CREATE_SESSION_REQ], INGRESS, pkts, 32);
    EXPECT_EQ(nburst, 1);
    //
    for(auto n = 0;n < nburst;n++){
        pgw_free_packet(pkts[n]);
    }
}
TEST(IntegrateTestPacketProcessing, TransferRxModifyBearerAppliancePgw){
    U8  tpkt[] = {
        0x48,0x22,0x00,0x3a,0x00,0x05,
        0x38,0x1c,0x00,0x80,0x7e,0x00,0x56,0x00,0x0d,0x00,0x18,0x02,0xf8,0x01,0x01,0x23,
        0x02,0xf8,0x01,0x00,0x01,0x23,0x45,0x53,0x00,0x03,0x00,0x02,0xf8,0x01,0x52,0x00,
        0x01,0x00,0x06,0x4d,0x00,0x05,0x00,0x00,0x10,0x00,0x00,0x00,0x91,0x00,0x08,0x00,
        0x02,0xf8,0x01,0x00,0x00,0xc3,0xa3,0xb9,
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
    // transfer to route for Modify Bearer
    nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPC_MODIFY_BEARER_REQ], INGRESS, pkts, 32);
    EXPECT_EQ(nburst, 1);
    //
    for(auto n = 0;n < nburst;n++){
        pgw_free_packet(pkts[n]);
    }
}
TEST(IntegrateTestPacketProcessing, TransferRxGtpuEcho){
    U8  tpkt[] = {
        0x32, 0x01, 0x00, 0x04,   0x03, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
    };
    struct sockaddr_in  saddr, caddr;
    server_t    srv;
    packet_ptr  pkts[32] = {NULL};
    //
    bzero(&saddr, sizeof(saddr));
    bzero(&caddr, sizeof(caddr));
    bzero(&srv,   sizeof(srv));
    srv.port = GTPU1_PORT;
    //
    auto nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPU_ECHO_REQ], INGRESS, pkts, 32);
    EXPECT_EQ(nburst, 0);
    EXPECT_EQ(event_rx(__inst, tpkt, sizeof(tpkt),  &saddr, sizeof(saddr), &caddr, sizeof(caddr), &srv), OK);
    nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPU_ECHO_REQ], INGRESS, pkts, 32);
    EXPECT_EQ(nburst, 1);
    //
    for(auto n = 0;n < nburst;n++){
        pgw_free_packet(pkts[n]);
    }
}
TEST(IntegrateTestPacketProcessing, TransferRxGtpuOther){
    U8  tpkt[] = {
        0x32,0xff,0x00,0x34,0x00,0x00,
        0x16,0xcc,0xcc,0x16,0x00,0x00,0x45,0x00,0x00,0x30,0x02,0x78,0x00,0x00,0xff,0x01,
        0x6d,0xe1,0xde,0xad,0xbe,0xaf,0xac,0x14,0x02,0x02,0x08,0x00,0x15,0x1d,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x16,0xcc,0xcc,0x16,0x00,0x00,
    };
    struct sockaddr_in  saddr, caddr;
    server_t    srv;
    packet_ptr  pkts[32] = {NULL};
    //
    bzero(&saddr, sizeof(saddr));
    bzero(&caddr, sizeof(caddr));
    bzero(&srv,   sizeof(srv));
    srv.port = GTPU1_PORT;
    //
    auto nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPU_OTHER_REQ], INGRESS, pkts, 32);
    EXPECT_EQ(nburst, 0);
    EXPECT_EQ(event_rx(__inst, tpkt, sizeof(tpkt),  &saddr, sizeof(saddr), &caddr, sizeof(caddr), &srv), OK);
    nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPU_OTHER_REQ], INGRESS, pkts, 32);
    EXPECT_EQ(nburst, 1);
    //
    for(auto n = 0;n < nburst;n++){
        pgw_free_packet(pkts[n]);
    }
}
TEST(IntegrateTestPacketProcessing, CreateSessionWithOutPCO){
    U8  tpkt[] = {
        0x48,0x20,0x00,0x7f,0x00,0x00,0x00,0x00,    0x90,0x20,0x00,0x00,
        0x01,0x00,0x08,0x00,0x42,0x60,0x97,0x90,    0x99,0x99,0x99,0xf9,        // imsi
        0x4c,0x00,0x06,0x00,0x18,0x09,0x99,0x99,    0x99,0x99,                  // msisdn
        0x52,0x00,0x01,0x00,0x01,                                               // rat type
        0x57,0x00,0x09,0x00,0x86,0x00,0x00,0x20,    0x90,0xc0,0xa8,0x38,0x14,   // f-teid
        0x47,0x00,0x0f,0x00,0x04,0x74,0x65,0x73,    0x74,0x05,0x78,0x66,0x6c,
        0x61,0x67,0x03,0x63,0x6f,0x6d,                                          // apn

        0x5d,0x00,0x2c,0x00,                                                    // bearer context
        0x49,0x00,0x01,0x00,0x01,                                               // ebi
        0x57,0x00,0x09,0x00,0x84,0x00,0x00,0x20,    0x91,0xc0,0xa8,0x38,0x14,   // fteid
        0x50,0x00,0x16,0x00,0x00,0x0e,0x00,0x00,    0x02,0x00,0x00,0x00,0x00,   // bearer qos
        0x02,0x00,0x00,0x00,0x00,0x02,0x00,0x00,    0x00,0x00,0x02,0x00,0x00,
        0x48,0x00,0x08,0x00,0x00,0x02,0x00,0x00,    0x00,0x02,0x00,0x00,        // ambr
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
    //
    EXPECT_EQ(event_rx(__inst, tpkt, sizeof(tpkt),  &saddr, sizeof(saddr), &caddr, sizeof(caddr), &srv), OK);
    EXPECT_EQ(event_gtpc_create_session_req(__inst, __inst->nodes[PGW_NODE_GTPC_CREATE_SESSION_REQ]), OK);
    auto nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPC_CREATE_SESSION_REQ], EGRESS, pkts, 32);
    EXPECT_EQ(nburst, 1);

    //
    bzero(&parse_state, sizeof(parse_state));
    if (!nburst){
        return;
    }
    EXPECT_EQ(gtpc_alloc_packet_from_payload(&parsed_pkt,(U8*)pkts[0]->data, pkts[0]->datalen), OK);
    gtpc_iterate_item(parsed_pkt, on_gtpc_parse_root, &parse_state);
    EXPECT_EQ((parse_state.flag&HAVE_CAUSE), HAVE_CAUSE);
    EXPECT_EQ((parse_state.flag&HAVE_FTEID_C), HAVE_FTEID_C);
    EXPECT_EQ((parse_state.flag&HAVE_PAA), HAVE_PAA);
    EXPECT_EQ((parse_state.flag&HAVE_APN_R), HAVE_APN_R);
    EXPECT_EQ((parse_state.flag&HAVE_AMBR_R), HAVE_AMBR_R);
    EXPECT_EQ((parse_state.flag&HAVE_PAA), HAVE_PAA);
    EXPECT_EQ((parse_state.flag&HAVE_PCO), 0);
    //
    EXPECT_EQ(parse_state.cause.cause, GTPC_CAUSE_REQUEST_ACCEPTED);
    EXPECT_EQ(gtpc_free_packet(&parsed_pkt), OK);

    //
    for(auto n = 0;n < nburst;n++){
        pgw_free_packet(pkts[n]);
    }
}
TEST(IntegrateTestPacketProcessing, CreateSessionWithPCO){
    U8  tpkt[] = {
        0x48,0x20,0x00,0x87,0x00,0x00,0x00,0x00,    0x90,0x20,0x00,0x00,
        0x01,0x00,0x08,0x00,0x42,0x60,0x97,0x90,    0x99,0x99,0x99,0xf9,                // imsi
        0x4c,0x00,0x06,0x00,0x18,0x09,0x99,0x99,    0x99,0x99,                          // msisdn
        0x52,0x00,0x01,0x00,0x01,                                                       // rat type
        0x57,0x00,0x09,0x00,0x86,0x00,0x00,0x20,    0x90,0xc0,0xa8,0x38,0x14,           // f-teid
        0x47,0x00,0x0f,0x00,0x04,0x74,0x65,0x73,    0x74,0x05,0x78,0x66,
        0x6c,0x61,0x67,0x03,0x63,0x6f,0x6d,                                             // apn
        0x5d,0x00,0x2c,0x00,                                                            // bearer context
        0x49,0x00,0x01,0x00,0x01,                                                       // ebi
        0x57,0x00,0x09,0x00,0x84,0x00,0x00,0x20,    0x91,0xc0,0xa8,0x38,0x14,           // f-teid
        0x50,0x00,0x16,0x00,0x00,0x0e,0x00,0x00,    0x02,0x00,0x00,0x00,0x00,
        0x02,0x00,0x00,0x00,0x00,0x02,0x00,0x00,    0x00,0x00,0x02,0x00,0x00,           // bearer qos
        0x48,0x00,0x08,0x00,0x00,0x02,0x00,0x00,    0x00,0x02,0x00,0x00,                // ambr
        0x4e,0x00,0x04,0x00,0x00,0x00,0x0d,0x00,                                        // pco
    };
    U8   ambrchk[] = {0x00,0x02,0x00,0x00};
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
    //
    EXPECT_EQ(event_rx(__inst, tpkt, sizeof(tpkt),  &saddr, sizeof(saddr), &caddr, sizeof(caddr), &srv), OK);
    EXPECT_EQ(event_gtpc_create_session_req(__inst, __inst->nodes[PGW_NODE_GTPC_CREATE_SESSION_REQ]), OK);
    auto nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPC_CREATE_SESSION_REQ], EGRESS, pkts, 32);
    EXPECT_EQ(nburst, 1);

    //
    bzero(&parse_state, sizeof(parse_state));

    EXPECT_EQ(gtpc_alloc_packet_from_payload(&parsed_pkt,(U8*)pkts[0]->data, pkts[0]->datalen), OK);
    // debugging: setup response-side parse flags
    parse_state.flag |= HAVE_RES_ORDER;
    gtpc_iterate_item(parsed_pkt, on_gtpc_parse_root, &parse_state);
    EXPECT_EQ((parse_state.flag&HAVE_CAUSE), HAVE_CAUSE);
    EXPECT_EQ((parse_state.flag&HAVE_PCO), HAVE_PCO);
    EXPECT_EQ(parse_state.cause.cause, GTPC_CAUSE_REQUEST_ACCEPTED);
    EXPECT_EQ(parse_state.flag&HAVE_PCO_DNS_IPV4, HAVE_PCO_DNS_IPV4);
    EXPECT_EQ(parse_state.flag&HAVE_AMBR_R, HAVE_AMBR_R);
    EXPECT_EQ(memcmp(&parse_state.ambr_r.uplink,ambrchk,4), 0);

    EXPECT_EQ(gtpc_free_packet(&parsed_pkt), OK);
    //
    for(auto n = 0;n < nburst;n++){
        pgw_free_packet(pkts[n]);
    }
}

TEST(IntegrateTestPacketProcessing, RxModifyBearerAppliancePgwWithPCO){
    U8  tpkt[] = {
        0x48,0x22,0x00,0x42,
        0x1e,0xad,0xc0,0xde,
        0x00,0x80,0x7e,0x00,0x56,0x00,0x0d,0x00,0x18,0x02,0xf8,0x01,0x01,0x23,
        0x02,0xf8,0x01,0x00,0x01,0x23,0x45,0x53,0x00,0x03,0x00,0x02,0xf8,0x01,0x52,0x00,
        0x01,0x00,0x06,0x4d,0x00,0x05,0x00,0x00,0x10,0x00,0x00,0x00,0x91,0x00,0x08,0x00,
        0x02,0xf8,0x01,0x00,0x00,0xc3,0xa3,0xb9,

        0x4e,0x00,0x04,0x00,0x00,0x00,0x03,0x00,

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

    // allocated 1 packet
    EXPECT_EQ(gtpc_memory_print(), ERR);

    EXPECT_EQ(event_gtpc_modify_bearer_req(__inst, __inst->nodes[PGW_NODE_GTPC_MODIFY_BEARER_REQ]), OK);
    nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPC_MODIFY_BEARER_REQ], EGRESS, pkts, 32);
    EXPECT_EQ(nburst, 1);
    struct gtpc_parse_state    parse_state;
    gtp_packet_ptr  parsed_pkt = NULL;

    // allocated 1 packet
    EXPECT_EQ(gtpc_memory_print(), ERR);
    EXPECT_EQ(gtpc_alloc_packet_from_payload(&parsed_pkt,(U8*)pkts[0]->data, pkts[0]->datalen), OK);
    // debugging: setup response-side parse flags
    parse_state.flag |= HAVE_RES_ORDER;
    gtpc_iterate_item(parsed_pkt, on_gtpc_parse_root, &parse_state);
    EXPECT_EQ((parse_state.flag&HAVE_CAUSE), HAVE_CAUSE);
    EXPECT_EQ((parse_state.flag&HAVE_PCO), HAVE_PCO);
    EXPECT_EQ(parse_state.cause.cause, GTPC_CAUSE_REQUEST_ACCEPTED);
    EXPECT_EQ(parse_state.flag&HAVE_PCO_DNS_IPV6, HAVE_PCO_DNS_IPV6);
    EXPECT_EQ(gtpc_free_packet(&parsed_pkt), OK);
    //
    for(auto n = 0;n < nburst;n++){
        pgw_free_packet(pkts[n]);
    }
}
TEST(IntegrateTestPacketProcessing, DeleteBearerWithoutPCO){
    U8  tpkt[] = {
        0x48,0x63,0x00,0x0d,
        0x1e,0xad,0xc0,0xde,
        0x00,0x80,0x7e,0x00,
        0x49,0x00,0x01,0x00,0x00,

    };
    struct sockaddr_in  saddr, caddr;
    server_t    srv;
    bzero(&saddr, sizeof(saddr));
    bzero(&caddr, sizeof(caddr));
    bzero(&srv,   sizeof(srv));
    srv.port = GTPC_PORT;

    packet_ptr  pkts[32] = {NULL};
    EXPECT_EQ(event_rx(__inst, tpkt, sizeof(tpkt),  &saddr, sizeof(saddr), &caddr, sizeof(caddr), &srv), OK);
    EXPECT_EQ(event_gtpc_other_req(__inst, __inst->nodes[PGW_NODE_GTPC_OTHER_REQ]), OK);
    auto nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPC_OTHER_REQ], EGRESS, pkts, 32);
    EXPECT_EQ(nburst, 1);
    struct gtpc_parse_state    parse_state;
    gtp_packet_ptr  parsed_pkt = NULL;

    // allocated 1 packet
    EXPECT_EQ(gtpc_memory_print(), ERR);
    EXPECT_EQ(gtpc_alloc_packet_from_payload(&parsed_pkt,(U8*)pkts[0]->data, pkts[0]->datalen), OK);
    bzero(&parse_state, sizeof(parse_state));
    // debugging: setup response-side parse flags
    parse_state.flag |= HAVE_RES_ORDER;
    gtpc_iterate_item(parsed_pkt, on_gtpc_parse_root, &parse_state);
    EXPECT_EQ((parse_state.flag&HAVE_CAUSE), HAVE_CAUSE);
    EXPECT_EQ(parse_state.flag&HAVE_PCO, 0);

    fprintf(stderr, "has pco : %u\n",parse_state.flag&HAVE_PCO);

    EXPECT_EQ(parse_state.cause.cause, GTPC_CAUSE_REQUEST_ACCEPTED);
    EXPECT_EQ(gtpc_free_packet(&parsed_pkt), OK);
    //
    for(auto n = 0;n < nburst;n++){
        pgw_free_packet(pkts[n]);
    }
}



TEST(IntegrateTestPacketProcessing, DeleteBearerWithPCO){
    U8  tpkt[] = {
        0x48,0x63,0x00,0x15,
        0x1e,0xad,0xc0,0xde,
        0x00,0x80,0x7e,0x00,
        0x49,0x00,0x01,0x00,0x00,

        0x4e,0x00,0x04,0x00,0x00,0x00,0x03,0x00,

    };
    struct sockaddr_in  saddr, caddr;
    server_t    srv;
    bzero(&saddr, sizeof(saddr));
    bzero(&caddr, sizeof(caddr));
    bzero(&srv,   sizeof(srv));
    srv.port = GTPC_PORT;

    packet_ptr  pkts[32] = {NULL};
    EXPECT_EQ(event_rx(__inst, tpkt, sizeof(tpkt),  &saddr, sizeof(saddr), &caddr, sizeof(caddr), &srv), OK);
    EXPECT_EQ(event_gtpc_other_req(__inst, __inst->nodes[PGW_NODE_GTPC_OTHER_REQ]), OK);
    auto nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPC_OTHER_REQ], EGRESS, pkts, 32);
    EXPECT_EQ(nburst, 1);
    struct gtpc_parse_state    parse_state;
    gtp_packet_ptr  parsed_pkt = NULL;

    // allocated 1 packet
    EXPECT_EQ(gtpc_memory_print(), ERR);
    EXPECT_EQ(gtpc_alloc_packet_from_payload(&parsed_pkt,(U8*)pkts[0]->data, pkts[0]->datalen), OK);
    // debugging: setup response-side parse flags
    parse_state.flag |= HAVE_RES_ORDER;
    gtpc_iterate_item(parsed_pkt, on_gtpc_parse_root, &parse_state);
    EXPECT_EQ((parse_state.flag&HAVE_CAUSE), HAVE_CAUSE);
    EXPECT_EQ((parse_state.flag&HAVE_PCO), HAVE_PCO);
    EXPECT_EQ(parse_state.cause.cause, GTPC_CAUSE_REQUEST_ACCEPTED);
    EXPECT_EQ(parse_state.flag&HAVE_PCO_DNS_IPV6, HAVE_PCO_DNS_IPV6);
    EXPECT_EQ(gtpc_free_packet(&parsed_pkt), OK);
    //
    for(auto n = 0;n < nburst;n++){
        pgw_free_packet(pkts[n]);
    }
}


TEST(IntegrateTestPacketProcessing, DeleteSessionWithPCO){
    U8  tpkt[] = {
        0x48,0x24,0x00,0x15,
        0x1e,0xad,0xc0,0xde,
        0x00,0x80,0x7e,0x00,
        0x49,0x00,0x01,0x00,0x00,

        0x4e,0x00,0x04,0x00,0x00,0x00,0x03,0x00,

    };
    struct sockaddr_in  saddr, caddr;
    server_t    srv;
    bzero(&saddr, sizeof(saddr));
    bzero(&caddr, sizeof(caddr));
    bzero(&srv,   sizeof(srv));
    srv.port = GTPC_PORT;

    packet_ptr  pkts[32] = {NULL};
    EXPECT_EQ(event_rx(__inst, tpkt, sizeof(tpkt),  &saddr, sizeof(saddr), &caddr, sizeof(caddr), &srv), OK);
    EXPECT_EQ(event_gtpc_other_req(__inst, __inst->nodes[PGW_NODE_GTPC_OTHER_REQ]), OK);
    auto nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPC_OTHER_REQ], EGRESS, pkts, 32);
    EXPECT_EQ(nburst, 1);
    struct gtpc_parse_state    parse_state;
    gtp_packet_ptr  parsed_pkt = NULL;

    // allocated 1 packet
    EXPECT_EQ(gtpc_memory_print(), ERR);
    EXPECT_EQ(gtpc_alloc_packet_from_payload(&parsed_pkt,(U8*)pkts[0]->data, pkts[0]->datalen), OK);
    // debugging: setup response-side parse flags
    parse_state.flag |= HAVE_RES_ORDER;
    gtpc_iterate_item(parsed_pkt, on_gtpc_parse_root, &parse_state);
    EXPECT_EQ((parse_state.flag&HAVE_CAUSE), HAVE_CAUSE);
    EXPECT_EQ((parse_state.flag&HAVE_PCO), HAVE_PCO);
    EXPECT_EQ(parse_state.cause.cause, GTPC_CAUSE_REQUEST_ACCEPTED);
    EXPECT_EQ(parse_state.flag&HAVE_PCO_DNS_IPV6, HAVE_PCO_DNS_IPV6);
    EXPECT_EQ(gtpc_free_packet(&parsed_pkt), OK);
    //
    for(auto n = 0;n < nburst;n++){
        pgw_free_packet(pkts[n]);
    }
}

TEST(IntegrateTestPacketProcessing, CreateSessionPdnPaaIpv4){
    U8  tpkt[] = {
        0x48,0x20,0x00,0x8c,0x00,0x00,0x00,0x00,    0x90,0x20,0x00,0x00,
        0x01,0x00,0x08,0x00,0x42,0x60,0x97,0x90,    0x99,0x99,0x99,0xf9,  // imsi
        0x4c,0x00,0x06,0x00,0x18,0x09,0x99,0x99,
        0x99,0x99,0x52,0x00,0x01,0x00,0x01,0x57,    0x00,0x09,0x00,0x86,0x00,0x00,0x20,0x90,
        0xc0,0xa8,0x38,0x14,0x47,0x00,0x0f,0x00,    0x04,0x74,0x65,0x73,0x74,0x05,0x78,0x66,
        0x6c,0x61,0x67,0x03,0x63,0x6f,0x6d,0x5d,    0x00,0x2c,0x00,0x49,0x00,0x01,0x00,0x01,
        0x57,0x00,0x09,0x00,0x84,0x00,0x00,0x20,    0x91,0xc0,0xa8,0x38,0x14,0x50,0x00,0x16,
        0x00,0x00,0x0e,0x00,0x00,0x02,0x00,0x00,    0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x02,
        0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x48,    0x00,0x08,0x00,0x00,0x02,0x00,0x00,0x00,
        0x02,0x00,0x00,

        0x4e,0x00,0x04,0x00,0x00,0x00,0x0d,0x00,
        0x63,0x00,0x01,0x00,0x01,
    };
    U8   ambrchk[] = {0x00,0x02,0x00,0x00};
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
    //
    EXPECT_EQ(event_rx(__inst, tpkt, sizeof(tpkt),  &saddr, sizeof(saddr), &caddr, sizeof(caddr), &srv), OK);
    EXPECT_EQ(event_gtpc_create_session_req(__inst, __inst->nodes[PGW_NODE_GTPC_CREATE_SESSION_REQ]), OK);
    auto nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPC_CREATE_SESSION_REQ], EGRESS, pkts, 32);
    EXPECT_EQ(nburst, 1);

    //
    bzero(&parse_state, sizeof(parse_state));

    EXPECT_EQ(gtpc_alloc_packet_from_payload(&parsed_pkt,(U8*)pkts[0]->data, pkts[0]->datalen), OK);
    // debugging: setup response-side parse flags
    parse_state.flag |= HAVE_RES_ORDER;
    gtpc_iterate_item(parsed_pkt, on_gtpc_parse_root, &parse_state);
    EXPECT_EQ((parse_state.flag&HAVE_CAUSE), HAVE_CAUSE);
    EXPECT_EQ((parse_state.flag&HAVE_PCO), HAVE_PCO);
    EXPECT_EQ(parse_state.cause.cause, GTPC_CAUSE_REQUEST_ACCEPTED);
    EXPECT_EQ(parse_state.flag&HAVE_PCO_DNS_IPV4, HAVE_PCO_DNS_IPV4);
    EXPECT_EQ(parse_state.flag&HAVE_AMBR_R, HAVE_AMBR_R);
    EXPECT_EQ(memcmp(&parse_state.ambr_r.uplink,ambrchk,4), 0);

    EXPECT_EQ(parse_state.flag&HAVE_PAA, HAVE_PAA);
    EXPECT_EQ(parse_state.paa.bit.pdn_type, GTPC_PAA_PDNTYPE_IPV4);

    char check_v4_ui[4] = {0x0a, 0x06, 0x07, 0x08};
    EXPECT_EQ(memcmp(&parse_state.paa.paa[0], check_v4_ui, sizeof(check_v4_ui)), 0);


    EXPECT_EQ(gtpc_free_packet(&parsed_pkt), OK);
    //
    for(auto n = 0;n < nburst;n++){
        pgw_free_packet(pkts[n]);
    }
}


TEST(IntegrateTestPacketProcessing, CreateSessionPdnPaaIpv6){
    U8  tpkt[] = {
        0x48,0x20,0x00,0x8c,0x00,0x00,0x00,0x00,    0x90,0x20,0x00,0x00,
        0x01,0x00,0x08,0x00,0x42,0x60,0x97,0x90,    0x99,0x99,0x99,0xf9,    // imsi
        0x4c,0x00,0x06,0x00,0x18,0x09,0x99,0x99,
        0x99,0x99,0x52,0x00,0x01,0x00,0x01,0x57,    0x00,0x09,0x00,0x86,0x00,0x00,0x20,0x90,
        0xc0,0xa8,0x38,0x14,0x47,0x00,0x0f,0x00,    0x04,0x74,0x65,0x73,0x74,0x05,0x78,0x66,
        0x6c,0x61,0x67,0x03,0x63,0x6f,0x6d,0x5d,    0x00,0x2c,0x00,0x49,0x00,0x01,0x00,0x01,
        0x57,0x00,0x09,0x00,0x84,0x00,0x00,0x20,    0x91,0xc0,0xa8,0x38,0x14,0x50,0x00,0x16,
        0x00,0x00,0x0e,0x00,0x00,0x02,0x00,0x00,    0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x02,
        0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x48,    0x00,0x08,0x00,0x00,0x02,0x00,0x00,0x00,
        0x02,0x00,0x00,

        0x4e,0x00,0x04,0x00,0x00,0x00,0x0d,0x00,
        0x63,0x00,0x01,0x00,0x02,
    };
    U8   ambrchk[] = {0x00,0x02,0x00,0x00};
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
    //
    EXPECT_EQ(event_rx(__inst, tpkt, sizeof(tpkt),  &saddr, sizeof(saddr), &caddr, sizeof(caddr), &srv), OK);
    EXPECT_EQ(event_gtpc_create_session_req(__inst, __inst->nodes[PGW_NODE_GTPC_CREATE_SESSION_REQ]), OK);
    auto nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPC_CREATE_SESSION_REQ], EGRESS, pkts, 32);
    EXPECT_EQ(nburst, 1);

    //
    bzero(&parse_state, sizeof(parse_state));

    EXPECT_EQ(gtpc_alloc_packet_from_payload(&parsed_pkt,(U8*)pkts[0]->data, pkts[0]->datalen), OK);
    // debugging: setup response-side parse flags
    parse_state.flag |= HAVE_RES_ORDER;
    gtpc_iterate_item(parsed_pkt, on_gtpc_parse_root, &parse_state);
    EXPECT_EQ((parse_state.flag&HAVE_CAUSE), HAVE_CAUSE);
    EXPECT_EQ((parse_state.flag&HAVE_PCO), HAVE_PCO);
    EXPECT_EQ(parse_state.cause.cause, GTPC_CAUSE_REQUEST_ACCEPTED);
    EXPECT_EQ(parse_state.flag&HAVE_PCO_DNS_IPV4, HAVE_PCO_DNS_IPV4);
    EXPECT_EQ(parse_state.flag&HAVE_AMBR_R, HAVE_AMBR_R);
    EXPECT_EQ(memcmp(&parse_state.ambr_r.uplink,ambrchk,4), 0);

    EXPECT_EQ(parse_state.flag&HAVE_PAA, HAVE_PAA);
    EXPECT_EQ(parse_state.paa.bit.pdn_type, GTPC_PAA_PDNTYPE_IPV6);


    U8 check_v6_ui[4] = {0xfe,0x06,0x07,0x08};
    EXPECT_EQ(memcmp(&parse_state.paa.paa[5], check_v6_ui, sizeof(check_v6_ui)), 0);

    EXPECT_EQ(gtpc_free_packet(&parsed_pkt), OK);
    //
    for(auto n = 0;n < nburst;n++){
        pgw_free_packet(pkts[n]);
    }
}


TEST(IntegrateTestPacketProcessing, CreateSessionMNO_01026279438){
    U8  tpkt[] = {
        0x48,0x20,0x01,0x4e,
        0x00,0x00,0x00,0x00,0x4d,0xae,0x2a,0x00,
        0x01,0x00,0x08,0x00,0x42,0x60,0x97,0x90,    0x99,0x99,0x99,0xf9,        // imsi
        0x03,0x00,0x01,0x00,0x05,0x47,0x00,0x1d,0x00,0x05,0x72,0x61,
        0x74,0x65,0x6c,0x03,0x63,0x6f,0x6d,0x06,0x6d,0x6e,0x63,0x30,0x31,0x30,0x06,0x6d,
        0x63,0x63,0x34,0x34,0x30,0x04,0x67,0x70,0x72,0x73,0x48,0x00,0x08,0x00,0x00,0x41,
        0x89,0x37,0x00,0x41,0x89,0x37,0x4b,0x00,0x08,0x00,0x53,0x89,0x60,0x60,0x50,0x57,
        0x05,0x32,0x4c,0x00,0x06,0x00,0x18,0x07,0x62,0x72,0x49,0x83,0x4d,0x00,0x04,0x00,
        0x00,0x00,0x00,0x00,0x4e,0x00,0x6e,0x00,0x80,0xc2,0x23,0x24,0x01,0x00,0x00,0x24,
        0x10,0x5f,0x0c,0x19,0x19,0x5f,0x0c,0x19,0x19,0x5f,0x0c,0x19,0x19,0x5f,0x0c,0x19,
        0x19,0x72,0x61,0x74,0x65,0x6c,0x40,0x72,0x61,0x74,0x65,0x6c,0x2e,0x63,0x6f,0x6d,
        0xc2,0x23,0x24,0x02,0x00,0x00,0x24,0x10,0x69,0xf1,0x81,0x39,0x49,0x34,0x85,0xe1,
        0x52,0xb6,0x07,0x53,0xf1,0xa7,0xe6,0x1e,0x72,0x61,0x74,0x65,0x6c,0x40,0x72,0x61,
        0x74,0x65,0x6c,0x2e,0x63,0x6f,0x6d,0x80,0x21,0x10,0x01,0x00,0x00,0x10,0x81,0x06,
        0x00,0x00,0x00,0x00,0x83,0x06,0x00,0x00,0x00,0x00,0x00,0x0d,0x00,0x00,0x0a,0x00,
        0x00,0x05,0x00,0x00,0x10,0x00,0x4f,0x00,0x05,0x00,0x01,0x00,0x00,0x00,0x00,0x52,
        0x00,0x01,0x00,0x06,0x53,0x00,0x03,0x00,0x42,0xf0,0x01,0x56,0x00,0x0d,0x00,0x18,
        0x42,0xf0,0x01,0x14,0xf4,0x42,0xf0,0x01,0x02,0xc5,0xa3,0x01,0x57,0x00,0x09,0x00,
        0x86,0x00,0x0e,0xc2,0x6d,0x31,0x67,0x06,0xec,0x5d,0x00,0x2c,0x00,0x49,0x00,0x01,
        0x00,0x06,0x50,0x00,0x16,0x00,0x7c,0x09,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x57,0x00,0x09,0x02,
        0x84,0x40,0xa4,0x0a,0x72,0x31,0x67,0x06,0xca,0x63,0x00,0x01,0x00,0x01,0x7f,0x00,
        0x01,0x00,0x00,0x80,0x00,0x01,0x00,0x01,0x72,0x00,0x02,0x00,0x63,0x00,
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
    //
    EXPECT_EQ(event_rx(__inst, tpkt, sizeof(tpkt),  &saddr, sizeof(saddr), &caddr, sizeof(caddr), &srv), OK);
    EXPECT_EQ(event_gtpc_create_session_req(__inst, __inst->nodes[PGW_NODE_GTPC_CREATE_SESSION_REQ_EXT0]), OK);
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
    EXPECT_EQ(parse_state.flag&HAVE_PCO_DNS_IPV4, HAVE_PCO_DNS_IPV4);
    EXPECT_EQ(parse_state.flag&HAVE_AMBR_R, HAVE_AMBR_R);
    EXPECT_EQ(parse_state.flag&HAVE_PAA, HAVE_PAA);
    EXPECT_EQ(parse_state.paa.bit.pdn_type, GTPC_PAA_PDNTYPE_IPV4);

    EXPECT_EQ(gtpc_free_packet(&parsed_pkt), OK);
    //
    for(auto n = 0;n < nburst;n++){
        pgw_free_packet(pkts[n]);
    }
}

TEST(IntegrateTestPacketProcessing, CreateSessionMNO_V64_01026279438){
    U8  tpkt[] = {
        0x48,0x20,0x01,0x62,
        0x00,0x00,0x00,0x00,0x0c,0xbe,0x41,0x00,
        0x01,0x00,0x08,0x00,0x42,0x60,0x97,0x90,    0x99,0x99,0x99,0xf9,        // imsi
        0x03,0x00,0x01,0x00,0x03,0x47,0x00,0x1d,0x00,0x05,0x72,0x61,
        0x74,0x65,0x6c,0x03,0x63,0x6f,0x6d,0x06,0x6d,0x6e,0x63,0x30,0x31,0x30,0x06,0x6d,
        0x63,0x63,0x34,0x34,0x30,0x04,0x67,0x70,0x72,0x73,0x48,0x00,0x08,0x00,0x00,0x41,
        0x89,0x37,0x00,0x41,0x89,0x37,0x4b,0x00,0x08,0x00,0x53,0x89,0x60,0x60,0x50,0x57,
        0x05,0x32,0x4c,0x00,0x06,0x00,0x18,0x07,0x62,0x72,0x49,0x83,0x4d,0x00,0x04,0x00,
        0x80,0x00,0x00,0x00,0x4e,0x00,0x71,0x00,0x80,0xc2,0x23,0x24,0x01,0x00,0x00,0x24,
        0x10,0xe6,0xbc,0xc5,0xc5,0xe6,0xbc,0xc5,0xc5,0xe6,0xbc,0xc5,0xc5,0xe6,0xbc,0xc5,
        0xc5,0x72,0x61,0x74,0x65,0x6c,0x40,0x72,0x61,0x74,0x65,0x6c,0x2e,0x63,0x6f,0x6d,
        0xc2,0x23,0x24,0x02,0x00,0x00,0x24,0x10,0x4f,0x04,0xb0,0xdd,0xe4,0x0d,0x4b,0xe3,
        0xb9,0xb0,0xc5,0x2b,0xad,0x20,0x21,0x84,0x72,0x61,0x74,0x65,0x6c,0x40,0x72,0x61,
        0x74,0x65,0x6c,0x2e,0x63,0x6f,0x6d,0x80,0x21,0x10,0x01,0x00,0x00,0x10,0x81,0x06,
        0x00,0x00,0x00,0x00,0x83,0x06,0x00,0x00,0x00,0x00,0x00,0x0d,0x00,0x00,0x03,0x00,
        0x00,0x0a,0x00,0x00,0x05,0x00,0x00,0x10,0x00,0x4f,0x00,0x16,0x00,0x03,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x52,0x00,0x01,0x00,0x06,0x53,0x00,0x03,0x00,0x42,0xf0,0x01,0x56,
        0x00,0x0d,0x00,0x18,0x42,0xf0,0x01,0x14,0xf4,0x42,0xf0,0x01,0x02,0xdc,0x34,0x50,
        0x57,0x00,0x09,0x00,0x86,0x00,0x11,0x23,0x46,0x31,0x67,0x42,0xb4,0x5d,0x00,0x2c,
        0x00,0x49,0x00,0x01,0x00,0x06,0x50,0x00,0x16,0x00,0x7c,0x09,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x57,0x00,0x09,0x02,0x84,0x40,0x64,0x71,0x91,0x31,0x67,0x42,0x8c,0x63,0x00,0x01,
        0x00,0x03,0x7f,0x00,0x01,0x00,0x00,0x80,0x00,0x01,0x00,0x01,0x72,0x00,0x02,0x00,
        0x63,0x00,

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
    //
    EXPECT_EQ(event_rx(__inst, tpkt, sizeof(tpkt),  &saddr, sizeof(saddr), &caddr, sizeof(caddr), &srv), OK);
    EXPECT_EQ(event_gtpc_create_session_req(__inst, __inst->nodes[PGW_NODE_GTPC_CREATE_SESSION_REQ]), OK);
    auto nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPC_CREATE_SESSION_REQ], EGRESS, pkts, 32);
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
    EXPECT_EQ(parse_state.flag&HAVE_PCO_DNS_IPV4, HAVE_PCO_DNS_IPV4);
    EXPECT_EQ(parse_state.flag&HAVE_AMBR_R, HAVE_AMBR_R);
    EXPECT_EQ(parse_state.flag&HAVE_PAA, HAVE_PAA);

#ifdef __IPV64__
    // ipv6/4
    EXPECT_EQ(parse_state.paa.bit.pdn_type, GTPC_PAA_PDNTYPE_BOTH);
#else
    // ipv6/4 -> notified only ipv4
    EXPECT_EQ(parse_state.paa.bit.pdn_type, GTPC_PAA_PDNTYPE_IPV4);
    //EXPECT_EQ(parse_state.paa.bit.pdn_type, GTPC_PAA_PDNTYPE_BOTH);
#endif

    EXPECT_EQ(gtpc_free_packet(&parsed_pkt), OK);
    //
    for(auto n = 0;n < nburst;n++){
        pgw_free_packet(pkts[n]);
    }
}


RETCD on_create_session_mno_00_ebi_8_rec__(U32 counter, U32 clmncnt, colmn_ptr col, record_ptr rec, void* udat){
    EXPECT_EQ(clmncnt, 1);
    if (clmncnt == 1){
        EXPECT_EQ(rec[0].u.nval, 8);
    }
    *((U32*)udat) = 1;
    return(OK);
}
TEST(IntegrateTestPacketProcessing, CreateSessionMNO_Ebi_8){
    U8  tpkt[] = {
        0x48,0x20,0x01,0x63,0x00,0x00,0x00,0x00,0x1c,0x00,0x01,0x00,
        0x01,0x00,0x08,0x00,0x42,0x60,0x97,0x90,    0x99,0x99,0x99,0xf9,        // imsi
        0x4c,0x00,0x06,0x00,0x18,0x07,0x62,0x72,
        0x49,0x83,0x4b,0x00,0x08,0x00,0x53,0x89,0x60,0x60,0x50,0x57,0x05,0x32,0x56,0x00,
        0x0d,0x00,0x18,0x42,0xf0,0x01,0x14,0xf4,0x42,0xf0,0x01,0x02,0xdc,0x34,0x00,0x53,
        0x00,0x03,0x00,0x42,0xf0,0x01,0x52,0x00,0x01,0x00,0x06,0x4d,0x00,0x05,0x00,0x80,
        0x00,0x00,0x00,0x00,0x57,0x00,0x09,0x00,0x86,0x47,0x82,0x46,0xf4,0x31,0x67,0x0f,
        0x04,0x47,0x00,0x1d,0x00,0x05,0x72,0x61,0x74,0x65,0x6c,0x03,0x63,0x6f,0x6d,0x06,
        0x6d,0x6e,0x63,0x30,0x31,0x30,0x06,0x6d,0x63,0x63,0x34,0x34,0x30,0x04,0x67,0x70,
        0x72,0x73,0x80,0x00,0x01,0x00,0x01,0x63,0x00,0x01,0x00,0x03,0x4f,0x00,0x16,0x00,
        0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x7f,0x00,0x01,0x00,0x00,0x48,0x00,0x08,0x00,0x00,
        0x41,0x89,0x37,0x00,0x41,0x89,0x37,0x4e,0x00,0x71,0x00,0x80,0xc2,0x23,0x24,0x01,
        0x00,0x00,0x24,0x10,0xd9,0x89,0x56,0x56,0xd9,0x89,0x56,0x56,0xd9,0x89,0x56,0x56,
        0xd9,0x89,0x56,0x56,0x72,0x61,0x74,0x65,0x6c,0x40,0x72,0x61,0x74,0x65,0x6c,0x2e,
        0x63,0x6f,0x6d,0xc2,0x23,0x24,0x02,0x00,0x00,0x24,0x10,0x9d,0xbd,0x74,0xa5,0xdf,
        0x33,0xa1,0xd5,0x93,0x70,0xd9,0x1b,0x3b,0xd2,0x3f,0x94,0x72,0x61,0x74,0x65,0x6c,
        0x40,0x72,0x61,0x74,0x65,0x6c,0x2e,0x63,0x6f,0x6d,0x80,0x21,0x10,0x01,0x00,0x00,
        0x10,0x81,0x06,0x00,0x00,0x00,0x00,0x83,0x06,0x00,0x00,0x00,0x00,0x00,0x0d,0x00,
        0x00,0x03,0x00,0x00,0x0a,0x00,0x00,0x05,0x00,0x00,0x10,0x00,0x5d,0x00,0x2c,0x00,
        0x49,0x00,0x01,0x00,0x08,0x57,0x00,0x09,0x02,0x84,0x39,0x3c,0xd0,0xfe,0x31,0x67,
        0x0f,0x04,0x50,0x00,0x16,0x00,0x7c,0x09,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x01,0x00,
        0x08,0x72,0x00,0x02,0x00,0x63,0x00,

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
    //
    EXPECT_EQ(event_rx(__inst, tpkt, sizeof(tpkt),  &saddr, sizeof(saddr), &caddr, sizeof(caddr), &srv), OK);
    EXPECT_EQ(event_gtpc_create_session_req(__inst, __inst->nodes[PGW_NODE_GTPC_CREATE_SESSION_REQ]), OK);
    auto nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPC_CREATE_SESSION_REQ], EGRESS, pkts, 32);
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
    EXPECT_EQ(parse_state.flag&HAVE_PCO_DNS_IPV4, HAVE_PCO_DNS_IPV4);
    EXPECT_EQ(parse_state.flag&HAVE_AMBR_R, HAVE_AMBR_R);
    EXPECT_EQ(parse_state.flag&HAVE_PAA, HAVE_PAA);

#ifdef __IPV64__
    // ipv6/4
    EXPECT_EQ(parse_state.paa.bit.pdn_type, GTPC_PAA_PDNTYPE_BOTH);
#else
    // ipv6/4 -> notified only ipv4
    EXPECT_EQ(parse_state.paa.bit.pdn_type, GTPC_PAA_PDNTYPE_IPV4);
    //EXPECT_EQ(parse_state.paa.bit.pdn_type, GTPC_PAA_PDNTYPE_BOTH);
#endif
    EXPECT_EQ(parse_state.flag&HAVE_EBI_B, HAVE_EBI_B);
    EXPECT_EQ(parse_state.ebi_b.ebi.low, 0x08);
    EXPECT_EQ(gtpc_free_packet(&parsed_pkt), OK);

    //
    for(auto n = 0;n < nburst;n++){
        pgw_free_packet(pkts[n]);
    }



    // compare with ebi in database
    char sql[1024] = {0};
    U32  rec_isexists = 0;
    DBPROVIDER_ST_BIND_I    bind;
    snprintf(sql, sizeof(sql) -1 ,"SELECT ebi FROM tunnel WHERE imsi=240679099999999");
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
    auto stmt = DBPROVIDER_STMT_INIT(dbh);
    EXPECT_EQ(stmt==NULL,false);
    if (!stmt) { return; }
    EXPECT_EQ(DBPROVIDER_STMT_PREPARE(stmt, sql, strlen(sql)), 0);
    EXPECT_EQ(DBPROVIDER_BIND_INIT(&bind), 0);
    EXPECT_EQ(DBPROVIDER_EXECUTE(stmt, bind.bind, on_create_session_mno_00_ebi_8_rec__, &rec_isexists), 0);
    DBPROVIDER_CLOSE(dbh);
    EXPECT_EQ(rec_isexists, 1);
}

#include <netinet/ip6.h>
#include <netinet/icmp6.h>


typedef struct icmpv6_pseudo_header{
    struct in6_addr     srcaddr;
    struct in6_addr     dstaddr;
    U16                 payloadlen;
    U16                 next;
}__attribute__ ((packed)) icmpv6_pseudo_header_t,*icmpv6_pseudo_header_ptr;

static unsigned _checksum(const void *data, unsigned short len, unsigned sum){
    unsigned  _sum   = sum,n;
    unsigned  _count = len;
    unsigned short* _addr  = (unsigned short*)data;
    //
    while( _count > 1 ) {
        _sum += ntohs(*_addr);
        _addr++;
        _count -= 2;
    }
    if(_count > 0 ){
        _sum += ntohs(*_addr);
    }
    while (_sum>>16){
        _sum = (_sum & 0xffff) + (_sum >> 16);
    }
    printf(">>>>> test(%u : %u) \n", (unsigned)len, (unsigned)sum);
    for(n = 0;n < len;n++){
        printf("%02x", *(U8*)&((char*)data)[n]);
        if (n % 4 == 0) { printf(" ");}
        if (n % 8 == 0) { printf("\n");}
    }
    printf("<<<<<(%u)\n", ~_sum);

    return(~_sum);
}
static unsigned short _wrapsum(unsigned sum){
    sum = sum & 0xFFFF;
    return (htons(sum));
}



TEST(IntegrateTestPacketProcessing, RouterSolicitaion){
    U8  tpkt[] = {
        0x30,0xff,0x00,0x30,
        0x8e,0xad,0xc0,0xde,
        0x60,0x00,0x00,0x00,0x00,0x08,0x3a,0xff,0xfe,0x80,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xff,0x02,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x85,0x00,0x7d,0x36,
        0x00,0x00,0x00,0x00,
    };
    struct sockaddr_in  saddr, caddr;
    struct gtpc_parse_state    parse_state;
    server_t    srv;
    packet_ptr  pkts[32] = {NULL};
    //
    bzero(&saddr, sizeof(saddr));
    bzero(&caddr, sizeof(caddr));
    bzero(&srv,   sizeof(srv));
    srv.port = GTPU1_PORT;
    //
    EXPECT_EQ(event_rx(__inst, tpkt, sizeof(tpkt),  &saddr, sizeof(saddr), &caddr, sizeof(caddr), &srv), OK);
    EXPECT_EQ(event_gtpu_other_req(__inst, __inst->nodes[PGW_NODE_GTPU_OTHER_REQ]), OK);
    auto nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPU_OTHER_REQ], EGRESS, pkts, 32);
    EXPECT_EQ(nburst, 1);
    if (nburst != 1){
        return;
    }
    // Router Advertizement packet
    auto gtpuh = gtpu_header(pkts[0]);
    struct ip6_hdr*             ip6h  = (struct ip6_hdr*)&(gtpuh[1]);
    struct nd_router_advert*    ndra  = (struct nd_router_advert*)&(ip6h[1]);
    struct nd_opt_mtu*          ndom  = (struct nd_opt_mtu*)&(ndra[1]);
    struct nd_opt_prefix_info*  ndopi = (struct nd_opt_prefix_info*)&(ndom[1]);

    // gtpu + ipv6 + icmpv6 + router advertizement.
    EXPECT_EQ(gtpuh->type, GTPU_G_PDU);
    EXPECT_EQ(ntohs(gtpuh->length), (sizeof(*ip6h) + sizeof(*ndra) + sizeof(*ndom) + sizeof(*ndopi)));
    if (ntohs(gtpuh->length) != (sizeof(*ip6h) + sizeof(*ndra) + sizeof(*ndom) + sizeof(*ndopi))){
        return;
    }

    EXPECT_EQ(ndom->nd_opt_mtu_type , ND_OPT_MTU);
    EXPECT_EQ(ndom->nd_opt_mtu_len , 1);
    EXPECT_EQ(ndom->nd_opt_mtu_mtu , htonl(1500));

    EXPECT_EQ(ndopi->nd_opt_pi_type, ND_OPT_PREFIX_INFORMATION);
    EXPECT_EQ(ndopi->nd_opt_pi_len, 4);
    EXPECT_EQ(ndopi->nd_opt_pi_prefix_len, 0x40);
    EXPECT_EQ(ndopi->nd_opt_pi_flags_reserved, (ND_OPT_PI_FLAG_AUTO));
    EXPECT_EQ(ndopi->nd_opt_pi_valid_time, htonl(86400));
    EXPECT_EQ(ndopi->nd_opt_pi_preferred_time ,htonl(43200));

    U8  xxxx_ipv6_network[16] = {0x24, 0x01, 0xf1, 0x00,
                    0xfe, 0x06, 0x07, 0x08,
                    0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00};
    EXPECT_EQ(memcmp(&ndopi->nd_opt_pi_prefix, xxxx_ipv6_network, 16)==0 ,true);

    U8  checksum_buf[2048] = {0};
    icmpv6_pseudo_header_ptr icmpv6_pseudo_h = (icmpv6_pseudo_header_ptr)checksum_buf;

    memcpy(&icmpv6_pseudo_h->srcaddr, &ip6h->ip6_src, sizeof(ip6h->ip6_src));
    memcpy(&icmpv6_pseudo_h->dstaddr, &ip6h->ip6_dst, sizeof(ip6h->ip6_dst));
    icmpv6_pseudo_h->payloadlen    = (ip6h->ip6_plen);
    icmpv6_pseudo_h->next          = ntohs(ip6h->ip6_nxt);
    auto beforechecksum = ndra->nd_ra_hdr.icmp6_cksum;
    ndra->nd_ra_hdr.icmp6_cksum    = 0;
    memcpy(icmpv6_pseudo_h+1, ndra, ntohs(ip6h->ip6_plen));



    auto checksum = _checksum(icmpv6_pseudo_h, sizeof(*icmpv6_pseudo_h) + ntohs(ip6h->ip6_plen), 0);
    EXPECT_EQ(beforechecksum, _wrapsum(checksum));

    //
    for(auto n = 0;n < nburst;n++){
        pgw_free_packet(pkts[n]);
    }
}



TEST(IntegrateTestPacketProcessing, CreateSession_V6ip){
    U8  tpkt[] = {
            0x48,0x20,0x01,0x48,0x00,0x00,0x00,0x00,0x3f,0xff,0xdb,0x00,
            0x01,0x00,0x08,0x00,
            0x42,0x60,0x97,0x90,    0x99,0x99,0x99,0xf9,        // imsi
            0x03,0x00,0x01,0x00,0x05,
            0x47,0x00,0x1d,0x00,0x05,0x72,0x61,0x74,0x65,0x6c,0x03,0x63,0x6f,0x6d,0x06,0x6d,0x6e,0x63,0x30,
            0x31,0x30,0x06,0x6d,0x63,0x63,0x34,0x34,0x30,0x04,0x67,0x70,0x72,0x73,
            0x48,0x00,0x08,0x00,0x00,0x41,0x89,0x37,0x00,0x41,0x89,0x37,
            0x4b,0x00,0x08,0x00,0x53,0x89,0x60,0x60,0x20,0x20,0x56,0x81,

            0x4c,0x00,0x06,0x00,0x18,0x07,0x62,0x72,0x49,0x83,

            0x4d,0x00,0x04,0x00,0x00,0x00,0x00,0x00,


            0x4e,0x00,0x5b,0x00,0x80,0xc2,0x23,0x24,
            0x01,0x00,0x00,0x24,0x10,0x4b,0xb1,0x52,0x52,0x4b,0xb1,0x52,0x52,0x4b,0xb1,0x52,
            0x52,0x4b,0xb1,0x52,0x52,0x72,0x61,0x74,0x65,0x6c,0x40,0x72,0x61,0x74,0x65,0x6c,
            0x2e,0x63,0x6f,0x6d,0xc2,0x23,0x24,0x02,0x00,0x00,0x24,0x10,0xdd,0x99,0x4e,0xdd,
            0x77,0xbc,0x2a,0xa7,0x35,0x15,0x74,0x89,0x67,0xe1,0x39,0x4c,0x72,0x61,0x74,0x65,
            0x6c,0x40,0x72,0x61,0x74,0x65,0x6c,0x2e,0x63,0x6f,0x6d,0x00,0x03,0x00,0x00,0x0a,
            0x00,0x00,0x05,0x00,0x00,0x10,0x00,

            0x4f,0x00,0x12,0x00,0x02,0x00,0x00,0x00,0x00,
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x52,0x00,0x01,
            0x00,0x06,0x53,0x00,0x03,0x00,0x42,0xf0,0x01,0x56,0x00,0x0d,0x00,0x18,0x42,0xf0,
            0x01,0x14,0xf4,0x42,0xf0,0x01,0x02,0xdc,0x34,0x00,0x57,0x00,0x09,0x00,0x86,0x00,
            0x08,0x98,0x7a,0x31,0x67,0x46,0x6c,0x5d,0x00,0x2c,0x00,0x49,0x00,0x01,0x00,0x0a,
            0x50,0x00,0x16,0x00,0x7c,0x09,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x57,0x00,0x09,0x02,0x84,0x40,
            0xa0,0xde,0x8d,0x31,0x67,0x46,0x48,0x63,0x00,0x01,0x00,0x02,0x7f,0x00,0x01,0x00,
            0x00,0x80,0x00,0x01,0x00,0x01,0x72,0x00,0x02,0x00,0x63,0x00,
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
    //
    EXPECT_EQ(event_rx(__inst, tpkt, sizeof(tpkt),  &saddr, sizeof(saddr), &caddr, sizeof(caddr), &srv), OK);
    EXPECT_EQ(event_gtpc_create_session_req(__inst, __inst->nodes[PGW_NODE_GTPC_CREATE_SESSION_REQ_EXT0]), OK);
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
    EXPECT_EQ(parse_state.flag&HAVE_PCO_DNS_IPV6, HAVE_PCO_DNS_IPV6);
    EXPECT_EQ(parse_state.flag&HAVE_PCO_IPV4_LINK_MTU, HAVE_PCO_IPV4_LINK_MTU);
    EXPECT_EQ(parse_state.pco.pco_head.extension,1);


    EXPECT_EQ(parse_state.flag&HAVE_AMBR_R, HAVE_AMBR_R);
    EXPECT_EQ(parse_state.flag&HAVE_PAA, HAVE_PAA);
    EXPECT_EQ(parse_state.paa.bit.pdn_type, GTPC_PAA_PDNTYPE_IPV6);
    U8  compare_ipv6[16] = {0x24, 0x01, 0xf1, 0x00,
                            0xfe, 0x06, 0x07, 0x08,
                            0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x01};
    EXPECT_EQ(memcmp(&parse_state.paa.paa[1], compare_ipv6, sizeof(compare_ipv6)),0);

#if 1
    bzero(&saddr, sizeof(saddr));
    bzero(&caddr, sizeof(caddr));
    caddr.sin_family = AF_INET;
    caddr.sin_port = htons(2123);
    inet_pton(AF_INET, "8.8.8.8", &caddr.sin_addr.s_addr);

    auto sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    auto ret = pgw_send_sock(sock, &caddr, sizeof(struct sockaddr_in), tpkt, sizeof(tpkt));
    ret = pgw_send_sock(sock, &caddr, sizeof(struct sockaddr_in), pkts[0]->data, pkts[0]->datalen);
    close(sock);
#endif


    EXPECT_EQ(gtpc_free_packet(&parsed_pkt), OK);
    //
    for(auto n = 0;n < nburst;n++){
        pgw_free_packet(pkts[n]);
    }
}



#include "gtpu_def.h"
TEST(IntegrateTestPacketProcessing, ErrorIndication){
    U8  tpkt[] = {
            0x32 ,0x1a ,0x00 ,0x10 ,0x00 ,0x00 ,0x00 ,0x00 ,
            0x00 ,0x00 ,0x00 ,0x00 ,0x10 ,0xde ,0xad ,0xc0 ,
            0xde ,0x85 ,0x00 ,0x04 ,0x31 ,0x67 ,0x47 ,0x45
    };
    struct sockaddr_in  saddr, caddr;
    server_t    srv;
    //
    bzero(&saddr, sizeof(saddr));
    bzero(&caddr, sizeof(caddr));
    bzero(&srv,   sizeof(srv));
    srv.port = GTPU1_PORT;

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
                                 3306,
                                 NULL,
                                 0)==0,false);
    DBPROVIDER_SET_CHAR(dbh, "utf8");
    auto ret = DBPROVIDER_QUERY(dbh, "DELETE FROM tunnel WHERE imsi = 240679098765432");
    EXPECT_EQ(ret, 0);


    ret = DBPROVIDER_QUERY(dbh,
                      "INSERT INTO tunnel(imsi,msisdn,ueipv4,pgw_teid,sgw_gtpu_teid,sgw_gtpu_ipv,active)VALUES "\
                      "(240679098765432,819098765432,'0.6.7.8',0xdeadc1de,0xdeadc0de,'49.103.71.69',1)");
    EXPECT_EQ(ret, 0);
    //
    EXPECT_EQ(event_rx(__inst, tpkt, sizeof(tpkt),  &saddr, sizeof(saddr), &caddr, sizeof(caddr), &srv), OK);
    EXPECT_EQ(event_gtpu_other_req(__inst, __inst->nodes[PGW_NODE_GTPU_OTHER_REQ]), OK);
    // session must be deleted
    ret = DBPROVIDER_QUERY(dbh, "SELECT COUNT(1) FROM tunnel WHERE imsi = 240679098765432 AND active=1");
    auto res = DBPROVIDER_STORE_RESULT(dbh);
    EXPECT_EQ(res != NULL,true);
    if (res){
        auto rown = DBPROVIDER_NUM_ROWS(res);
        EXPECT_EQ(rown, 1);
        if (rown == 1){
            auto row = DBPROVIDER_FETCH_ROW(res);
            EXPECT_EQ(row!=NULL,true);
            if (row){
                EXPECT_EQ(atoi(row[0]), 0);
            }
        }
        DBPROVIDER_FREE_RESULT(res);
    }
    DBPROVIDER_CLOSE(dbh);
}




TEST(IntegrateTestPacketProcessing, CreateSession_PCO_IPCP){
    U8  tpkt[] = {
        0x48,0x20,0x01,0x1a,0x00,0x00,0x00,0x00,0x17,0x6d,0x05,0x00,
        0x01,0x00,0x08,0x00,0x42,0x60,0x97,0x90,    0x99,0x99,0x99,0xf9,        // imsi
        0x03,0x00,0x01,0x00,0x03,                                           // recovery

        0x47,0x00,0x1d,0x00,0x05,0x72,0x61,0x74,0x65,0x6c,0x03,0x63,0x6f,0x6d,0x06,0x6d,
        0x6e,0x63,0x30,0x31,0x30,0x06,0x6d,0x63,0x63,0x34,0x34,0x30,0x04,0x67,0x70,0x72,
        0x73,                                                               // apn

        0x48,0x00,0x08,0x00,0x00,0x41,0x89,0x37,0x00,0x41,0x89,0x37,        // ambr
        0x4b,0x00,0x08,0x00,0x53,0x72,0x22,0x70,0x96,0x74,0x16,0x10,        // mei
        0x4c,0x00,0x06,0x00,0x18,0x02,0x91,0x63,0x54,0x39,                  // msisdn
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
    auto ret = DBPROVIDER_QUERY(dbh, "DELETE FROM tunnel WHERE imsi IN( 812019364593, 240679099999999)");
    EXPECT_EQ(ret, 0);
    ret = DBPROVIDER_QUERY(dbh,
                      "INSERT INTO tunnel(imsi,msisdn,ueipv4,pgw_teid,sgw_gtpu_teid,sgw_gtpu_ipv,active)VALUES "\
                      "(812019364593,812019364593,'0.6.7.8',0xdead1234,0xdead6549,'49.103.71.69',1)");
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(event_rx(__inst, tpkt, sizeof(tpkt),  &saddr, sizeof(saddr), &caddr, sizeof(caddr), &srv), OK);
    EXPECT_EQ(event_gtpc_create_session_req(__inst, __inst->nodes[PGW_NODE_GTPC_CREATE_SESSION_REQ_EXT0]), OK);
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
    EXPECT_EQ(parse_state.flag&HAVE_PCO_DNS_IPV4, HAVE_PCO_DNS_IPV4);
    EXPECT_EQ(parse_state.pco.pco_head.extension,1);
    EXPECT_EQ(parse_state.flag&HAVE_PCO_IPCP_P_DNS, HAVE_PCO_IPCP_P_DNS);
    EXPECT_EQ(parse_state.flag&HAVE_PCO_IPCP_S_DNS, HAVE_PCO_IPCP_S_DNS);

    if (parse_state.flag&(HAVE_PCO_IPCP_S_DNS|HAVE_PCO_IPCP_S_DNS)){

        U8     compare[19];
        memcpy(compare, PCO_DNS_IPCP_IPV4_VAL, sizeof(compare));
        // must be nak
        compare[3] = 0x03;
        EXPECT_EQ(memcmp(&parse_state.pco.pco[14],compare, sizeof(compare)),0);
    }
    EXPECT_EQ(parse_state.flag&HAVE_AMBR_R, HAVE_AMBR_R);
    EXPECT_EQ(parse_state.flag&HAVE_PAA, HAVE_PAA);
    EXPECT_EQ(gtpc_free_packet(&parsed_pkt), OK);
    //
    for(auto n = 0;n < nburst;n++){
        pgw_free_packet(pkts[n]);
    }
    DBPROVIDER_CLOSE(dbh);
}
TEST(IntegrateTestPacketProcessing, CreateSession_IMSI_00){
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
                      "INSERT INTO tunnel(imsi, msisdn, ueipv4, pgw_teid, sgw_gtpu_teid, sgw_gtpu_ipv, active)VALUES "\
                      "(240679077777777,0,'0.6.7.8',0xdead2345,0xdead3456,'49.103.71.69',1)");
    EXPECT_EQ(ret, 0);
    // set re-start count
    PGW_RECOVERY_COUNT = (GTPC_RECOVERY_1 + 12);

    EXPECT_EQ(event_rx(__inst, tpkt, sizeof(tpkt),  &saddr, sizeof(saddr), &caddr, sizeof(caddr), &srv), OK);
    EXPECT_EQ(event_gtpc_create_session_req(__inst, __inst->nodes[PGW_NODE_GTPC_CREATE_SESSION_REQ_EXT0]), OK);
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
#if 1
    bzero(&saddr, sizeof(saddr));
    bzero(&caddr, sizeof(caddr));
    caddr.sin_family = AF_INET;
    caddr.sin_port = htons(2123);
    inet_pton(AF_INET, "8.8.8.8", &caddr.sin_addr.s_addr);

    auto sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    ret = pgw_send_sock(sock, &caddr, sizeof(struct sockaddr_in), tpkt, sizeof(tpkt));
    ret = pgw_send_sock(sock, &caddr, sizeof(struct sockaddr_in), pkts[0]->data, pkts[0]->datalen);
    close(sock);
#endif
    //  msisdn must be updated
    ret = DBPROVIDER_QUERY(dbh, "SELECT msisdn FROM tunnel WHERE imsi = 240679077777777");
    auto res = DBPROVIDER_STORE_RESULT(dbh);
    EXPECT_EQ(res != NULL,true);
    if (res){
        auto rown = DBPROVIDER_NUM_ROWS(res);
        EXPECT_EQ(rown, 1);
        if (rown == 1){
            auto row = DBPROVIDER_FETCH_ROW(res);
            EXPECT_EQ(row!=NULL,true);
            if (row){
                EXPECT_EQ((uint64_t)strtoull(row[0], NULL, 10), (uint64_t)888888888888);
            }
        }
        DBPROVIDER_FREE_RESULT(res);
    }
    for(auto n = 0;n < nburst;n++){
        pgw_free_packet(pkts[n]);
    }
    // not found by imsi, being authenticated by msisdn
    ret = DBPROVIDER_QUERY(dbh, "DELETE FROM tunnel WHERE imsi IN( 240679077777777, 240679077777778)");
    EXPECT_EQ(ret, 0);
    ret = DBPROVIDER_QUERY(dbh,
                      "INSERT INTO tunnel(imsi, msisdn, ueipv4, pgw_teid, sgw_gtpu_teid, sgw_gtpu_ipv, active)VALUES "\
                      "(240679077777778,888888888888,'0.6.7.8',0xdead3456,0xdead4567,'192.168.71.69',1)");
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(event_rx(__inst, tpkt, sizeof(tpkt),  &saddr, sizeof(saddr), &caddr, sizeof(caddr), &srv), OK);
    EXPECT_EQ(event_gtpc_create_session_req(__inst, __inst->nodes[PGW_NODE_GTPC_CREATE_SESSION_REQ_EXT0]), OK);
    nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPC_CREATE_SESSION_REQ_EXT0], EGRESS, pkts, 32);
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
    EXPECT_EQ(parse_state.cause.cause, GTPC_CAUSE_REQUEST_ACCEPTED);
    EXPECT_EQ(gtpc_free_packet(&parsed_pkt), OK);
#if 1
    bzero(&saddr, sizeof(saddr));
    bzero(&caddr, sizeof(caddr));
    caddr.sin_family = AF_INET;
    caddr.sin_port = htons(2123);
    inet_pton(AF_INET, "8.8.8.8", &caddr.sin_addr.s_addr);

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    ret = pgw_send_sock(sock, &caddr, sizeof(struct sockaddr_in), tpkt, sizeof(tpkt));
    ret = pgw_send_sock(sock, &caddr, sizeof(struct sockaddr_in), pkts[0]->data, pkts[0]->datalen);
    close(sock);
#endif

    // imsi must be updated with msisdn
    // recovery counter must be saved.
    ret = DBPROVIDER_QUERY(dbh, "SELECT imsi,restart_counter FROM tunnel WHERE msisdn = 888888888888");
    res = DBPROVIDER_STORE_RESULT(dbh);
    EXPECT_EQ(res != NULL,true);
    if (res){
        auto rown = DBPROVIDER_NUM_ROWS(res);
        EXPECT_EQ(rown, 1);
        if (rown == 1){
            auto row = DBPROVIDER_FETCH_ROW(res);
            EXPECT_EQ(row!=NULL,true);
            if (row){
                EXPECT_EQ((uint64_t)strtoull(row[0], NULL, 10), (uint64_t)240679077777778);
                EXPECT_EQ((uint64_t)strtoull(row[1], NULL, 10), (uint64_t)3);
            }
        }
        DBPROVIDER_FREE_RESULT(res);
    }

    //
    for(auto n = 0;n < nburst;n++){
        pgw_free_packet(pkts[n]);
    }
    DBPROVIDER_CLOSE(dbh);
}


TEST(IntegrateTestPacketProcessing, GtpcEchoSgwPeer){
    U8  tpkt[] = {
        0x40,0x01,0x00,0x09,
        0x00,0x00,0x00,0x00,
        0x03,0x00,0x01,0x00,
        0x06
    };
    struct sockaddr_in  saddr, caddr;
    server_t    srv;
    packet_ptr  pkts[32] = {NULL};
    //
    bzero(&saddr, sizeof(saddr));
    bzero(&caddr, sizeof(caddr));
    bzero(&srv,   sizeof(srv));
    srv.port = GTPC_PORT;

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
                                 3306,
                                 NULL,
                                 0)==0,false);
    DBPROVIDER_SET_CHAR(dbh, "utf8");

    DBPROVIDER_QUERY(dbh, "DELETE FROM tunnel WHERE sgw_gtpc_ipv = '32.32.32.32'");
    DBPROVIDER_QUERY(dbh, "INSERT INTO tunnel(imsi, msisdn, ueipv4, pgw_teid, sgw_gtpu_teid, sgw_gtpu_ipv, sgw_gtpc_ipv, active)VALUES "\
                     "(54545454,54545454,'0.9.9.9',852963741,852963741,'54.54.54.54','32.32.32.32',1), "\
                     "(87878787,87878787,'0.9.9.1',147258369,147258369,'54.54.54.54','32.32.32.32',1), ");
    saddr.sin_addr.s_addr = 0x20202020;
    //
    EXPECT_EQ(event_rx(__inst, tpkt, sizeof(tpkt),  &saddr, sizeof(saddr), &caddr, sizeof(caddr), &srv), OK);
    EXPECT_EQ(event_gtpc_echo_req(__inst, __inst->nodes[PGW_NODE_GTPC_ECHO_REQ]), OK);
    auto nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPC_ECHO_REQ], EGRESS, pkts, 32);
    EXPECT_EQ(nburst>1, true);
    for(auto n = 0;n < nburst;n++){
        pgw_free_packet(pkts[n]);
    }
    // target tunnel must be disconnected.
    auto ret = DBPROVIDER_QUERY(dbh, "SELECT COUNT(1) FROM tunnel WHERE sgw_gtpc_ipv = '32.32.32.32' AND  active=0");
    auto res = DBPROVIDER_STORE_RESULT(dbh);
    EXPECT_EQ(res != NULL,true);
    if (res){
        auto rown = DBPROVIDER_NUM_ROWS(res);
        EXPECT_EQ(rown, 1);
        if (rown == 1){
            auto row = DBPROVIDER_FETCH_ROW(res);
            EXPECT_EQ(row!=NULL,true);
            if (row){
                EXPECT_EQ(atoi(row[0]), 0);
            }
        }
        DBPROVIDER_FREE_RESULT(res);
    }
    DBPROVIDER_CLOSE(dbh);
}


TEST(IntegrateTestPacketProcessing, Stop){
    EXPECT_EQ(pgw_stop(__inst, NULL, NULL), OK);
    EXPECT_EQ(pgw_release_instance(&__inst), OK);
}

TEST(IntegrateTestPacketProcessing, MemoryCheck){
    EXPECT_EQ(gtpc_memory_print(), OK);
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
            // test target is above 2 events
            break;
        default:
            pgw_panic("not implemented event.(%u)", event);
            break;
    }
    usleep(100000);
    return(OK);
}
