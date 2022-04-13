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

//
TEST(Translator, VlanExtract){
    bzero(pkt,sizeof(pkt));
    PktHeader           pkth(pkt, 512);
    FilterContainer     container;
    ProcessParameter    prm;
    // link of teid
    std::auto_ptr<DualBufferedLookupTable<link_t>>   tbl(DualBufferedLookupTable<link_t>::Create());
    prm.Set(ProcessParameter::TBL_TEID, 9, tbl.get());

    // policer table
    std::auto_ptr<DualBufferedLookupTable<policer_t>>   tblc(DualBufferedLookupTable<policer_t>::Create());
    prm.Set(ProcessParameter::TBL_POLICER, 9, tblc.get());


    prm.Set(ProcessParameter::USG_NEXT_HOP_VLANID, 123);


    //
    memset(EH((&pkth))->ether_dhost, (u_char)0x98, ETHER_ADDR_LEN);
    memset(EH((&pkth))->ether_shost, (u_char)0x76, ETHER_ADDR_LEN);
    EH((&pkth))->ether_type = htons(ETHERTYPE_IP);

    //
    IP((&pkth))->ip_hl = 5;
    IP_I((&pkth))->ip_hl = 5;
    IP_I((&pkth))->ip_v = IPVERSION;
    link_t cnv;
    bzero(&cnv, sizeof(cnv));
    cnv.w.pgw_teid.gid = 9;
    cnv.w.pgw_teid.gcnt = 4321;
    //
    GTPU((&pkth))->tid = htonl(cnv.w.pgw_teid_uc);
    GTPU((&pkth))->u.v1_flags.version = GTPU_VERSION_1;
    GTPU((&pkth))->u.v1_flags.proto = GTPU_PROTO_GTP;
    GTPU((&pkth))->u.v1_flags.npdu = GTPU_NPDU_OFF;
    GTPU((&pkth))->type = GTPU_G_PDU;

    // set found item to table
    tbl.get()->Add(cnv.w.pgw_teid_uc, &cnv, 0);

    policer_t   policer;
    bzero(&policer, sizeof(policer));
    policer.commit_rate = 1024;
    policer.commit_burst_size = 1024;
    policer.commit_information_rate = 1024;
    policer.excess_rate = 1024;
    policer.excess_burst_size = 1024;
    policer.excess_information_rate = 1024;
    //
    tblc.get()->Add(cnv.w.pgw_teid_uc, &policer, 0);
    //
    TranslatorDecapMarker td;
    container.SetFilter(FilterContainer::GTPU, &td,  &prm);

    EXPECT_EQ(td.OnFilter(&pkth,&pktflg), RETOK);
    memset(macaddr,0x76,ETHER_ADDR_LEN);
    EXPECT_EQ(memcmp(EH((&pkth))->ether_dhost, macaddr, ETHER_ADDR_LEN),0);
    EXPECT_EQ(EH((&pkth))->ether_type,  htons(ETHERTYPE_VLAN));
    EXPECT_EQ(*((u_short*)(EH((&pkth))+1)+0),  (u_short)htons(123));
    EXPECT_EQ(*((u_short*)(EH((&pkth))+1)+1),  (u_short)htons(ETHERTYPE_IP));
}
