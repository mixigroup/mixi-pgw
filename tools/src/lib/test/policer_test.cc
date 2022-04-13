//
// Created by mixi on 2017/05/16.
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
#include "lib/filter/gtpu_decap.hpp"

#include "lib/const/gtpu.h"
#include "lib/const/link.h"
#include "lib/const/counter.h"
#include "lib/const/policer.h"
#include "lib/const/process_ctrl.h"

#include "lib_lookup/dual_buffered_lookup_table_tpl.hpp"


using namespace MIXIPGW_TOOLS;

static char        pkt[1024] = {0};
static int         pktflg(0);
static char        macaddr[ETHER_ADDR_LEN+1] = {0};

static void set_pckth(PktHeader& pkth, link_t& cnv){
    memset(EH((&pkth))->ether_dhost, (u_char)0x22, ETHER_ADDR_LEN);
    memset(EH((&pkth))->ether_shost, (u_char)0x33, ETHER_ADDR_LEN);
    EH((&pkth))->ether_type = htons(ETHERTYPE_IP);
    //
    IP((&pkth))->ip_hl = 5;
    IP_I((&pkth))->ip_hl = 5;
    IP_I((&pkth))->ip_v = IPVERSION;
    //
    GTPU((&pkth))->tid = htonl(cnv.w.pgw_teid_uc);
    GTPU((&pkth))->u.v1_flags.version = GTPU_VERSION_1;
    GTPU((&pkth))->u.v1_flags.proto = GTPU_PROTO_GTP;
    GTPU((&pkth))->u.v1_flags.npdu = GTPU_NPDU_OFF;
    GTPU((&pkth))->type = GTPU_G_PDU;
}

//
TEST(Policer, InitParameters){
    FilterContainer     container;
    ProcessParameter    prm;
    // link of teid
    std::auto_ptr<DualBufferedLookupTable<link_t>>   tbl(DualBufferedLookupTable<link_t>::Create());
    prm.Set(ProcessParameter::TBL_TEID, 1, tbl.get());

    // policer table
    std::auto_ptr<DualBufferedLookupTable<policer_t>>   tblc(DualBufferedLookupTable<policer_t>::Create());
    prm.Set(ProcessParameter::TBL_POLICER, 1, tblc.get());

    link_t cnv;
    bzero(&cnv, sizeof(cnv));
    cnv.w.pgw_teid.gid = 1;
    cnv.w.pgw_teid.gcnt = 1155;

#define POLICER_VALUE   (1024)

    policer_t   policer;
    bzero(&policer, sizeof(policer));
    policer.commit_rate             = POLICER_VALUE;
    policer.commit_burst_size       = POLICER_VALUE;
    policer.commit_information_rate = POLICER_VALUE;
    policer.excess_rate             = POLICER_VALUE;
    policer.excess_burst_size       = POLICER_VALUE;
    policer.excess_information_rate = POLICER_VALUE;
    //
    tblc.get()->Add(cnv.w.pgw_teid_uc, &policer, 0);

    TranslatorDecapEnforcer td;
    container.SetFilter(FilterContainer::GTPU, &td,  &prm);
    bzero(pkt,sizeof(pkt));
    pktflg = 0;
    {
        PktHeader pkth(pkt, 512);
        set_pckth(pkth, cnv);

        // set found item to table
        tbl.get()->Add(cnv.w.pgw_teid_uc, &cnv, 0);
        //
        pktflg = PROC_TRIGGER_1S_EVENT;
        // 1 packet
        EXPECT_EQ(td.OnFilter(&pkth,&pktflg), RETOK);
        EXPECT_EQ(pktflg&PROC_TRIGGER_1S_EVENT?true:false, false);
        EXPECT_EQ(pktflg&PROC_NEED_SEND?true:false,true);
    }
    for(auto n = 0;n < 3;n++){
        bzero(pkt,sizeof(pkt));
        pktflg = 0;
        PktHeader pkth(pkt, 512);
        set_pckth(pkth, cnv);
        //
        EXPECT_EQ(td.OnFilter(&pkth,&pktflg), RETOK);
        EXPECT_EQ(pktflg&PROC_RFC4115_RED?true:false, false);
        EXPECT_EQ(pktflg&PROC_NEED_SEND?true:false,true);
    }
    bzero(pkt,sizeof(pkt));
    pktflg = 0;
    {
        PktHeader pkth(pkt, 512);
        set_pckth(pkth, cnv);
        EXPECT_EQ(td.OnFilter(&pkth,&pktflg), RETOK);
    }
    EXPECT_EQ(pktflg&PROC_RFC4115_RED?true:false, true);
    EXPECT_EQ(pktflg&PROC_NEED_SEND?true:false,false);

    // to be dropped
    bzero(pkt,sizeof(pkt));
    pktflg = 0;
    {
        PktHeader pkth(pkt, 512);
        set_pckth(pkth, cnv);
        EXPECT_EQ(td.OnFilter(&pkth,&pktflg), RETOK);
    }
    EXPECT_EQ(pktflg&PROC_RFC4115_RED?true:false, true);
    EXPECT_EQ(pktflg&PROC_NEED_SEND?true:false,false);


    // displayed 1 sec elapsed
    // Token replenished in COMMIT/EXCESS
    //  so transmission will be available.
    bzero(pkt,sizeof(pkt));
    pktflg = 0;
    {
        PktHeader pkth(pkt, 512);
        set_pckth(pkth, cnv);

        // set found item to table.
        tbl.get()->Add(cnv.w.pgw_teid_uc, &cnv, 0);
        pktflg = PROC_TRIGGER_1S_EVENT;
        EXPECT_EQ(td.OnFilter(&pkth,&pktflg), RETOK);
        EXPECT_EQ(pktflg&PROC_TRIGGER_1S_EVENT?true:false, false);
        EXPECT_EQ(pktflg&PROC_NEED_SEND?true:false,true);
    }
}