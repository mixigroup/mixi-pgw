/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       core_pgw_ingress_distributor_worker.cc
    @brief      PGW - Distributor ingress worker core implemented
*******************************************************************************
	PGW : distributor ingress : \n
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
   PGW - Distributor Ingress worker core interface  :  constructor \n
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
MIXIPGW::CorePgwIngressDistributorWorker::CorePgwIngressDistributorWorker(COREID lcore, TXT dburi, unsigned serverid, unsigned cpuid, unsigned greipv4, unsigned gresrcipv4,unsigned groupid,unsigned pgwid)
        :CorePgwBaseWorker(lcore, dburi, serverid, cpuid, greipv4, gresrcipv4, groupid, pgwid,1,TYPE::PGW_INGRESS){
    //
    PGW_LOG(RTE_LOG_INFO, "lcore: %u/dburi: %s/serverid: %u/cpuid: %u/greipv4: %08x/gresrcipv4: %08x/groupid: %u/pgwid: %u\n", lcore, dburi, serverid, cpuid, greipv4, gresrcipv4, groupid, pgwid);
}

/**
   PGW - Distributor Ingress worker core interface  : destructor\n
   *******************************************************************************
   destructor\n
   *******************************************************************************

 */
MIXIPGW::CorePgwIngressDistributorWorker::~CorePgwIngressDistributorWorker(){}
/**
   get core type.\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return TYPE  return core type[worker]
 */
TYPE MIXIPGW::CorePgwIngressDistributorWorker::GetType(void){
    return(TYPE::PGW_INGRESS);
}
/**
   object name\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return TXT  object name 
 */
TXT MIXIPGW::CorePgwIngressDistributorWorker::GetObjectName(void){
    return("CorePgwIngressDistributorWorker");
}

/**
   convert GTPU capsul to GRE capsul.\n
   *******************************************************************************
   CoreEncapWorker process encapsulates GTPU packet with ipv4.\n
   *******************************************************************************
   @param[in]     inpkt change encapsulate target packet 
 */
RETCD MIXIPGW::CorePgwIngressDistributorWorker::ModifyPacket(struct rte_mbuf* inpkt){
    static const unsigned char nops[12]= {0x01,0x01,0x01,0x01, 0x01,0x01,0x01,0x01, 0x01,0x01,0x01,0x01};
    static const unsigned char greo[4] = {0x00,0x00,0x08,0x00};
    //
    auto eh = rte_pktmbuf_mtod_offset(inpkt, struct ether_hdr*, 0);
    auto ip = rte_pktmbuf_mtod_offset(inpkt, struct ipv4_hdr*, sizeof(ether_hdr));
    auto nop= rte_pktmbuf_mtod_offset(inpkt, char*, sizeof(ether_hdr) + sizeof(ipv4_hdr));
    auto gre= rte_pktmbuf_mtod_offset(inpkt, char*, sizeof(ether_hdr) + sizeof(ipv4_hdr) + sizeof(nops));
    auto gtph= rte_pktmbuf_mtod_offset(inpkt, struct gtpu_hdr*, sizeof(ether_hdr) + sizeof(ipv4_hdr) + sizeof(udp_hdr));
    auto inip_ofst  = (sizeof(ether_hdr) + sizeof(ipv4_hdr) + sizeof(udp_hdr) + sizeof(gtpu_hdr));
    auto iniph = rte_pktmbuf_mtod_offset(inpkt, struct ipv4_hdr*, inip_ofst);
    lookup_t itm;

    uint32_t ipkey  = 0;

    if ((iniph->version_ihl&0xf0)==0x40) {
        auto ip4 = rte_pktmbuf_mtod_offset(inpkt, struct ipv4_hdr *, inip_ofst);
        ipkey = ip4->src_addr;
    }else if ((iniph->version_ihl&0xf0)==0x60) {
        auto ip6 = rte_pktmbuf_mtod_offset(inpkt, struct ipv6_hdr *, inip_ofst);
        rte_memcpy(&ipkey, &ip6->src_addr[4], 4);
    }else{
        counter_.Inc(rte_pktmbuf_pkt_len(inpkt), 1, 0, 0);
        PGW_LOG(RTE_LOG_ERR, "ingress/ missing. ipv[4/6](%u)\n", (unsigned)inpkt->packet_type);
        rte_pktmbuf_free(inpkt);
        return(1);
    }
    
    if (Find(ntohl(gtph->tid), &itm) != 0){
        counter_.Inc(rte_pktmbuf_pkt_len(inpkt), 1, 0, 0);
#ifdef __TEST_MODE__
        // error indicator notify
        error_indicate(inpkt);
#else
        // forward GTPU packet to tap send-side ring.
        // enable saving of error packets with common tools
        // such as tcpdump in userland processes.
        if (rte_ring_sp_enqueue(ring_err_ind_, inpkt) < 0){
            rte_pktmbuf_free(inpkt);
            PGW_LOG(RTE_LOG_ERR, "rte_ring_sp_enqueue - find(%p)\n", ring_err_ind_);
            return(1);
        }
#endif
        return(0);
    }
    // error notify even if ip address is different.
    auto ue_ipv4_cmp = (itm.ue_ipv4&0xffffff00);
    auto src_addr_cmp= (ipkey&0xffffff00);
    if (ue_ipv4_cmp != src_addr_cmp){
        counter_.Inc(rte_pktmbuf_pkt_len(inpkt), 1, 0, 0);
        PGW_LOG(RTE_LOG_ERR, "err ind diff ip(%08x - %08x: %08x - %08x)\n", itm.ue_ipv4, ipkey, ue_ipv4_cmp, src_addr_cmp);
        // error indicator notify - response error on dpdk.
 //     error_indicate(inpkt);

        // forward GTPU packet to tap send-side ring.
        // enable saving of error packets with common tools
        // such as tcpdump in userland processes.
        if (rte_ring_sp_enqueue(ring_err_ind_, inpkt) < 0){
            rte_pktmbuf_free(inpkt);
            PGW_LOG(RTE_LOG_ERR, "rte_ring_sp_enqueue(%p)\n", ring_err_ind_);
            return(1);
        }
        return(0);
    }
#ifndef __CANNOT_USE_GRE__ 
    // rewrite out-side destination ip address
    //  to GRE termination address. 
    ip->version_ihl         = 0x48;           // version4 + ip option 2
    ip->src_addr            = htonl(gresrcipv4_);
    ip->dst_addr            = htonl(greipv4_);
    ip->next_proto_id       = IPPROTO_GRE;    // gre
    ip->hdr_checksum        = 0;
    // fill udp header(was originally located) region to ip-option(0x01=nop) 8 octets.
    rte_memcpy(nop, nops, sizeof(nops));
    // set GTPU header(was originally located) region to GRE
    rte_memcpy(gre, greo, sizeof(greo));
    // ip checksum
    ip->hdr_checksum        = _wrapsum(_checksum(ip, 0x08<<2,0));
    // swap mac
    auto t_macaddr = eh->d_addr;
    eh->d_addr = eh->s_addr;
    eh->s_addr = t_macaddr;

    // swap mac
    inpkt->packet_type = (RTE_PTYPE_L3_IPV4 | RTE_PTYPE_TUNNEL_GRE);
#else
    ip->src_addr            = htonl(gresrcipv4_);
    ip->dst_addr            = htonl(greipv4_);
    ip->hdr_checksum        = 0;
    ip->hdr_checksum        = _wrapsum(_checksum(ip, 20,0));
    // swap mac
    auto t_macaddr = eh->d_addr;
    eh->d_addr = eh->s_addr;
    eh->s_addr = t_macaddr;
    
#endif
    //
    return(0);
}

