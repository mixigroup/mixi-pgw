#include "pgw_ext.h"
#include "node_ext.h"
#include "gtpc_ext.h"
#include "../src/pgw/pgw.h"
#include "gtest/gtest.h"

// instance
int PGW_LOG_LEVEL = 99;
pthread_mutex_t __mysql_mutex;

#define _TESTDB    ("test_gtpcv1")

#include "test_cmn_inc.inl"

static handle_ptr  __inst = NULL;
static server_ptr  __srvr = NULL;

static RETCD event_cb(EVENT , handle_ptr , evutil_socket_t , short , const U8* , const INT ,  struct sockaddr_in*, ssize_t , struct sockaddr_in* ,ssize_t , void* );

//
int main(int argc, char* argv[]){
    testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new CustomEnvironment());

    EXPECT_EQ(pgw_create_instance(SERVER, &__inst), OK);

    return RUN_ALL_TESTS();
}

TEST(Gtpcv1IntegrationTest, SetupDb){
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
                                 3306,
                                 NULL,
                                 0)==0,false);
    DBPROVIDER_SET_CHAR(dbh, "utf8");
    auto ret = DBPROVIDER_QUERY(dbh, "DELETE FROM tunnel WHERE msisdn = 817032924226");
    EXPECT_EQ(ret, 0);
#ifndef __USESQLITE3_ON_TEST__
    ret = DBPROVIDER_QUERY(dbh,
                      "INSERT INTO tunnel(imsi,msisdn,ueipv4,pgw_gtpu_ipv,pgw_gtpc_ipv,pgw_teid,sgw_gtpu_teid,sgw_gtpu_ipv,active,sgw_gtpc_teid)VALUES "\
        "(240679200134811,817032924226,'0.6.7.8','22.22.22.22','33.33.33.33',0x44333344,0xdeadc0de,'0.0.0.0',1,0) ON DUPLICATE KEY UPDATE active=VALUES(active)");
#else
    ret = DBPROVIDER_QUERY(dbh,
                      "INSERT OR IGNORE INTO tunnel(imsi,msisdn,ueipv4,pgw_gtpu_ipv,pgw_gtpc_ipv,pgw_teid,sgw_gtpu_teid,sgw_gtpu_ipv,active,sgw_gtpc_teid)VALUES "\
        "(240679200134811,817032924226,'0.6.7.8','22.22.22.22','33.33.33.33',0x44333344,0xdeadc0de,'0.0.0.0',1,0) ");
#endif
    EXPECT_EQ(ret, 0);
    DBPROVIDER_CLOSE(dbh);
}

//
TEST(Gtpcv1IntegrationTest, Start){
    U32 nodecnt = PGW_NODE_COUNT;
    U32 port = 3306;
    U32 opt = 0;
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
    usleep(1000000);
}



TEST(Gtpcv1IntegrationTest, CreatePDPContext){
    U8  tpkt[] = {
        0x32 ,                          // version.1
        0x10 ,                          // Create PDP Context Request
        0x00 ,0x68 ,
        0x00 ,0x00 ,0x00 ,0x00 ,
        0x0c ,0x01 ,                    // sequence number
        0x00 ,0x00 ,
        0x02 ,0x42 ,0x00 ,0x01, 0x21 ,0x43 ,0x65 ,0x87 ,0xf9 ,
        0x0e ,0x03 ,0x0f ,0x01 ,0x10 ,0x00 ,0x00 ,0x00 ,0x01 ,
        0x11 ,0xde ,0xad ,0xc0 ,0xde ,  // teid-ControlPlane
        0x14 ,0x00 ,0x1a ,0x08 ,0x00 ,0x80 ,0x00 ,0x02 ,0xf1 ,0x21 ,0x83 ,0x00 ,0x09,
        0x08 ,0x69 ,0x6e ,0x74 ,0x65 ,0x72 ,0x6e ,0x65 ,0x74 ,0x84 ,0x00 ,0x15 ,0x80 ,0xc0 ,0x23 ,0x11,
        0x01 ,0x01 ,0x00 ,0x11 ,0x03 ,0x6d ,0x69 ,0x67 ,0x08 ,0x68 ,0x65 ,0x6d ,0x6d ,0x65 ,0x6c ,0x69,
        0x67 ,0x85 ,0x00 ,0x04 ,0x7f ,0x00 ,0x00 ,0x02 ,0x85 ,0x00 ,0x04 ,0x7f ,0x00 ,0x00 ,0x02 ,0x86,
        0x00 ,0x07 ,0x91 ,0x64 ,0x07 ,0x12 ,0x32 ,0x54 ,0xf6 ,0x87 ,0x00 ,0x04 ,0x00 ,0x0b ,0x92 ,0x1f,
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

    EXPECT_EQ(event_gtpc_other_req(__inst, __inst->nodes[PGW_NODE_GTPC_OTHER_REQ]), OK);
    nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPC_OTHER_REQ], EGRESS, pkts, 32);
    EXPECT_EQ(nburst, 1);

    if (nburst == 1){
        auto p = (gtpc_v1_header_ptr)pkts[0]->data;
        EXPECT_EQ(p->f.gtpc_v1_flags.version, GTPC_VERSION_1);

        fprintf(stderr, "tid : %08x\n", p->tid);

        EXPECT_EQ(p->tid, htonl(0xdeadc0de));
        EXPECT_EQ(p->type, GTPC_V1_MSG_CREATE_PDP_CONTEXT_RES);
        auto seqno = htons(0x0c01);
        EXPECT_EQ(memcmp((p+1), &seqno, sizeof(seqno)), 0);
    }
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

TEST(Gtpcv1IntegrationTest, CreatePDPContext_SB){
    U8  tpkt[] = {
        0x32,                   // version.1
        0x10,                   // Create PDP Context Request
        0x00,0xe2,              // length
        0x00,0x00,0x00,0x00,    // teid
        0x8b,0x28,              // sequence number
        0x00,0x00,
        0x02,0x42,0x60,0x97,0x02,0x10,0x43,0x18,0xf1,   // imsi
        0x03,0x42,0xf0,0x76,0xff,0xfe,0xff,             // routing area
        0x0e,0x82,0x0f,0xfc,                            // recovery
        0x10,0x17,0x36,0x4e,0xb0,                       // teid : data i
        0x11,0x17,0x36,0x4e,0xae,                       // teid : control
        0x14,0x05,                                      // nsapi
        0x80,0x00,0x02,0xf1,0x21,                       // end user address
        0x83,0x00,0x0a,0x05,0x72,0x61,0x74,0x65,0x6c,0x03,0x63,0x6f,0x6d,
                                                        // access point name
        0x84,0x00,0x6b,0x80,0xc2,0x23,0x24,0x01,0x00,0x00,0x24,0x10,0x84,0x57,0xf2,0xf2,0x84,
        0x57,0xf2,0xf2,0x84,0x57,0xf2,0xf2,0x84,0x57,0xf2,0xf2,0x72,0x61,0x74,0x65,0x6c,0x40,
        0x72,0x61,0x74,0x65,0x6c,0x2e,0x63,0x6f,0x6d,0xc2,0x23,0x24,0x02,0x00,0x00,0x24,0x10,
        0xf6,0xd5,0xd8,0x98,0xaf,0x35,0x51,0x3b,0x8f,0x4f,0x3d,0x3d,0x56,0x23,0x8c,0x72,0x72,
        0x61,0x74,0x65,0x6c,0x40,0x72,0x61,0x74,0x65,0x6c,0x2e,0x63,0x6f,0x6d,0x80,0x21,0x10,
        0x01,0x00,0x00,0x10,0x81,0x06,0x00,0x00,0x00,0x00,0x83,0x06,0x00,0x00,0x00,0x00,0x00,
        0x0d,0x00,0x00,0x0a,0x00,0x00,0x05,0x00,
                                                        // pco
        0x85,0x00,0x04,0x71,0xd5,0x81,0xc1,             // sgsn address for signaling
        0x85,0x00,0x04,0x71,0xd5,0x81,0xc4,             // sgsn address for traffic
        0x86,0x00,0x07,0x91,0x18,0x07,0x23,0x29,0x24,0x62,  // msisdn
        0x87,0x00,0x0f,0x02,0x23,0x92,0x1f,0x93,0x96,0x68,0xfe,0x74,0xfb,0x10,0x40,0x00,0x64,0x00,
                                                        // qos
        0x97,0x00,0x01,0x01,                            // rat type
        0x99,0x00,0x02,0x63,0x20,                       // ms time zone
        0x9a,0x00,0x08,0x53,0x45,0x07,0x60,0x61,0x37,0x37,0x41,
                                                        // imei
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

    EXPECT_EQ(event_gtpc_other_req(__inst, __inst->nodes[PGW_NODE_GTPC_OTHER_REQ]), OK);
    nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPC_OTHER_REQ], EGRESS, pkts, 32);
    EXPECT_EQ(nburst, 1);

    if (nburst == 1){
        auto p = (gtpc_v1_header_ptr)pkts[0]->data;
        EXPECT_EQ(p->f.gtpc_v1_flags.version, GTPC_VERSION_1);
        EXPECT_EQ(p->tid, htonl(0x17364eae));
        EXPECT_EQ(p->type, GTPC_V1_MSG_CREATE_PDP_CONTEXT_RES);
        auto seqno = htons(0x8b28);
        EXPECT_EQ(memcmp((p+1), &seqno, sizeof(seqno)), 0);
    }
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

TEST(Gtpcv1IntegrationTest, CreatePDPContext_Exists){
    U8  tpkt[] = {
        0x32,                   // version.1
        0x10,                   // Create PDP Context Request
        0x00,0xe2,              // length
        0x00,0x00,0x00,0x00,    // teid
        0x8b,0x28,              // sequence number
        0x00,0x00,
        0x02,0x42,0x60,0x97,0x02,0x10,0x43,0x18,0xf1,   // imsi = 240679200134811
        0x03,0x42,0xf0,0x76,0xff,0xfe,0xff,             // routing area
        0x0e,0x82,0x0f,0xfc,                            // recovery
        0x10,0x17,0x36,0x4e,0xb0,                       // teid : data i
        0x11,0x17,0x36,0x4e,0xae,                       // teid : control
        0x14,0x05,                                      // nsapi
        0x80,0x00,0x02,0xf1,0x21,                       // end user address
        0x83,0x00,0x0a,0x05,0x72,0x61,0x74,0x65,0x6c,0x03,0x63,0x6f,0x6d,
        // access point name
        0x84,0x00,0x6b,0x80,0xc2,0x23,0x24,0x01,0x00,0x00,0x24,0x10,0x84,0x57,0xf2,0xf2,0x84,
        0x57,0xf2,0xf2,0x84,0x57,0xf2,0xf2,0x84,0x57,0xf2,0xf2,0x72,0x61,0x74,0x65,0x6c,0x40,
        0x72,0x61,0x74,0x65,0x6c,0x2e,0x63,0x6f,0x6d,0xc2,0x23,0x24,0x02,0x00,0x00,0x24,0x10,
        0xf6,0xd5,0xd8,0x98,0xaf,0x35,0x51,0x3b,0x8f,0x4f,0x3d,0x3d,0x56,0x23,0x8c,0x72,0x72,
        0x61,0x74,0x65,0x6c,0x40,0x72,0x61,0x74,0x65,0x6c,0x2e,0x63,0x6f,0x6d,0x80,0x21,0x10,
        0x01,0x00,0x00,0x10,0x81,0x06,0x00,0x00,0x00,0x00,0x83,0x06,0x00,0x00,0x00,0x00,0x00,
        0x0d,0x00,0x00,0x0a,0x00,0x00,0x05,0x00,
        // pco
        0x85,0x00,0x04,0x71,0xd5,0x81,0xc1,             // sgsn address for signaling
        0x85,0x00,0x04,0x71,0xd5,0x81,0xc4,             // sgsn address for traffic
        0x86,0x00,0x07,0x91,0x18,0x07,0x23,0x29,0x24,0x62,  // msisdn
        0x87,0x00,0x0f,0x02,0x23,0x92,0x1f,0x93,0x96,0x68,0xfe,0x74,0xfb,0x10,0x40,0x00,0x64,0x00,
        // qos
        0x97,0x00,0x01,0x01,                            // rat type
        0x99,0x00,0x02,0x63,0x20,                       // ms time zone
        0x9a,0x00,0x08,0x53,0x45,0x07,0x60,0x61,0x37,0x37,0x41,
        // imei
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

    EXPECT_EQ(event_gtpc_other_req(__inst, __inst->nodes[PGW_NODE_GTPC_OTHER_REQ]), OK);
    nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPC_OTHER_REQ], EGRESS, pkts, 32);
    EXPECT_EQ(nburst, 1);
    if (nburst == 1){
        auto p = (gtpc_v1_header_ptr)pkts[0]->data;
        EXPECT_EQ(p->f.gtpc_v1_flags.version, GTPC_VERSION_1);
        EXPECT_EQ(p->tid, htonl(0x17364eae));
        EXPECT_EQ(p->type, GTPC_V1_MSG_CREATE_PDP_CONTEXT_RES);
        auto seqno = htons(0x8b28);
        EXPECT_EQ(memcmp((p+1), &seqno, sizeof(seqno)), 0);
        U16 offset,extlen,datalen;
        U8 *payload,*datap;
        const struct messge_in_gtpc_v1* pmsg;
        gtpc_parse_state_t parse_state;

        // Create PDP Context must be success.
        offset = sizeof(gtpc_v1_header_t);
        if (p->f.gtpc_v1_flags.sequence ||
            p->f.gtpc_v1_flags.npdu ||
            p->f.gtpc_v1_flags.extension){
            offset += 4;
        }
        if (p->f.gtpc_v1_flags.extension){
            payload = ((U8*)p)+offset;
            for(;offset < pkts[0]->datalen;){
                extlen = (*(((U8*)p)+offset))<<2;
                offset += extlen;
                // next extension?
                if (*(((char*)p)+offset-1) == 0){
                    break;
                }
            }
        }
        auto pmsgs = gtpcv1_message_table();

        for(;offset < pkts[0]->datalen;){
            payload = ((U8*)p)+offset;
            pmsg = &pmsgs[(*payload)];
            //
            if (pmsg->type == 0){
                EXPECT_EQ(!"invalid pmsg->type", true);
            }
            if (pmsg->len <= 0){
                // variable length
                memcpy(&extlen, payload + sizeof(U8), sizeof(extlen));
                datalen = ntohs(extlen);
                extlen = ntohs(extlen) + sizeof(U8) + sizeof(U16);
                datap = payload;
            }else{
                // fixed length
                extlen = pmsg->len;
                datalen = extlen - sizeof(U8);
                datap = payload;
            }
            if (pmsg->parser != NULL){
                if (((iterate_func)(pmsg->parser))(pmsg->type, datap, datalen, &parse_state) != 0){
                    EXPECT_EQ(!"invalid parser", true);
                }
            }
            offset += extlen;
        }

        // assigned 33.33.33.33
        EXPECT_EQ(parse_state.flag_v1&HAVE_V1_IPADR_DATA,HAVE_V1_IPADR_DATA);
        EXPECT_EQ(parse_state.u_teid.teid_grekey,0x44333344);

    }
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



TEST(Gtpcv1IntegrationTest, CreatePDPContext_CSFB){
    U8  tpkt[] = {
        0x32,0x10,0x00,0xe5,    // gptc-v1
        0x00,0x00,0x00,0x00,    // teid
//      0x30,0x13,              // seq no
        0x8b,0x28,              // sequence number
        0x00,0x00,
//      0x02,0x42,0x60,0x87,0x91,0x54,0x85,0x88,0xf2,   // imsi
        0x02,0x42,0x60,0x97,0x02,0x10,0x43,0x18,0xf1,   // imsi = 240679200134811
        0x03,0x42,0xf0,0x76,0xff,0xfe,0xff,             // routing area identity
        0x0e,0x3d,0x0f,0xfc,                            // recovery
        0x10,0x3b,0x55,0xda,0x0f,                       // teid data i
        0x11,0x3b,0x55,0xda,0x0d,                       // teid control plane
        0x14,0x05,                                      // nsapi
        0x80,0x00,0x02,0xf1,0x21,                       // end user address.
        0x83,0x00,0x0a,0x05,0x72,0x61,0x74,0x65,0x6c,0x03,0x63,0x6f,0x6d,   // access point name.
        0x84,0x00,
        0x6e,0x80,0xc2,0x23,0x24,0x01,0x00,0x00,0x24,0x10,0x7e,0x42,0x96,0x96,0x7e,0x42,
        0x96,0x96,0x7e,0x42,0x96,0x96,0x7e,0x42,0x96,0x96,0x72,0x61,0x74,0x65,0x6c,0x40,
        0x72,0x61,0x74,0x65,0x6c,0x2e,0x63,0x6f,0x6d,0xc2,0x23,0x24,0x02,0x00,0x00,0x24,
        0x10,0x12,0x3b,0x59,0x6e,0xa3,0xe9,0x99,0x49,0xd4,0x8e,0xa0,0xf0,0x4d,0x96,0x82,
        0xe5,0x72,0x61,0x74,0x65,0x6c,0x40,0x72,0x61,0x74,0x65,0x6c,0x2e,0x63,0x6f,0x6d,
        0x80,0x21,0x10,0x01,0x00,0x00,0x10,0x81,0x06,0x00,0x00,0x00,0x00,0x83,0x06,0x00,
        0x00,0x00,0x00,0x00,0x0d,0x00,0x00,0x03,0x00,0x00,0x0a,0x00,0x00,0x05,0x00,
                                                        // pco
        0x85,0x00,0x04,0x71,0xd5,0x81,0xa1,             // GSN address
        0x85,0x00,0x04,0x71,0xd5,0x81,0xa7,             // GSN address
        0x86,0x00,0x07,0x91,0x18,0x07,0x31,0x81,0x19,0x77,  // MSISDN
        0x87,0x00,0x0f,0x02,0x23,0x92,0x1f,0x93,0x96,
        0x68,0xfe,0x74,0xfb,0x10,0x40,0x00,0x64,0x00,       // QOS
        0x97,0x00,0x01,0x01,                            // RAT Type
        0x99,0x00,0x02,0x63,0x20,                       // MS Time Zone
        0x9a,0x00,0x08,0x53,0x75,0x03,0x70,0x16,0x56,0x13,0x22, // IMEI
    };
    struct sockaddr_in  saddr, caddr;
    server_t    srv;
    bzero(&saddr, sizeof(saddr));
    bzero(&caddr, sizeof(caddr));
    bzero(&srv,   sizeof(srv));
    srv.port = GTPC_PORT;

    // validate db
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
    auto ret = DBPROVIDER_QUERY(dbh, "UPDATE tunnel SET rat = 6 WHERE imsi = 240679200134811");

    packet_ptr  pkts[32] = {NULL};
    auto nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPC_MODIFY_BEARER_REQ], INGRESS, pkts, 32);
    //
    EXPECT_EQ(nburst, 0);
    EXPECT_EQ(event_rx(__inst, tpkt, sizeof(tpkt),  &saddr, sizeof(saddr), &caddr, sizeof(caddr), &srv), OK);

    EXPECT_EQ(event_gtpc_other_req(__inst, __inst->nodes[PGW_NODE_GTPC_OTHER_REQ]), OK);
    nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPC_OTHER_REQ], EGRESS, pkts, 32);
    EXPECT_EQ(nburst, 1);
    if (nburst == 1){
        auto p = (gtpc_v1_header_ptr)pkts[0]->data;
        EXPECT_EQ(p->f.gtpc_v1_flags.version, GTPC_VERSION_1);
        EXPECT_EQ(p->tid, htonl(0x3b55da0d));
        EXPECT_EQ(p->type, GTPC_V1_MSG_CREATE_PDP_CONTEXT_RES);
        auto seqno = htons(0x8b28);
        EXPECT_EQ(memcmp((p+1), &seqno, sizeof(seqno)), 0);
        U16 offset,extlen,datalen;
        U8 *payload,*datap;
        const struct messge_in_gtpc_v1* pmsg;
        gtpc_parse_state_t parse_state;

        // Create PDP Context must be success.
        offset = sizeof(gtpc_v1_header_t);
        if (p->f.gtpc_v1_flags.sequence ||
            p->f.gtpc_v1_flags.npdu ||
            p->f.gtpc_v1_flags.extension){
            offset += 4;
        }
        if (p->f.gtpc_v1_flags.extension){
            payload = ((U8*)p)+offset;
            for(;offset < pkts[0]->datalen;){
                extlen = (*(((U8*)p)+offset))<<2;
                offset += extlen;
                // next extension?
                if (*(((char*)p)+offset-1) == 0){
                    break;
                }
            }
        }
        auto pmsgs = gtpcv1_message_table();

        for(;offset < pkts[0]->datalen;){
            payload = ((U8*)p)+offset;
            pmsg = &pmsgs[(*payload)];
            //
            if (pmsg->type == 0){
                EXPECT_EQ(!"invalid pmsg->type", true);
            }
            if (pmsg->len <= 0){
                // variable length
                memcpy(&extlen, payload + sizeof(U8), sizeof(extlen));
                datalen = ntohs(extlen);
                extlen = ntohs(extlen) + sizeof(U8) + sizeof(U16);
                datap = payload;
            }else{
                // fixed length
                extlen = pmsg->len;
                datalen = extlen - sizeof(U8);
                datap = payload;
            }
            if (pmsg->parser != NULL){
                if (((iterate_func)(pmsg->parser))(pmsg->type, datap, datalen, &parse_state) != 0){
                    EXPECT_EQ(!"invalid parser", true);
                }
            }
            offset += extlen;
        }

        // assigned 33.33.33.33
        EXPECT_EQ(parse_state.flag_v1&HAVE_V1_IPADR_DATA,HAVE_V1_IPADR_DATA);
        EXPECT_EQ(parse_state.u_teid.teid_grekey,0x44333344);

    }
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
    //
    for(auto n = 0;n < nburst;n++){
        pgw_free_packet(pkts[n]);
    }
    ret = DBPROVIDER_QUERY(dbh, "SELECT rat FROM tunnel WHERE imsi = 240679200134811");
    auto res = DBPROVIDER_STORE_RESULT(dbh);
    EXPECT_EQ(res != NULL,true);
    if (res){
        auto rown = DBPROVIDER_NUM_ROWS(res);
        EXPECT_EQ(rown, 1);
        if (rown == 1){
            auto row = DBPROVIDER_FETCH_ROW(res);
            EXPECT_EQ(row!=NULL,true);
            if (row){
                EXPECT_EQ(atoi(row[0]), (int)1);
            }
        }
        DBPROVIDER_FREE_RESULT(res);
    }
    DBPROVIDER_CLOSE(dbh);
}


TEST(Gtpcv1IntegrationTest, UpdatePDPContext_CSFB){
    U8  tpkt[] = {
        0x32,0x12,0x00,0x44,        // version 1
        0x44,0x33,0x33,0x44,        // teid 0x44333344
        0xdb,0xe3,0x00,0x00,        // sequence number.
        0x03,0x42,0xf0,0x76,0xff,0xfe,0xff, // Routing area identity.
        0x0e,0x3d,                  // recovery 61
        0x10,0x3b,0x56,0x8c,0x7b,   // teid data - i
        0x11,0x3b,0x56,0x8c,0x7d,   // teid control plane 0x3b568c7d
        0x14,0x05,                  // nsapi 5
        0x85,0x00,0x04,0x71,0xd5,0x81,0xa1, // ggsn address for control plane
        0x85,0x00,0x04,0x71,0xd5,0x81,0xa7, // ggsn address for user traffic
        0x87,0x00,0x11,0x02,0x23,0x92,0x1f,0x93,0x96,0xfe,0xfe,0x74,0xfb,0xff,0xff,0x00,0x6a,0x00,0x00,0x00,
                                    // qos
        0x97,0x00,0x01,0x01,        // rat type : utran
        0x99,0x00,0x02,0x63,0x20,   // MS time zone
    };
    struct sockaddr_in  saddr, caddr;
    server_t    srv;
    bzero(&saddr, sizeof(saddr));
    bzero(&caddr, sizeof(caddr));
    bzero(&srv,   sizeof(srv));
    srv.port = GTPC_PORT;


    // validate db
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
    auto ret = DBPROVIDER_QUERY(dbh, "UPDATE tunnel SET rat = 6 WHERE imsi = 240679200134811");


    packet_ptr  pkts[32] = {NULL};
    auto nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPC_MODIFY_BEARER_REQ], INGRESS, pkts, 32);
    //
    EXPECT_EQ(nburst, 0);
    EXPECT_EQ(event_rx(__inst, tpkt, sizeof(tpkt),  &saddr, sizeof(saddr), &caddr, sizeof(caddr), &srv), OK);
    EXPECT_EQ(event_gtpc_other_req(__inst, __inst->nodes[PGW_NODE_GTPC_OTHER_REQ]), OK);
    nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPC_OTHER_REQ], EGRESS, pkts, 32);
    EXPECT_EQ(nburst, 1);
    if (nburst == 1){
        auto p = (gtpc_v1_header_ptr)pkts[0]->data;
        EXPECT_EQ(p->f.gtpc_v1_flags.version, GTPC_VERSION_1);
        EXPECT_EQ(p->tid, htonl(0x3b568c7d));
        EXPECT_EQ(p->type, GTPC_V1_MSG_UPDATE_PDP_CONTEXT_RES);
        auto seqno = htons(0xdbe3);
        EXPECT_EQ(memcmp((p+1), &seqno, sizeof(seqno)), 0);
        U16 offset,extlen,datalen;
        U8 *payload,*datap;
        const struct messge_in_gtpc_v1* pmsg;
        gtpc_parse_state_t parse_state;

        // Update PDP Context must be suceeded.
        offset = sizeof(gtpc_v1_header_t);
        if (p->f.gtpc_v1_flags.sequence ||
            p->f.gtpc_v1_flags.npdu ||
            p->f.gtpc_v1_flags.extension){
            offset += 4;
        }
        if (p->f.gtpc_v1_flags.extension){
            payload = ((U8*)p)+offset;
            for(;offset < pkts[0]->datalen;){
                extlen = (*(((U8*)p)+offset))<<2;
                offset += extlen;
                // next extension?
                if (*(((char*)p)+offset-1) == 0){
                    break;
                }
            }
        }
        auto pmsgs = gtpcv1_message_table();

        for(;offset < pkts[0]->datalen;){
            payload = ((U8*)p)+offset;
            pmsg = &pmsgs[(*payload)];
            //
            if (pmsg->type == 0){
                EXPECT_EQ(!"invalid pmsg->type", true);
            }
            if (pmsg->len <= 0){
                // variable length
                memcpy(&extlen, payload + sizeof(U8), sizeof(extlen));
                datalen = ntohs(extlen);
                extlen = ntohs(extlen) + sizeof(U8) + sizeof(U16);
                datap = payload;
            }else{
                // fixed length
                extlen = pmsg->len;
                datalen = extlen - sizeof(U8);
                datap = payload;
            }
            if (pmsg->parser != NULL){
                if (((iterate_func)(pmsg->parser))(pmsg->type, datap, datalen, &parse_state) != 0){
                    EXPECT_EQ(!"invalid parser", true);
                }
            }
            offset += extlen;
        }
        // assigned pgw [c/u] teid
        EXPECT_EQ(parse_state.flag_v1&HAVE_V1_TEID_DATA,HAVE_V1_TEID_DATA);
//      EXPECT_EQ(parse_state.flag_v1&HAVE_V1_TEID_CTRL,HAVE_V1_TEID_CTRL);
        EXPECT_EQ(parse_state.u_teid.teid_grekey,0x44333344);
//      EXPECT_EQ(parse_state.c_teid.teid_grekey,0x44333344);
        // gtpu = 22.22.22.22/ gtpc = 33.33.33.33
        U32 cmpadr = 0x16161616;
        EXPECT_EQ(memcmp(parse_state.u_teid.blk, &cmpadr ,4),0);
        cmpadr = 0x21212121;
        EXPECT_EQ(memcmp(parse_state.c_teid.blk, &cmpadr ,4),0);
    }
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
    //
    for(auto n = 0;n < nburst;n++){
        pgw_free_packet(pkts[n]);
    }
    //
    ret = DBPROVIDER_QUERY(dbh, "SELECT rat FROM tunnel WHERE imsi = 240679200134811");
    auto res = DBPROVIDER_STORE_RESULT(dbh);
    EXPECT_EQ(res != NULL,true);
    if (res){
        auto rown = DBPROVIDER_NUM_ROWS(res);
        EXPECT_EQ(rown, 1);
        if (rown == 1){
            auto row = DBPROVIDER_FETCH_ROW(res);
            EXPECT_EQ(row!=NULL,true);
            if (row){
                EXPECT_EQ(atoi(row[0]), (int)1);
            }
        }
        DBPROVIDER_FREE_RESULT(res);
    }
    DBPROVIDER_CLOSE(dbh);
}


TEST(Gtpcv1IntegrationTest, DeletePDPContext_CSFB){
    U8  tpkt[] = {
        0x32,0x14,0x00,0x08,        // delete pdp
        0x44,0x33,0x33,0x44,        // teid 0x44333344
        0xdb,0xe7,0x00,0x00,        // sequence number
        0x13,0xff,                  // teardown indicator : true
        0x14,0x05,                  // nsapi : 5
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
    EXPECT_EQ(event_gtpc_other_req(__inst, __inst->nodes[PGW_NODE_GTPC_OTHER_REQ]), OK);
    nburst = pgw_deq_burst(__inst->nodes[PGW_NODE_GTPC_OTHER_REQ], EGRESS, pkts, 32);
    EXPECT_EQ(nburst, 1);
    if (nburst == 1){
        auto p = (gtpc_v1_header_ptr)pkts[0]->data;
        EXPECT_EQ(p->f.gtpc_v1_flags.version, GTPC_VERSION_1);
        EXPECT_EQ(p->tid, htonl(0x3b568c7d));
        EXPECT_EQ(p->type, GTPC_V1_MSG_DELETE_PDP_CONTEXT_RES);
        auto seqno = htons(0xdbe7);
        EXPECT_EQ(memcmp((p+1), &seqno, sizeof(seqno)), 0);
        U16 offset,extlen,datalen;
        U8 *payload,*datap;
        const struct messge_in_gtpc_v1* pmsg;
        gtpc_parse_state_t parse_state;

        // Update PDP Context must be succeeded.
        offset = sizeof(gtpc_v1_header_t);
        if (p->f.gtpc_v1_flags.sequence ||
            p->f.gtpc_v1_flags.npdu ||
            p->f.gtpc_v1_flags.extension){
            offset += 4;
        }
        if (p->f.gtpc_v1_flags.extension){
            payload = ((U8*)p)+offset;
            for(;offset < pkts[0]->datalen;){
                extlen = (*(((U8*)p)+offset))<<2;
                offset += extlen;
                // next extension?
                if (*(((char*)p)+offset-1) == 0){
                    break;
                }
            }
        }
        auto pmsgs = gtpcv1_message_table();

        for(;offset < pkts[0]->datalen;){
            payload = ((U8*)p)+offset;
            pmsg = &pmsgs[(*payload)];
            //
            if (pmsg->type == 0){
                EXPECT_EQ(!"invalid pmsg->type", true);
            }
            if (pmsg->len <= 0){
                // variable length
                memcpy(&extlen, payload + sizeof(U8), sizeof(extlen));
                datalen = ntohs(extlen);
                extlen = ntohs(extlen) + sizeof(U8) + sizeof(U16);
                datap = payload;
            }else{
                // fixed length
                extlen = pmsg->len;
                datalen = extlen - sizeof(U8);
                datap = payload;
            }
            if (pmsg->parser != NULL){
                if (((iterate_func)(pmsg->parser))(pmsg->type, datap, datalen, &parse_state) != 0){
                    EXPECT_EQ(!"invalid parser", true);
                }
            }
            offset += extlen;
        }
        EXPECT_EQ(parse_state.flag_v1&HAVE_V1_CAUSE,HAVE_V1_CAUSE);
        EXPECT_EQ(parse_state.cause_v1, GTPC_V1_CAUSE_ACCEPTED);
    }
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


TEST(Gtpcv1IntegrationTest, Stop){
    EXPECT_EQ(pgw_stop(__inst, NULL, NULL), OK);
    EXPECT_EQ(pgw_release_instance(&__inst), OK);
}
TEST(Gtpcv1IntegrationTest, MemoryCheck){
    EXPECT_EQ(gtpc_memory_print(), OK);

    gtpc_v1_header_t    itm;
    bzero(&itm, sizeof(itm));
    itm.f.gtpc_v1_flags.sequence = 1;
    EXPECT_NE(itm.f.flags&GTPC_V1_NSE_MASK,0);
    bzero(&itm, sizeof(itm));
    itm.f.gtpc_v1_flags.npdu = 1;
    EXPECT_NE(itm.f.flags&GTPC_V1_NSE_MASK,0);
    bzero(&itm, sizeof(itm));
    itm.f.gtpc_v1_flags.extension = 1;
    EXPECT_NE(itm.f.flags&GTPC_V1_NSE_MASK,0);
    sleep(1);
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
