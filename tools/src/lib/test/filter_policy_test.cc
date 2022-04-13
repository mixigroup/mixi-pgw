//
// Created by mixi on 2017/07/05.
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
#include "lib/filter/policy.hpp"
#include "lib/const/policy.h"

#include "lib_lookup/lookup_table_tpl.hpp"



using namespace MIXIPGW_TOOLS;

static char        pkt[1024] = {0};
static int         pktflg(0);
static char        macaddr[ETHER_ADDR_LEN+1] = {0};
#define     INPUT_VLANID    (10)
#define     LINKED_VLANID   (11)
#define     LINKED_IPV4     (0x00057687)
typedef LookupTable<policy_t,1048576,uint32_t>  POLICYTBL;

TEST(FilterPolicy, BasicIpv4){
    bzero(pkt,sizeof(pkt));


    ((struct ether_header*)pkt)->ether_type = htons(ETHERTYPE_VLAN);
    *((uint16_t*)(((struct ether_header*)pkt)+1)) =  htons(INPUT_VLANID);
    *((uint16_t*)(((struct ether_header*)pkt)+2)) =  htons(ETHERTYPE_IP);

    PktHeader           pkth(pkt, (18+20+8+128));
    ProcessParameter    prm;
    //
    EH((&pkth))->ether_type = htons(ETHERTYPE_VLAN);
    IP((&pkth))->ip_hl = 5;
    IP((&pkth))->ip_v = 4;
    IP((&pkth))->ip_dst.s_addr = htonl(LINKED_IPV4);

    memset(EH((&pkth))->ether_dhost, (u_char)0xde, ETHER_ADDR_LEN);
    memset(EH((&pkth))->ether_shost, (u_char)0xfe, ETHER_ADDR_LEN);
    Logger::LOGINF("now ,vlanid(%u)", *((uint16_t*)(((struct ether_header*)pkt)+1)));

    //
    policy_t    lnk;
    lnk.linked_ipv4 = htonl(LINKED_IPV4);
    lnk.stat.linked_vlanid = LINKED_VLANID;
    // link of teid
    std::auto_ptr<POLICYTBL>   tbl(POLICYTBL::Create());
    prm.Set(ProcessParameter::TBL_POLICY, 0, tbl.get());

    Logger::LOGINF("completed. table setup.(%p)", tbl.get());
    // set found item to table
    EXPECT_EQ(tbl.get()->Add(LINKED_IPV4, &lnk, 0), 0);
    //
    Policy policy;
    FilterContainer container;
    container.SetFilter(FilterContainer::POLICY,  &policy,  &prm);
    //
    EXPECT_EQ(policy.OnFilter(&pkth,&pktflg), RETOK);
    memset(macaddr,0xfe,ETHER_ADDR_LEN);
    EXPECT_EQ(memcmp(EH((&pkth))->ether_dhost, macaddr, ETHER_ADDR_LEN),0);
    EXPECT_EQ(*((uint16_t*)(((struct ether_header*)pkt)+1)),htons(LINKED_VLANID));

    Logger::LOGINF("vlanid(%u)", *((uint16_t*)(((struct ether_header*)pkt)+1)));
}


TEST(FilterPolicy, BasicIpv6){
    bzero(pkt,sizeof(pkt));

    ((struct ether_header*)pkt)->ether_type = htons(ETHERTYPE_VLAN);
    *((uint16_t*)(((struct ether_header*)pkt)+1)) =  htons(INPUT_VLANID);
    *((uint16_t*)(((struct ether_header*)pkt)+2)) =  htons(ETHERTYPE_IP);


    uint8_t    ipv6_testaddr[16] = {0x00,0x00,0x00,0x00,
                                    0x0a,0x05,0x76,0x87,   // <- used here.
                                    0x00,0x00,0x00,0x00,
                                    0x00,0x00,0x00,0x00};
    ((struct ip6_hdr*)(pkt+14+4))->ip6_nxt = IPPROTO_UDP;
    ((struct ip6_hdr*)(pkt+14+4))->ip6_vfc = 6;
    memcpy(((struct ip6_hdr*)(pkt+14+4))->ip6_dst.s6_addr, ipv6_testaddr, 16);

    PktHeader           pkth(pkt, (14+4+40+8+128));
    ProcessParameter    prm;

    memset(EH((&pkth))->ether_dhost, (u_char)0xde, ETHER_ADDR_LEN);
    memset(EH((&pkth))->ether_shost, (u_char)0xfe, ETHER_ADDR_LEN);
    Logger::LOGINF("now ,vlanid(%u)", *((uint16_t*)(((struct ether_header*)pkt)+1)));

    //
    policy_t    lnk;
    lnk.linked_ipv4 = LINKED_IPV4;
    lnk.stat.linked_vlanid = LINKED_VLANID;
    // link of teid
    std::auto_ptr<POLICYTBL>   tbl(POLICYTBL::Create());
    prm.Set(ProcessParameter::TBL_POLICY, 0, tbl.get());

    Logger::LOGINF("completed. table setup.(%p)", tbl.get());
    // set found item to table.
    EXPECT_EQ(tbl.get()->Add(LINKED_IPV4, &lnk, 0), 0);
    //
    Policy policy;
    FilterContainer container;
    container.SetFilter(FilterContainer::POLICY,  &policy,  &prm);
    //
    EXPECT_EQ(policy.OnFilter(&pkth,&pktflg), RETOK);
    memset(macaddr,0xfe,ETHER_ADDR_LEN);
    EXPECT_EQ(memcmp(EH((&pkth))->ether_dhost, macaddr, ETHER_ADDR_LEN),0);
    EXPECT_EQ(*((uint16_t*)(((struct ether_header*)pkt)+1)),htons(LINKED_VLANID));

    Logger::LOGINF("vlanid(%u)", *((uint16_t*)(((struct ether_header*)pkt)+1)));
}

