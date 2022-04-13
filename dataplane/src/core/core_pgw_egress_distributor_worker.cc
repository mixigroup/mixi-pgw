/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       core_pgw_egress_distributor_worker.cc
    @brief      PGW - distributor egress worker core implemented
*******************************************************************************
	PGW : Distributor egress : \n
    \n
*******************************************************************************
    @date       created(16/nov/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 16/nov/2017 
      -# Initial Version
******************************************************************************/
#include "mixi_pgw_data_plane_def.hpp"
#include "../../inc/core.hpp"

using namespace MIXIPGW;

/**
   PGW - Distributor Egress worker core interface  :  constructor \n
   *******************************************************************************
   constructor \n
   *******************************************************************************
   @param[in]     lcore         core id
   @param[in]     dburi         databaseuri
   @parma[in]     serverid      database server id
   @param[in]     cpuid         thread for host stack(binlog) cpu/core
   @param[in]     greipv4       GRE terminated ip address
   @param[in]     gresrcipv4    GRE terminated ip source  address
   @param[in]     groupid       group id
   @param[in]     pgwid         PGW id
 */
MIXIPGW::CorePgwEgressDistributorWorker::CorePgwEgressDistributorWorker(COREID lcore, TXT dburi, unsigned serverid, unsigned cpuid, unsigned greipv4, unsigned gresrcipv4,unsigned groupid, unsigned pgwid)
        :CorePgwBaseWorker(lcore, dburi, serverid, cpuid, greipv4, gresrcipv4, groupid, pgwid,1, TYPE::PGW_EGRESS){
    //
    PGW_LOG(RTE_LOG_INFO, "lcore: %u/dburi: %s/serverid: %u/cpuid: %u/greipv4: %08x/gresrcipv4: %08x/groupid: %u/pgwid: %u\n", lcore, dburi, serverid, cpuid, greipv4, gresrcipv4, groupid, pgwid);
}

/**
   PGW - Distributor Egress worker core interface  : destructor\n
   *******************************************************************************
   destructor\n
   *******************************************************************************

 */
MIXIPGW::CorePgwEgressDistributorWorker::~CorePgwEgressDistributorWorker(){}
/**
   get core type.\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return TYPE  return core type[worker]
 */
TYPE MIXIPGW::CorePgwEgressDistributorWorker::GetType(void){
    return(TYPE::PGW_EGRESS);
}
/**
   object name\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return TXT  object name 
 */
TXT MIXIPGW::CorePgwEgressDistributorWorker::GetObjectName(void){
    return("CorePgwEgressDistributorWorker");
}

/**
   convert GTPU capsul to GRE capsul.\n
   *******************************************************************************
   CoreEncapWorker process encapsulates GTPU packet with ipv4.\n
   *******************************************************************************
   @param[in]     inpkt change encapsulate target packet 
 */
RETCD MIXIPGW::CorePgwEgressDistributorWorker::ModifyPacket(struct rte_mbuf* inpkt){
    auto inip_ofst  = (sizeof(ether_hdr) + sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr) + sizeof(struct gtpu_hdr));
    auto eh    = rte_pktmbuf_mtod_offset(inpkt, struct ether_hdr*, 0);
    auto ip_ot = rte_pktmbuf_mtod_offset(inpkt, struct ipv4_hdr*, sizeof(ether_hdr));
    auto udp   = rte_pktmbuf_mtod_offset(inpkt, struct udp_hdr*, sizeof(ether_hdr) + sizeof(ipv4_hdr));
    auto gtph  = rte_pktmbuf_mtod_offset(inpkt, struct gtpu_hdr*, sizeof(ether_hdr) + sizeof(ipv4_hdr) + sizeof(udp_hdr));
    auto iniph = rte_pktmbuf_mtod_offset(inpkt, struct ipv4_hdr*, inip_ofst);

    uint32_t ipkey  = 0;
    // out side is always ipv4
    // extract keys from innner ipv4/v6
    if ((iniph->version_ihl&0xf0)==0x40) {
        auto ip4 = rte_pktmbuf_mtod_offset(inpkt, struct ipv4_hdr *, inip_ofst);
        ipkey = ip4->dst_addr;
    }else if ((iniph->version_ihl&0xf0)==0x60) {
        auto ip6 = rte_pktmbuf_mtod_offset(inpkt, struct ipv6_hdr *, inip_ofst);
        rte_memcpy(&ipkey, &ip6->dst_addr[4], 4);
    }else{
        counter_.Inc(rte_pktmbuf_pkt_len(inpkt), 1, 0, 0);
        PGW_LOG(RTE_LOG_ERR, "egress/ missing. ipv[4/6](%u)\n", (unsigned)inpkt->packet_type);
        rte_pktmbuf_free(inpkt);
        return(1);
    }
    // find from lookup table
    lookup_t itm;
    if (Find(ipkey&0xffffff00, &itm) != 0){
        counter_.Inc(rte_pktmbuf_pkt_len(inpkt), 1, 0, 0);
#ifndef NON_SUPPORTED_ERRIND
        error_indicate(inpkt);
        return(0);
#else
        // error indicator notify
        // forward GTPU packets to tap send-side ring
        // enable saving of error packets with common tools
        // such as tcpdump in userland processes.
        if (rte_ring_sp_enqueue(ring_err_ind_, inpkt) < 0){
            rte_pktmbuf_free(inpkt);
        }
        return(1);
#endif
    }
    // sgw:gtpu - set teid
    gtph->tid = itm.sgw_gtpu_teid;
    // restore udp port to default
    udp->dst_port = htons(GTPU_PORT);
    udp->src_port = htons(GTPU_PORT);
    // sgw dest ip address - gre source ipv4=use as pgw ipv4
    ip_ot->src_addr     = itm.pgw_gtpu_ipv4;
    ip_ot->dst_addr     = itm.sgw_gtpu_ipv4;
    ip_ot->hdr_checksum = 0;
    ip_ot->hdr_checksum = _wrapsum(_checksum(ip_ot, 20, 0));
    // swap mac
    auto t_macaddr = eh->d_addr;
    eh->d_addr = eh->s_addr;
    eh->s_addr = t_macaddr;
    //
#ifdef __PCAP_DEBUG__ 
    PGW_LOG(RTE_LOG_DEBUG, "egress.. pcap");
    pcap_open();
    pcap_append(0, (char*)eh, rte_pktmbuf_pkt_len(inpkt));
    pcap_close();
#endif
    return(0);
}

