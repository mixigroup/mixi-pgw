//
// Created by mixi on 2017/05/15.
//

#include "gtest/gtest.h"

#include "mixipgw_tools_def.h"

#include <stdexcept>
#include <memory>
#include <functional>
#include <vector>
#include <string>

#include "lib/module.hpp"
#include "lib/pktheader.hpp"
#include "lib/process.hpp"
#include "lib/logger.hpp"
#include "lib/filter/arp.hpp"
#include "lib/filter/bfd.hpp"
#include "lib/filter/icmp.hpp"
#include "lib/filter/gtpu_counter_egress.hpp"
#include "lib/filter/gtpu_counter_ingress.hpp"
#include "lib/filter/gtpu_decap.hpp"
#include "lib/filter/gtpu_encap.hpp"
#include "lib/filter/gtpu_echo.hpp"
#include "lib/filter/xepc_ctrl.hpp"

#include "lib/const/gtpu.h"
#include "lib/const/link.h"
#include "lib/const/counter.h"
#include "lib/const/policer.h"
#include "lib/const/process_ctrl.h"

#include "lib_lookup/dual_buffered_lookup_table_tpl.hpp"


using namespace MIXIPGW_TOOLS;

static char        pkt[1024] = {0};
static int         pktflg(0);

static void CheckNoticeBits(__be16 id, DualBufferedLookupTable<teid_counter_t>* tblc, std::function<void(int,DualBufferedLookupTable<teid_counter_t>*)> fnc){
    uint64_t bmp64 = tblc->FindNoticeBmp64((id>>6)<<6);
    for(int m = 0;m < 64;m++){
        if (bmp64&(((uint64_t)1)<<m)){
            fnc(m, tblc);
            return;
        }
    }
}

//
TEST(GtpuCounterIngressFilter, CounterIncr){

    bzero(pkt,sizeof(pkt));
    PktHeader           pkth(pkt, 512);
    FilterContainer     container;
    ProcessParameter    prm;
    // link of teid
    std::auto_ptr<DualBufferedLookupTable<link_t>>   tbl(DualBufferedLookupTable<link_t>::Create());
    prm.Set(ProcessParameter::TBL_TEID, 8, tbl.get());

    // teid counter table
    std::auto_ptr<DualBufferedLookupTable<teid_counter_t>>   tblc(DualBufferedLookupTable<teid_counter_t>::Create());
    prm.Set(ProcessParameter::TBL_COUNTER_I, 8, tblc.get());

    //
    IP((&pkth))->ip_hl = 5;
    link_t cnv;
    bzero(&cnv, sizeof(cnv));
    cnv.w.pgw_teid.gid = 8;
    cnv.w.pgw_teid.gcnt = 8989;

    GTPU((&pkth))->tid = htonl(cnv.w.pgw_teid_uc);
    GTPU((&pkth))->u.v1_flags.version = GTPU_VERSION_1;
    GTPU((&pkth))->u.v1_flags.proto = GTPU_PROTO_GTP;
    GTPU((&pkth))->u.v1_flags.npdu = GTPU_NPDU_OFF;
    GTPU((&pkth))->type = GTPU_G_PDU;

    // setup table to found item
    tbl.get()->Add(cnv.w.pgw_teid_uc, &cnv, 0);
    EXPECT_EQ(cnv.w.pgw_teid.gcnt, 8989);
    EXPECT_EQ(cnv.w.pgw_teid.gid, 8);
    Logger::LOGINF("cnv: %u / %u", cnv.w.pgw_teid.gcnt, cnv.w.pgw_teid.gid);
    //
    CounterIngress ci;
    container.SetFilter(FilterContainer::GTPU, &ci,  &prm);

    // 1 packets -> 512 bytes
    bool isok = true;
    EXPECT_EQ(ci.OnFilter(&pkth,&pktflg), RETOK);
    CheckNoticeBits(8989, tblc.get(), [&isok](int m, DualBufferedLookupTable<teid_counter_t>*){ isok = false; });
    EXPECT_EQ(isok, true);
    // before 1MB
    for(auto l = 0;l < 2046;l++){
        EXPECT_EQ(ci.OnFilter(&pkth,&pktflg), RETOK);
        CheckNoticeBits(8989, tblc.get(), [&isok](int m,DualBufferedLookupTable<teid_counter_t>*){ isok = false; });
    }
    EXPECT_EQ(isok, true);
    Logger::LOGINF("cnv:.. %u / %u", cnv.w.pgw_teid.gcnt, cnv.w.pgw_teid.gid);
    //
    isok = false;
    // boundary of 1MB
    EXPECT_EQ(ci.OnFilter(&pkth,&pktflg), RETOK);
    CheckNoticeBits(8989, tblc.get(), [&isok,&cnv](int m,DualBufferedLookupTable<teid_counter_t>* tbl){
        isok = true;
        // teid with notify
        union _teidcnv tcnv;
        tcnv.pgw_teid.gcnt = (__be32)(((8989>>6)<<6)+m);
        tcnv.pgw_teid.gid  = 8;

        Logger::LOGINF("cnv...: %u / %u", cnv.w.pgw_teid.gcnt, cnv.w.pgw_teid.gid);
        Logger::LOGINF("tcnv...:%u / %u", tcnv.pgw_teid.gcnt, tcnv.pgw_teid.gid);
        //
        EXPECT_EQ(cnv.w.pgw_teid.gcnt , tcnv.pgw_teid.gcnt);
        EXPECT_EQ(cnv.w.pgw_teid.gid, tcnv.pgw_teid.gid);
        tbl->ClearNoticeBmp64(tcnv.pgw_teid.gcnt);
        isok = true;
    });
    EXPECT_EQ(isok, true);
    Logger::LOGINF("cnv....: %u / %u", cnv.w.pgw_teid.gcnt, cnv.w.pgw_teid.gid);

    // boundary of 2MB
    for(auto l = 0;l < 2047;l++){
        EXPECT_EQ(ci.OnFilter(&pkth,&pktflg), RETOK);
        CheckNoticeBits(8989, tblc.get(), [&isok](int m,DualBufferedLookupTable<teid_counter_t>*){
            Logger::LOGERR("2MB..(%d)", m);
            isok = false;
        });
    }
    EXPECT_EQ(isok, true);
    Logger::LOGINF("cnv:.. %u / %u", cnv.w.pgw_teid.gcnt, cnv.w.pgw_teid.gid);

    //
    isok = false;
    // boundary of 2MB
    EXPECT_EQ(ci.OnFilter(&pkth,&pktflg), RETOK);
    CheckNoticeBits(8989, tblc.get(), [&isok,&cnv](int m,DualBufferedLookupTable<teid_counter_t>*){
        // notify with teid
        union _teidcnv tcnv;
        tcnv.pgw_teid.gcnt = (__be32)(((8989>>6)<<6)+m);
        tcnv.pgw_teid.gid  = 8;

        Logger::LOGINF("2MB.cnv...: %u / %u", cnv.w.pgw_teid.gcnt, cnv.w.pgw_teid.gid);
        Logger::LOGINF("2MB.tcnv...:%u / %u", tcnv.pgw_teid.gcnt, tcnv.pgw_teid.gid);

        //
        EXPECT_EQ(cnv.w.pgw_teid.gcnt , tcnv.pgw_teid.gcnt);
        EXPECT_EQ(cnv.w.pgw_teid.gid, tcnv.pgw_teid.gid);
        isok = true;
    });
    EXPECT_EQ(isok, true);
    Logger::LOGINF("2MB.cnv:.. %u / %u", cnv.w.pgw_teid.gcnt, cnv.w.pgw_teid.gid);
}
