/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       core_pgw_egress_worker.cc
    @brief      PGW - egress worker core implemented
*******************************************************************************
	PGW : egress : \n
	\n
    \n
    \n
*******************************************************************************
    @date       created(31/oct/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 31/oct/2017 
      -# Initial Version
******************************************************************************/
#include "mixi_pgw_data_plane_def.hpp"
#include "../../inc/core.hpp"

using namespace MIXIPGW;

/**
   PGW - Egress worker core interface  :  constructor \n
   *******************************************************************************
   constructor \n
   *******************************************************************************
   @param[in]     lcore         core id
   @param[in]     dburi         databaseuri
   @parma[in]     serverid      database server id
   @param[in]     cpuid         thread for host stack(binlog) cpu/core
   @param[in]     greipv4       GRE terminated ip address
   @param[in]     gresrcipv4    GRE terminated ip source  address
   @param[in]     groupid        group id
   @param[in]     pgwid         PGW id
 */
MIXIPGW::CorePgwEgressWorker::CorePgwEgressWorker(COREID lcore, TXT dburi, unsigned serverid, unsigned cpuid, unsigned greipv4, unsigned gresrcipv4,unsigned groupid, unsigned pgwid)
        :CorePgwBaseWorker(lcore, dburi, serverid, cpuid, greipv4, gresrcipv4, groupid, pgwid,1,TYPE::PGW_EGRESS){
    //
    PGW_LOG(RTE_LOG_INFO, "lcore: %u/dburi: %s/serverid: %u/cpuid: %u/greipv4: %08x/gresrcipv4: %08x/groupid: %u/pgwid: %u\n", lcore, dburi, serverid, cpuid, greipv4, gresrcipv4, groupid, pgwid);
}

/**
   PGW - Egress worker core interface  : destructor\n
   *******************************************************************************
   destructor\n
   *******************************************************************************

 */
MIXIPGW::CorePgwEgressWorker::~CorePgwEgressWorker(){}
/**
   get core type.\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return TYPE  return core type[worker]
 */
TYPE MIXIPGW::CorePgwEgressWorker::GetType(void){
    return(TYPE::PGW_EGRESS);
}
/**
   object name\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return TXT  object name 
 */
TXT MIXIPGW::CorePgwEgressWorker::GetObjectName(void){
    return("CorePgwEgressWorker");
}

/**
   convert GTPU capsul to GRE capsul.\n
   *******************************************************************************
   CoreEncapWorker processe encapsulates GTPU packet with ipv4.\n
   *******************************************************************************
   @param[in]     inpkt change encapsulate target packet 
 */
RETCD MIXIPGW::CorePgwEgressWorker::ModifyPacket(struct rte_mbuf* inpkt){
    auto inip_ofst  = (sizeof(ether_hdr) + sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr) + sizeof(struct gtpu_hdr));
    auto ip_ot      = rte_pktmbuf_mtod_offset(inpkt, struct ipv4_hdr*, sizeof(ether_hdr));
    auto gtph       = rte_pktmbuf_mtod_offset(inpkt, struct gtpu_hdr*, sizeof(ether_hdr) + sizeof(ipv4_hdr) + sizeof(udp_hdr));

    uint32_t ipkey  = 0;
    // extract keys from inner ipv4/v6
    if (RTE_ETH_IS_IPV4_HDR(inpkt->packet_type)) {
        auto ip4 = rte_pktmbuf_mtod_offset(inpkt, struct ipv4_hdr *, inip_ofst);
        ipkey = ip4->dst_addr;
    }else if (RTE_ETH_IS_IPV6_HDR(inpkt->packet_type)) {
        auto ip6 = rte_pktmbuf_mtod_offset(inpkt, struct ipv6_hdr *, inip_ofst);
        rte_memcpy(&ipkey, &ip6->dst_addr[12], 4);
    }else{
        rte_pktmbuf_free(inpkt);
        return(1);
    }
    // find from lookup table
    lookup_t  itm;
    if (Find(ipkey, &itm) != 0){
        rte_pktmbuf_free(inpkt);
        return(1);
    }

    // sgw:gtpu - set teid
    gtph->tid = itm.sgw_gtpu_teid;
    // sgw destination ip address   - gre source ipv4 = use as pgw ipv4
    ip_ot->src_addr     = gresrcipv4_;
    ip_ot->dst_addr     = itm.sgw_gtpu_ipv4;
    ip_ot->hdr_checksum = 0;
    //
    return(0);
}

