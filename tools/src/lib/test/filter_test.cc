//
// Created by mixi on 2017/05/14.
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
static char        macaddr[ETHER_ADDR_LEN+1] = {0};

// must be swapped sending and receiving MAC addresses.
// generic packets list(apr,bfd,icmp)
TEST(BasicFilters, MacAddressReverse){
    bzero(pkt,sizeof(pkt));
    PktHeader           pkth(pkt, (14+20+8+128));
                        // ether + ip + udp + ctrl(user payload)
    ProcessParameter    prm;

    IP((&pkth))->ip_hl = 5;
    EH((&pkth))->ether_type = htons(ETHERTYPE_IP);

    Logger::LOGINF("ip_hl(%u)", IP((&pkth))->ip_hl);

    memset(EH((&pkth))->ether_dhost, (u_char)0xde, ETHER_ADDR_LEN);
    memset(EH((&pkth))->ether_shost, (u_char)0xfe, ETHER_ADDR_LEN);
    memset(ARP((&pkth))->arp_sha,    (u_char)0xfe, ETHER_ADDR_LEN);
    Logger::LOGINF("ip_hl(%u)", IP((&pkth))->ip_hl);
    // basic filter's
    {   Arp arp;
        FilterContainer container;
        container.SetFilter(FilterContainer::ARP,  &arp,  &prm);

        memset(macaddr,0xcd,ETHER_ADDR_LEN);
        prm.Set(ProcessParameter::TXT_MAC_DST, macaddr);

        EXPECT_EQ(arp.OnFilter(&pkth,&pktflg), RETOK);
        memset(macaddr,0xfe,ETHER_ADDR_LEN);
        EXPECT_EQ(memcmp(EH((&pkth))->ether_dhost, macaddr, ETHER_ADDR_LEN),0);
    }
    IP((&pkth))->ip_hl = 5;
    Logger::LOGINF("ip_hl(%u)", IP((&pkth))->ip_hl);
    //
    {   Bfd bfd;
        FilterContainer container;
        container.SetFilter(FilterContainer::BFD,  &bfd,  &prm);
        container.SetFilter(FilterContainer::BFDC, &bfd,  &prm);
        EXPECT_EQ(bfd.OnFilter(&pkth,&pktflg), RETOK);
        memset(macaddr,0xcd,ETHER_ADDR_LEN);
        EXPECT_EQ(memcmp(EH((&pkth))->ether_dhost, macaddr, ETHER_ADDR_LEN),0);
    }
    Logger::LOGINF("ip_hl(%u)", IP((&pkth))->ip_hl);
    //
    {   Icmp icmp;
        FilterContainer container;
        container.SetFilter(FilterContainer::ICMP, &icmp,  &prm);
        EXPECT_EQ(icmp.OnFilter(&pkth,&pktflg), RETOK);
        memset(macaddr,0xfe,ETHER_ADDR_LEN);
        EXPECT_EQ(memcmp(EH((&pkth))->ether_dhost, macaddr, ETHER_ADDR_LEN),0);
    }
    IP((&pkth))->ip_hl = 5;
    Logger::LOGINF("ip_hl(%u)", IP((&pkth))->ip_hl);
    //
    {   GtpuEcho    gech;
        FilterContainer container;
        container.SetFilter(FilterContainer::GTPUECHOREQ, &gech,  &prm);
        EXPECT_EQ(gech.OnFilter(&pkth,&pktflg), RETOK);
        memset(macaddr,0xcd,ETHER_ADDR_LEN);
        EXPECT_EQ(memcmp(EH((&pkth))->ether_dhost, macaddr, ETHER_ADDR_LEN),0);
    }
}
//
TEST(GtpuCounterEgressFilter, MacAddressReverse){

    bzero(pkt,sizeof(pkt));
    PktHeader           pkth(pkt, 512);
    FilterContainer     container;
    ProcessParameter    prm;
    // link of teid
    std::auto_ptr<DualBufferedLookupTable<link_t>>   tbl(DualBufferedLookupTable<link_t>::Create());
    prm.Set(ProcessParameter::TBL_TEID, 7, tbl.get());

    // counter of teid table
    std::auto_ptr<DualBufferedLookupTable<teid_counter_t>>   tblc(DualBufferedLookupTable<teid_counter_t>::Create());
    prm.Set(ProcessParameter::TBL_COUNTER_E, 7, tblc.get());

    //
    IP((&pkth))->ip_hl = 5;
    link_t cnv;
    bzero(&cnv, sizeof(cnv));
    cnv.w.pgw_teid.gid = 7;
    cnv.w.pgw_teid.gcnt = 1234;

    GTPU((&pkth))->tid = htonl(cnv.w.pgw_teid_uc);

    // set foud item to table
    tbl.get()->Add(cnv.w.pgw_teid_uc, &cnv, 0);
    //
    memset(EH((&pkth))->ether_dhost, (u_char)0xde, ETHER_ADDR_LEN);
    memset(EH((&pkth))->ether_shost, (u_char)0xfe, ETHER_ADDR_LEN);
    //
    CounterEgress ce;
    container.SetFilter(FilterContainer::GTPU, &ce,  &prm);

    EXPECT_EQ(ce.OnFilter(&pkth,&pktflg), RETOK);
    memset(macaddr,0xfe,ETHER_ADDR_LEN);
    EXPECT_EQ(memcmp(EH((&pkth))->ether_dhost, macaddr, ETHER_ADDR_LEN),0);
}
//
TEST(GtpuCounterIngressFilter, MacAddressReverse){

    bzero(pkt,sizeof(pkt));
    PktHeader           pkth(pkt, 512);
    FilterContainer     container;
    ProcessParameter    prm;
    // link of teid
    std::auto_ptr<DualBufferedLookupTable<link_t>>   tbl(DualBufferedLookupTable<link_t>::Create());
    prm.Set(ProcessParameter::TBL_TEID, 6, tbl.get());

    // table of teid counter
    std::auto_ptr<DualBufferedLookupTable<teid_counter_t>>   tblc(DualBufferedLookupTable<teid_counter_t>::Create());
    prm.Set(ProcessParameter::TBL_COUNTER_I, 6, tblc.get());

    //
    IP((&pkth))->ip_hl = 5;
    link_t cnv;
    bzero(&cnv, sizeof(cnv));
    cnv.w.pgw_teid.gid = 6;
    cnv.w.pgw_teid.gcnt = 2345;

    GTPU((&pkth))->tid = htonl(cnv.w.pgw_teid_uc);
    GTPU((&pkth))->u.v1_flags.version = GTPU_VERSION_1;
    GTPU((&pkth))->u.v1_flags.proto = GTPU_PROTO_GTP;
    GTPU((&pkth))->u.v1_flags.npdu = GTPU_NPDU_OFF;
    GTPU((&pkth))->type = GTPU_G_PDU;

    // set found item to table.
    tbl.get()->Add(cnv.w.pgw_teid_uc, &cnv, 0);
    //
    memset(EH((&pkth))->ether_dhost, (u_char)0xda, ETHER_ADDR_LEN);
    memset(EH((&pkth))->ether_shost, (u_char)0xfa, ETHER_ADDR_LEN);
    //
    CounterIngress ci;
    container.SetFilter(FilterContainer::GTPU, &ci,  &prm);

    EXPECT_EQ(ci.OnFilter(&pkth,&pktflg), RETOK);
    memset(macaddr,0xfa,ETHER_ADDR_LEN);
    EXPECT_EQ(memcmp(EH((&pkth))->ether_dhost, macaddr, ETHER_ADDR_LEN),0);
}
//
TEST(GtpuDecapFilter, MacAddressReverse){
    bzero(pkt,sizeof(pkt));
    PktHeader           pkth(pkt, 512);
    FilterContainer     container;
    ProcessParameter    prm;
    // link of teid
    std::auto_ptr<DualBufferedLookupTable<link_t>>   tbl(DualBufferedLookupTable<link_t>::Create());
    prm.Set(ProcessParameter::TBL_TEID, 3, tbl.get());

    // policer table
    std::auto_ptr<DualBufferedLookupTable<policer_t>>   tblc(DualBufferedLookupTable<policer_t>::Create());
    prm.Set(ProcessParameter::TBL_POLICER, 3, tblc.get());

    //
    IP((&pkth))->ip_hl = 5;
    IP_I((&pkth))->ip_v = IPVERSION;
    link_t cnv;
    bzero(&cnv, sizeof(cnv));
    cnv.w.pgw_teid.gid = 3;
    cnv.w.pgw_teid.gcnt = 3456;

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
    memset(EH((&pkth))->ether_dhost, (u_char)0xdc, ETHER_ADDR_LEN);
    memset(EH((&pkth))->ether_shost, (u_char)0xfc, ETHER_ADDR_LEN);
    //
    TranslatorDecapMarker td;
    container.SetFilter(FilterContainer::GTPU, &td,  &prm);

    EXPECT_EQ(td.OnFilter(&pkth,&pktflg), RETOK);
    memset(macaddr,0xfc,ETHER_ADDR_LEN);
    EXPECT_EQ(memcmp(EH((&pkth))->ether_dhost, macaddr, ETHER_ADDR_LEN),0);
}
//
TEST(GtpuEncapFilter, MacAddressReverse){
    bzero(pkt,sizeof(pkt));
    PktHeader           pkth(pkt, 512);
    FilterContainer     container;
    ProcessParameter    prm;
    // link of teid
    std::auto_ptr<DualBufferedLookupTable<link_t>>   tbl(DualBufferedLookupTable<link_t>::Create());
    prm.Set(ProcessParameter::TBL_TEID, 4, tbl.get());

    // policer table
    std::auto_ptr<DualBufferedLookupTable<policer_t>>   tblc(DualBufferedLookupTable<policer_t>::Create());
    prm.Set(ProcessParameter::TBL_POLICER, 4, tblc.get());

    //
    IP((&pkth))->ip_hl = 5;
    IP_I((&pkth))->ip_v = IPVERSION;
    link_t cnv;
    bzero(&cnv, sizeof(cnv));
    cnv.w.pgw_teid.gid = 4;
    cnv.w.pgw_teid.gcnt = 4567;

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
    memset(EH((&pkth))->ether_dhost, (u_char)0xdd, ETHER_ADDR_LEN);
    memset(EH((&pkth))->ether_shost, (u_char)0xfd, ETHER_ADDR_LEN);
    //
    TranslatorDecapMarker td;
    container.SetFilter(FilterContainer::GTPU, &td,  &prm);

    EXPECT_EQ(td.OnFilter(&pkth,&pktflg), RETOK);
    memset(macaddr,0xfd,ETHER_ADDR_LEN);
    EXPECT_EQ(memcmp(EH((&pkth))->ether_dhost, macaddr, ETHER_ADDR_LEN),0);
}

//
TEST(EpcCtrlFilter, MacAddressReverse){
    bzero(pkt,sizeof(pkt));
    PktHeader           pkth(pkt, (14+20+8+128));
    FilterContainer     container;
    ProcessParameter    prm;
    // link of teid
    std::auto_ptr<DualBufferedLookupTable<link_t>>   tbl(DualBufferedLookupTable<link_t>::Create());
    prm.Set(ProcessParameter::TBL_TEID, 5, tbl.get());

    // policer table
    std::auto_ptr<DualBufferedLookupTable<policer_t>>   tblc(DualBufferedLookupTable<policer_t>::Create());
    prm.Set(ProcessParameter::TBL_POLICER, 5, tblc.get());

    //
    IP((&pkth))->ip_hl = 5;
    IP_I((&pkth))->ip_v = IPVERSION;
    link_t cnv;
    bzero(&cnv, sizeof(cnv));
    cnv.w.pgw_teid.gid = 5;
    cnv.w.pgw_teid.gcnt = 5678;

    GTPU((&pkth))->tid = htonl(cnv.w.pgw_teid_uc);
    GTPU((&pkth))->u.v1_flags.version = GTPU_VERSION_1;
    GTPU((&pkth))->u.v1_flags.proto = GTPU_PROTO_GTP;
    GTPU((&pkth))->u.v1_flags.npdu = GTPU_NPDU_OFF;
    GTPU((&pkth))->type = GTPU_G_PDU;

    ((process_ctrl_ptr)UDPP((&pkth)))->teid = htonl(cnv.w.pgw_teid_uc);

    // set found item to table.
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
    memset(EH((&pkth))->ether_dhost, (u_char)0xea, ETHER_ADDR_LEN);
    memset(EH((&pkth))->ether_shost, (u_char)0x11, ETHER_ADDR_LEN);
    //
    EpcCtrl xe;
    container.SetFilter(FilterContainer::CTRL, &xe,  &prm);

    EXPECT_EQ(xe.OnFilter(&pkth,&pktflg), RETOK);
    memset(macaddr,0x11,ETHER_ADDR_LEN);
    EXPECT_EQ(memcmp(EH((&pkth))->ether_dhost, macaddr, ETHER_ADDR_LEN),0);
}

