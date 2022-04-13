#include "pgw_ext.h"
#include "node_ext.h"
#include "gtpc_ext.h"
#include "gtest/gtest.h"

// instance
int PGW_LOG_LEVEL = PGW_LOG_LEVEL_ALL;

//
class CustomEnvironment :public ::testing::Environment {
public:
    virtual ~CustomEnvironment() {}
    virtual void SetUp() { }
    virtual void TearDown() { }
};
//
int main(int argc, char* argv[]){
    testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new CustomEnvironment());
    return RUN_ALL_TESTS();
}
// basic. malloc / free
TEST(GtpcParseTest, MallocFree){
    for(auto n = 0; n < 1000; n++){
        gtp_packet_ptr  pkt = NULL;
        //
        EXPECT_EQ(gtpc_alloc_packet(&pkt, GTPC_CREATE_SESSION_REQ), OK);
        EXPECT_EQ(pkt!=NULL,true);
        EXPECT_EQ(gtpc_free_packet(&pkt), OK);

        auto mc = (U32)gtpc_memory_status();
        EXPECT_EQ(mc, (U32)0);
    }
}
//
TEST(GtpcParseTest, GenerateSimple){
    gtp_packet_ptr  pkt = NULL;
    EXPECT_EQ(gtpc_alloc_packet(&pkt, GTPC_CREATE_SESSION_REQ), OK);
    EXPECT_EQ(pkt!=NULL,true);
    if (!pkt){ return; }
    //
    gtpc_header_t   gtpch;
    bzero(&gtpch, sizeof(gtpch));
    for(auto n = 0;n < sizeof(gtpch);n++){
        ((U8*)&gtpch)[n] = (U8)n;
    }
    EXPECT_EQ(gtpc_append_item(pkt, GTPC_TYPE_IMSI, (U8*)&gtpch, sizeof(gtpch) - 4), OK);
    // ether packet size = 1514 <= (12 + 12 * 125) + 2
    for(auto n = 0;n < 124;n++){
        // must be able to add
        EXPECT_EQ(gtpc_append_item(pkt, GTPC_TYPE_IMSI, (U8*)&gtpch, sizeof(gtpch)), OK);
    }
    // must be error.
    EXPECT_EQ(gtpc_append_item(pkt, GTPC_TYPE_IMSI, (U8*)&gtpch, sizeof(gtpch)), ERR);
    gtpc_packet_print(pkt);
    //
    EXPECT_EQ(gtpc_free_packet(&pkt), OK);
}
//
TEST(GtpcParseTest, CheckGtpcHeaderLength){
    gtp_packet_ptr  pkt = NULL;
    EXPECT_EQ(gtpc_alloc_packet(&pkt, GTPC_CREATE_SESSION_REQ), OK);
    EXPECT_EQ(pkt!=NULL,true);
    if (!pkt){ return; }
    //
    gtpc_header_t   gtpch;
    gtpc_header_ptr pgtpch;
    bzero(&gtpch, sizeof(gtpch));
    for(auto n = 0;n < sizeof(gtpch);n++){
        ((U8*)&gtpch)[n] = (U8)n;
    }
    for(auto n = 0;n < 4;n++){
        EXPECT_EQ(gtpc_append_item(pkt, GTPC_TYPE_IMSI, (U8*)&gtpch, sizeof(gtpch)), OK);
    }
    // added 12 x 4 item ,  , 
    // header length of gtp-cpacket is 12 + (12*4) = 60 - 4 => 56
    EXPECT_EQ(ntohs(gtpc_header_(pkt)->length), 56);

    gtpc_packet_print(pkt);
    // initialize packet
    EXPECT_EQ(gtpc_free_packet(&pkt), OK);
    EXPECT_EQ(gtpc_alloc_packet(&pkt, GTPC_CREATE_SESSION_REQ), OK);

    gtpc_packet_print(pkt);
    //
    EXPECT_EQ(gtpc_free_packet(&pkt), OK);
}

struct _fteid_test{
    U8      inst,iftype;
    U32     teid;
    U8      ip[4];
};

struct _bqos_test{
    U8      flag,qci;
    U64     m_upl,m_dwl,g_upl,g_dwl;
};

struct _bctx_test{
    gtpc_f_teid_t       fteid;
    gtpc_ebi_t          ebi;
    gtpc_bearer_qos_t   bqos;
};


//
TEST(GtpcParseTest, Imsi){
    gtp_packet_ptr  pkt = NULL,parsed_pkt = NULL;
    EXPECT_EQ(gtpc_alloc_packet(&pkt, GTPC_CREATE_SESSION_REQ), OK);
    EXPECT_EQ(pkt!=NULL,true);
    if (!pkt){ return; }
    //
    U64 imsi_num = 175675478978012;
    gtpc_imsi_t imsi;
    //
    EXPECT_EQ(gtpc_imsi_set(&imsi, imsi_num), OK);
    EXPECT_EQ(gtpc_append_item(pkt, GTPC_TYPE_IMSI, (U8*)&imsi, sizeof(imsi)), OK);
    //
    gtpc_packet_print(pkt);

    // attach to packet
    auto gtpch = gtpc_header_(pkt);
    // packet = header:12 + imsi:12 = 24
    EXPECT_EQ(gtpc_alloc_packet_from_payload(&parsed_pkt,(U8*)gtpch, 24), OK);

    gtpc_parse_state_t  state;
    // iteration and imsi item should be included.
    gtpc_iterate_item(parsed_pkt,on_gtpc_parse_root,&state);
    EXPECT_EQ(state.flag&HAVE_IMSI, HAVE_IMSI);
    EXPECT_EQ(gtpc_digits_get(state.imsi.digits, GTPC_IMSI_LEN), imsi_num);
    //
    EXPECT_EQ(gtpc_free_packet(&pkt), OK);
    EXPECT_EQ(gtpc_free_packet(&parsed_pkt), OK);
}
//
TEST(GtpcParseTest, Recovery){
    gtp_packet_ptr  pkt = NULL,parsed_pkt = NULL;
    EXPECT_EQ(gtpc_alloc_packet(&pkt, GTPC_CREATE_SESSION_REQ), OK);
    EXPECT_EQ(pkt!=NULL,true);
    if (!pkt){ return; }
    //
    gtpc_recovery_t recovery;
    U8  recovery_cnt = 0xfa;
    //
    EXPECT_EQ(gtpc_recovery_set(&recovery, recovery_cnt), OK);
    EXPECT_EQ(gtpc_append_item(pkt, GTPC_TYPE_RECOVERY, (U8*)&recovery, sizeof(recovery)), OK);

    // attach to packet
    auto gtpch = gtpc_header_(pkt);
    EXPECT_EQ(gtpc_alloc_packet_from_payload(&parsed_pkt,(U8*)gtpc_header_(pkt), pkt->offset), OK);
    gtpc_parse_state_t  state;
    // iteration and recovery item should be included.
    gtpc_iterate_item(parsed_pkt,on_gtpc_parse_root,&state);
    EXPECT_EQ(state.flag&HAVE_RECOVERY, HAVE_RECOVERY);
    EXPECT_EQ(state.recovery.recovery_restart_counter, recovery_cnt);
    //
    EXPECT_EQ(gtpc_free_packet(&pkt), OK);
    EXPECT_EQ(gtpc_free_packet(&parsed_pkt), OK);
}
//
TEST(GtpcParseTest, Cause){
    gtp_packet_ptr  pkt = NULL,parsed_pkt = NULL;
    EXPECT_EQ(gtpc_alloc_packet(&pkt, GTPC_CREATE_SESSION_RES), OK);
    EXPECT_EQ(pkt!=NULL,true);
    if (!pkt){ return; }
    //
    gtpc_cause_t cause;
    U8 cause_code = 16;
    //
    EXPECT_EQ(gtpc_cause_set(&cause, cause_code), OK);
    EXPECT_EQ(gtpc_append_item(pkt, GTPC_TYPE_CAUSE, (U8*)&cause, gtpc_cause_length(&cause)), OK);

    // attach to packet
    auto gtpch = gtpc_header_(pkt);
    EXPECT_EQ(gtpc_alloc_packet_from_payload(&parsed_pkt,(U8*)gtpc_header_(pkt), pkt->offset), OK);

    gtpc_parse_state_t  state;
    // iteration and cause item should be included
    gtpc_iterate_item(parsed_pkt,on_gtpc_parse_root,&state);
    EXPECT_EQ(state.flag&HAVE_CAUSE, HAVE_CAUSE);
    EXPECT_EQ(state.cause.cause, cause_code);
    //
    EXPECT_EQ(gtpc_free_packet(&pkt), OK);
    EXPECT_EQ(gtpc_free_packet(&parsed_pkt), OK);
}
//
TEST(GtpcParseTest, Msisdn){
    gtp_packet_ptr  pkt = NULL,parsed_pkt = NULL;
    EXPECT_EQ(gtpc_alloc_packet(&pkt, GTPC_CREATE_SESSION_REQ), OK);
    EXPECT_EQ(pkt!=NULL,true);
    if (!pkt){ return; }
    //
    U64 msisdn_num = 987654321654;

    gtpc_msisdn_t msisdn;
    //
    EXPECT_EQ(gtpc_msisdn_set(&msisdn, msisdn_num), OK);
    EXPECT_EQ(gtpc_append_item(pkt, GTPC_TYPE_MSISDN, (U8*)&msisdn, sizeof(msisdn)), OK);
    //
    gtpc_packet_print(pkt);

    // attach to packet
    auto gtpch = gtpc_header_(pkt);
    //
    EXPECT_EQ(gtpc_alloc_packet_from_payload(&parsed_pkt,(U8*)gtpch, pkt->offset), OK);

    gtpc_parse_state_t  state;
    // iteration  msisdn  item  should be included
    gtpc_iterate_item(parsed_pkt,on_gtpc_parse_root, &state);
    EXPECT_EQ(state.flag&HAVE_MSISDN, HAVE_MSISDN);
    EXPECT_EQ(gtpc_digits_get(state.msisdn.digits,GTPC_MSISDN_LEN), msisdn_num);
    //
    EXPECT_EQ(gtpc_free_packet(&pkt), OK);
    EXPECT_EQ(gtpc_free_packet(&parsed_pkt), OK);
}
//
TEST(GtpcParseTest, Mei){
    gtp_packet_ptr  pkt = NULL,parsed_pkt = NULL;
    EXPECT_EQ(gtpc_alloc_packet(&pkt, GTPC_CREATE_SESSION_REQ), OK);
    EXPECT_EQ(pkt!=NULL,true);
    if (!pkt){ return; }
    //
    U64 mei_num = 665544339988;

    gtpc_mei_t mei;
    //
    EXPECT_EQ(gtpc_mei_set(&mei, mei_num), OK);
    EXPECT_EQ(gtpc_append_item(pkt, GTPC_TYPE_MEI, (U8*)&mei, sizeof(mei)), OK);
    //
    gtpc_packet_print(pkt);

    // attach to packet
    auto gtpch = gtpc_header_(pkt);
    EXPECT_EQ(gtpc_alloc_packet_from_payload(&parsed_pkt,(U8*)gtpch, pkt->offset), OK);

    gtpc_parse_state_t  state;
    //iteration , mei  item should be included.
    gtpc_iterate_item(parsed_pkt,on_gtpc_parse_root, &state);
    EXPECT_EQ(state.flag&HAVE_MEI, HAVE_MEI);
    EXPECT_EQ(gtpc_digits_get(state.mei.digits,GTPC_MEI_LEN), mei_num);
    //
    EXPECT_EQ(gtpc_free_packet(&pkt), OK);
    EXPECT_EQ(gtpc_free_packet(&parsed_pkt), OK);
}
//
TEST(GtpcParseTest, Fteid){
    gtp_packet_ptr  pkt = NULL,parsed_pkt = NULL;
    EXPECT_EQ(gtpc_alloc_packet(&pkt, GTPC_CREATE_SESSION_REQ), OK);
    EXPECT_EQ(pkt!=NULL,true);
    if (!pkt){ return; }

    struct _fteid_test  fteid_test;
    //
    fteid_test.inst      = 1;
    fteid_test.iftype    = 2;
    fteid_test.teid      = 98765;
    fteid_test.ip[0]     = 10;
    fteid_test.ip[1]     = 11;
    fteid_test.ip[2]     = 12;
    fteid_test.ip[3]     = 13;

    gtpc_f_teid_t fteid;
    //
    EXPECT_EQ(gtpc_f_teid_set(&fteid, fteid_test.inst,fteid_test.iftype,ntohl(fteid_test.teid),fteid_test.ip,NULL), OK);
    // ipv4 : 13/ipv6 : 25
    EXPECT_EQ(gtpc_append_item(pkt, GTPC_TYPE_F_TEID, (U8*)&fteid, 13), OK);
    gtpc_packet_print(pkt);

    // attach to packet
    auto gtpch = gtpc_header_(pkt);
    EXPECT_EQ(gtpc_alloc_packet_from_payload(&parsed_pkt,(U8*)gtpch, pkt->offset), OK);

    gtpc_parse_state_t  state;
    //iteration , fteid  item should be included.
    gtpc_iterate_item(parsed_pkt,on_gtpc_parse_root, &state);
    EXPECT_EQ(state.flag&HAVE_FTEID_C, HAVE_FTEID_C);
    EXPECT_EQ(state.c_teid.teid_grekey, fteid_test.teid);
    EXPECT_EQ(memcmp(state.c_teid.blk, fteid_test.ip, sizeof(fteid_test.ip)),0);
    EXPECT_EQ(state.c_teid.head.inst.instance, fteid_test.inst);
    EXPECT_EQ(state.c_teid.bit.iftype, fteid_test.iftype);
    //
    EXPECT_EQ(gtpc_free_packet(&pkt), OK);
    EXPECT_EQ(gtpc_free_packet(&parsed_pkt), OK);
}
//
TEST(GtpcParseTest, Ambr){
    gtp_packet_ptr  pkt = NULL,parsed_pkt = NULL;
    EXPECT_EQ(gtpc_alloc_packet(&pkt, GTPC_CREATE_SESSION_REQ), OK);
    EXPECT_EQ(pkt!=NULL,true);
    if (!pkt){ return; }
    //
    U32 ambr_num = 55555;
    gtpc_ambr_t ambr;
    //
    EXPECT_EQ(gtpc_ambr_set(&ambr, ambr_num, ambr_num + 2), OK);
    EXPECT_EQ(gtpc_append_item(pkt, GTPC_TYPE_AMBR, (U8*)&ambr, sizeof(ambr)), OK);
    //
    gtpc_packet_print(pkt);

    // attach to packet
    auto gtpch = gtpc_header_(pkt);
    EXPECT_EQ(gtpc_alloc_packet_from_payload(&parsed_pkt,(U8*)gtpch, pkt->offset), OK);

    gtpc_parse_state_t  state;
    //iteration , msisdn  item should be included.
    gtpc_iterate_item(parsed_pkt,on_gtpc_parse_root, &state);
    EXPECT_EQ(state.flag&HAVE_AMBR_R, HAVE_AMBR_R);
    EXPECT_EQ(ntohl(state.ambr_r.uplink), ambr_num);
    EXPECT_EQ(ntohl(state.ambr_r.downlink), ambr_num + 2);
    //
    EXPECT_EQ(gtpc_free_packet(&pkt), OK);
    EXPECT_EQ(gtpc_free_packet(&parsed_pkt), OK);
}
//
TEST(GtpcParseTest, ChargingId){
    gtp_packet_ptr  pkt = NULL,parsed_pkt = NULL;
    EXPECT_EQ(gtpc_alloc_packet(&pkt, GTPC_CREATE_SESSION_REQ), OK);
    EXPECT_EQ(pkt!=NULL,true);
    if (!pkt){ return; }
    //
    U32 charging_num = 987654;
    gtpc_charging_id_t chrg;
    //
    EXPECT_EQ(gtpc_charging_id_set(&chrg, charging_num), OK);
    EXPECT_EQ(gtpc_append_item(pkt, GTPC_TYPE_CHARGING_ID, (U8*)&chrg, sizeof(chrg)), OK);
    //
    gtpc_packet_print(pkt);

    // attach to packet
    auto gtpch = gtpc_header_(pkt);
    EXPECT_EQ(gtpc_alloc_packet_from_payload(&parsed_pkt,(U8*)gtpch, pkt->offset), OK);

    gtpc_parse_state_t  state;

    //iteration , charging id  item should be included.
    gtpc_iterate_item(parsed_pkt,on_gtpc_parse_root, &state);
    EXPECT_EQ(state.flag&HAVE_CHRGID, HAVE_CHRGID);
    EXPECT_EQ(ntohl(state.charg.charging_id), charging_num);
    //
    EXPECT_EQ(gtpc_free_packet(&pkt), OK);
    EXPECT_EQ(gtpc_free_packet(&parsed_pkt), OK);
}
//
TEST(GtpcParseTest, Ebi){
    gtp_packet_ptr  pkt = NULL,parsed_pkt = NULL;
    EXPECT_EQ(gtpc_alloc_packet(&pkt, GTPC_CREATE_SESSION_REQ), OK);
    EXPECT_EQ(pkt!=NULL,true);
    if (!pkt){ return; }
    //
    U8 ebi_num = 0x2;
    gtpc_ebi_t ebi;
    //
    EXPECT_EQ(gtpc_ebi_set(&ebi, ebi_num, ebi_num + 1), OK);
    EXPECT_EQ(gtpc_append_item(pkt, GTPC_TYPE_EBI, (U8*)&ebi, sizeof(ebi)), OK);
    //
    gtpc_packet_print(pkt);

    // attach to packet
    auto gtpch = gtpc_header_(pkt);
    EXPECT_EQ(gtpc_alloc_packet_from_payload(&parsed_pkt,(U8*)gtpch, pkt->offset), OK);

    gtpc_parse_state_t  state;

    //iteration , ebi  item should be included.
    gtpc_iterate_item(parsed_pkt,on_gtpc_parse_root, &state);

    EXPECT_EQ(state.flag&HAVE_EBI, HAVE_EBI);
    EXPECT_EQ(state.ebi_r.head.inst.instance, ebi_num);
    EXPECT_EQ(state.ebi_r.ebi.low, ebi_num + 1);
    //
    EXPECT_EQ(gtpc_free_packet(&pkt), OK);
    EXPECT_EQ(gtpc_free_packet(&parsed_pkt), OK);
}
//
TEST(GtpcParseTest, Pco){
    gtp_packet_ptr  pkt = NULL,parsed_pkt = NULL;
    EXPECT_EQ(gtpc_alloc_packet(&pkt, GTPC_CREATE_SESSION_REQ), OK);
    EXPECT_EQ(pkt!=NULL,true);
    if (!pkt){ return; }
    //
    U64 pco_val = 0x7f1232458799aabb;
    gtpc_pco_t pco;
    //
    EXPECT_EQ(gtpc_pco_set(&pco, (U8*)&pco_val, sizeof(pco_val)+1), OK);
    EXPECT_EQ(gtpc_append_item(pkt, GTPC_TYPE_PCO, (U8*)&pco, gtpc_pco_length(&pco)), OK);
    //
    gtpc_packet_print(pkt);

    // attach to packet
    auto gtpch = gtpc_header_(pkt);
    EXPECT_EQ(gtpc_alloc_packet_from_payload(&parsed_pkt,(U8*)gtpch, pkt->offset), OK);

    gtpc_parse_state_t  state;
    bzero(&state, sizeof(state));

    //iteration , pco  item should be included.
    gtpc_iterate_item(parsed_pkt,on_gtpc_parse_root, &state);
    EXPECT_EQ(state.flag&HAVE_PCO, HAVE_PCO);
    EXPECT_EQ(memcmp(state.pco.pco, &pco_val, 8), 0);
    //
    EXPECT_EQ(gtpc_free_packet(&pkt), OK);
    EXPECT_EQ(gtpc_free_packet(&parsed_pkt), OK);
}
//
TEST(GtpcParseTest, Paa){
    gtp_packet_ptr  pkt = NULL,parsed_pkt = NULL;
    EXPECT_EQ(gtpc_alloc_packet(&pkt, GTPC_CREATE_SESSION_REQ), OK);
    EXPECT_EQ(pkt!=NULL,true);
    if (!pkt){ return; }
    //
    U64 paa_val = 0x1020789789789789;
    gtpc_paa_t paa;
    //
    EXPECT_EQ(gtpc_paa_set(&paa, 3, (U8*)&paa_val, 9), OK);
    EXPECT_EQ(gtpc_append_item(pkt, GTPC_TYPE_PAA, (U8*)&paa, gtpc_paa_length(&paa)), OK);
    //
    gtpc_packet_print(pkt);

    // attach to packet
    auto gtpch = gtpc_header_(pkt);
    EXPECT_EQ(gtpc_alloc_packet_from_payload(&parsed_pkt,(U8*)gtpch, pkt->offset), OK);
    gtpc_parse_state_t  state;

    //iteration , paa  item should be included.
    gtpc_iterate_item(parsed_pkt,on_gtpc_parse_root, &state);
    EXPECT_EQ(state.flag&HAVE_PAA, HAVE_PAA);
    EXPECT_EQ(memcmp(state.paa.paa, &paa_val, 8), 0);
    //
    EXPECT_EQ(gtpc_free_packet(&pkt), OK);
    EXPECT_EQ(gtpc_free_packet(&parsed_pkt), OK);
}
//
TEST(GtpcParseTest, ApnRestriction){
    gtp_packet_ptr  pkt = NULL,parsed_pkt = NULL;
    EXPECT_EQ(gtpc_alloc_packet(&pkt, GTPC_CREATE_SESSION_REQ), OK);
    EXPECT_EQ(pkt!=NULL,true);
    if (!pkt){ return; }
    //
    U8 apn_r_type = 0x1;
    gtpc_apn_restriction_t apnr;
    //
    EXPECT_EQ(gtpc_apn_restriction_set(&apnr, apn_r_type), OK);
    EXPECT_EQ(gtpc_append_item(pkt, GTPC_TYPE_APN_RESTRICTION, (U8*)&apnr, sizeof(apnr)), OK);
    //
    gtpc_packet_print(pkt);

    // attach to packet
    auto gtpch = gtpc_header_(pkt);
    EXPECT_EQ(gtpc_alloc_packet_from_payload(&parsed_pkt,(U8*)gtpch, pkt->offset), OK);
    gtpc_parse_state_t  state;

    //iteration , apn restriction  item should be included.
    gtpc_iterate_item(parsed_pkt,on_gtpc_parse_root, &state);
    EXPECT_EQ(state.flag&HAVE_APN_R, HAVE_APN_R);
    EXPECT_EQ(state.apn_r.restriction_type, apn_r_type);
    //
    EXPECT_EQ(gtpc_free_packet(&pkt), OK);
    EXPECT_EQ(gtpc_free_packet(&parsed_pkt), OK);
}
//
TEST(GtpcParseTest, BearerQos){
    gtp_packet_ptr  pkt = NULL,parsed_pkt = NULL;
    EXPECT_EQ(gtpc_alloc_packet(&pkt, GTPC_CREATE_SESSION_REQ), OK);
    EXPECT_EQ(pkt!=NULL,true);
    if (!pkt){ return; }
    //
    struct _bqos_test   bqos;
    bqos.flag   = 0;
    bqos.qci    = 1;
    bqos.m_upl  = 123;
    bqos.m_dwl  = 234;
    bqos.g_upl  = 345;
    bqos.g_dwl  = 456;
    //
    gtpc_bearer_qos_t bq;
    //
    EXPECT_EQ(gtpc_gtpc_bearer_qos_set(&bq,
                                       bqos.flag,
                                       bqos.qci,
                                       bqos.m_upl,
                                       bqos.m_dwl,
                                       bqos.g_upl,
                                       bqos.g_dwl
    ), OK);
    EXPECT_EQ(gtpc_append_item(pkt, GTPC_TYPE_BEARER_QOS, (U8*)&bq, sizeof(bq)), OK);
    //
    gtpc_packet_print(pkt);
    // attach to packet
    auto gtpch = gtpc_header_(pkt);
    EXPECT_EQ(gtpc_alloc_packet_from_payload(&parsed_pkt,(U8*)gtpch, pkt->offset), OK);
    gtpc_parse_state_t  state;

    //iteration , bqos  item should be included.
    gtpc_iterate_item(parsed_pkt,on_gtpc_parse_root, &state);

    EXPECT_EQ(state.flag&HAVE_BQOS_R, HAVE_BQOS_R);
    uint64_t val(0);
    //
    EXPECT_EQ(state.bqos_r.qci, bqos.qci);
    //
    val = 0;
    memcpy(&((char*)&val)[3],&state.bqos_r.rate[0],5);
    EXPECT_EQ(XNTOHLL(val), bqos.m_upl);
    val = 0;
    memcpy(&((char*)&val)[3],&state.bqos_r.rate[5],5);
    EXPECT_EQ(XNTOHLL(val), bqos.m_dwl);
    val = 0;
    memcpy(&((char*)&val)[3],&state.bqos_r.rate[10],5);
    EXPECT_EQ(XNTOHLL(val), bqos.g_upl);
    val = 0;
    memcpy(&((char*)&val)[3],&state.bqos_r.rate[15],5);
    EXPECT_EQ(XNTOHLL(val), bqos.g_dwl);
    //
    EXPECT_EQ(gtpc_free_packet(&pkt), OK);
    EXPECT_EQ(gtpc_free_packet(&parsed_pkt), OK);
}


//
TEST(GtpcParseTest, BearerContext){
    gtp_packet_ptr  pkt = NULL,parsed_pkt = NULL;
    EXPECT_EQ(gtpc_alloc_packet(&pkt, GTPC_CREATE_SESSION_REQ), OK);
    EXPECT_EQ(pkt!=NULL,true);
    if (!pkt){ return; }

    struct _bctx_test   bctx_test;
    U8      ipv6[16] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};


    bzero(&bctx_test, sizeof(bctx_test));
    //
    gtpc_bearer_ctx_t bctx;
    // bearer
    EXPECT_EQ(gtpc_bearer_ctx_set(&bctx), OK);
    //      - fteid
    EXPECT_EQ(gtpc_f_teid_set(&bctx_test.fteid, 1, 2, 3, NULL, ipv6), OK);
    //      - ebi
    EXPECT_EQ(gtpc_ebi_set(&bctx_test.ebi, 1, 2), OK);
    //      - bearer qos
    EXPECT_EQ(gtpc_gtpc_bearer_qos_set(&bctx_test.bqos, 1, 2, 3, 4, 5, 6), OK);
    // add child item to bearer
    EXPECT_EQ(gtpc_bearer_ctx_add_child(&bctx, (U8*)&bctx_test.fteid, gtpc_f_teid_length(&bctx_test.fteid)), OK);
    EXPECT_EQ(gtpc_bearer_ctx_add_child(&bctx, (U8*)&bctx_test.ebi, sizeof(bctx_test.ebi)), OK);
    EXPECT_EQ(gtpc_bearer_ctx_add_child(&bctx, (U8*)&bctx_test.bqos, sizeof(bctx_test.bqos)), OK);


    EXPECT_EQ(gtpc_append_item(pkt, GTPC_TYPE_BEARER_CTX, (U8*)&bctx, gtpc_bearer_ctx_length(&bctx)), OK);
    //
    gtpc_packet_print(pkt);

    // attach to packet
    auto gtpch = gtpc_header_(pkt);
    EXPECT_EQ(gtpc_alloc_packet_from_payload(&parsed_pkt,(U8*)gtpch, pkt->offset), OK);

    gtpc_parse_state_t  state;
    //iteration , Bearer Context { Ebi/Fteid/BearerQos }  item should be included.
    gtpc_iterate_item(parsed_pkt,on_gtpc_parse_root, &state);

    EXPECT_EQ(state.flag&HAVE_FTEID_U, HAVE_FTEID_U);
    EXPECT_EQ(state.flag&HAVE_BQOS, HAVE_BQOS);
    EXPECT_EQ(state.flag&HAVE_EBI_B, HAVE_EBI_B);

    EXPECT_EQ(memcmp(&state.u_teid, &bctx_test.fteid, sizeof(bctx_test.fteid) - 12),0);
    EXPECT_EQ(memcmp(&state.ebi_b, &bctx_test.ebi, sizeof(bctx_test.ebi)),0);
    EXPECT_EQ(memcmp(&state.bqos, &bctx_test.bqos, sizeof(bctx_test.bqos)),0);
    //
    EXPECT_EQ(gtpc_free_packet(&pkt), OK);
    EXPECT_EQ(gtpc_free_packet(&parsed_pkt), OK);
}

TEST(GtpcParseTest, PaaMNOIpv64){
    gtp_packet_ptr  pkt = NULL,parsed_pkt = NULL;
    EXPECT_EQ(gtpc_alloc_packet(&pkt, GTPC_CREATE_SESSION_REQ), OK);
    EXPECT_EQ(pkt!=NULL,true);
    if (!pkt){ return; }
    //
    U8 paa_val[] = {
        0x01, 0x00, 0x00, 0x00,
        0x00, 0x02, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x03, 0x00, 0x00,
        0x00, 0x00, 0x04, 0x00,
        0x00,
    };
    gtpc_paa_t paa;
    //
    EXPECT_EQ(gtpc_paa_set(&paa, 3, paa_val, sizeof(paa_val)), OK);
    EXPECT_EQ(gtpc_append_item(pkt, GTPC_TYPE_PAA, (U8*)&paa, gtpc_paa_length(&paa)), OK);
    //
    gtpc_packet_print(pkt);

    // attach to packet
    auto gtpch = gtpc_header_(pkt);
    EXPECT_EQ(gtpc_alloc_packet_from_payload(&parsed_pkt,(U8*)gtpch, pkt->offset), OK);
    gtpc_parse_state_t  state;

    //iteration , paa  item should be included.
    gtpc_iterate_item(parsed_pkt,on_gtpc_parse_root, &state);
    EXPECT_EQ(state.flag&HAVE_PAA, HAVE_PAA);
    EXPECT_EQ(memcmp(state.paa.paa, &paa_val, sizeof(paa_val)), 0);
    //
    EXPECT_EQ(gtpc_free_packet(&pkt), OK);
    EXPECT_EQ(gtpc_free_packet(&parsed_pkt), OK);
}


TEST(GtpcParseTest, PaaErr){
    gtp_packet_ptr  pkt = NULL;
    EXPECT_EQ(gtpc_alloc_packet(&pkt, GTPC_CREATE_SESSION_REQ), OK);
    EXPECT_EQ(pkt!=NULL,true);
    if (!pkt){ return; }
    //
    U8 paa_val[] = {
        0x01, 0x00, 0x00, 0x00,
        0x00, 0x02, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x03, 0x00, 0x00,
        0x00, 0x00, 0x04, 0x00,
        0x00, 0x99,
    };
    gtpc_paa_t paa;
    //
    EXPECT_EQ(gtpc_paa_set(&paa, 3, paa_val, sizeof(paa_val)), ERR);
    EXPECT_EQ(gtpc_free_packet(&pkt), OK);
}



TEST(GtpcParseTest, MemoryCheck){
    EXPECT_EQ(gtpc_memory_print(), OK);
}