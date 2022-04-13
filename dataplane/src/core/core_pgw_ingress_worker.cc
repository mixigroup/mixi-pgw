/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       core_pgw_ingress_worker.cc
    @brief      PGW - ingress worker core implemented
*******************************************************************************
	PGW : ingress : \n
	\n
    \n
    \n
*******************************************************************************
    @date       created(30/oct/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 30/oct/2017 
      -# Initial Version
******************************************************************************/
#include "mixi_pgw_data_plane_def.hpp"
#include "../../inc/core.hpp"

using namespace MIXIPGW;

/**
   PGW - Ingress worker core interface  :  constructor \n
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
MIXIPGW::CorePgwIngressWorker::CorePgwIngressWorker(COREID lcore, TXT dburi, unsigned serverid, unsigned cpuid, unsigned greipv4, unsigned gresrcipv4,unsigned groupid,unsigned pgwid)
        :CorePgwBaseWorker(lcore, dburi, serverid, cpuid, greipv4, gresrcipv4, groupid, pgwid,1, TYPE::PGW_INGRESS){
    //
    PGW_LOG(RTE_LOG_INFO, "lcore: %u/dburi: %s/serverid: %u/cpuid: %u/greipv4: %08x/gresrcipv4: %08x/groupid: %u/pgwid: %u\n", lcore, dburi, serverid, cpuid, greipv4, gresrcipv4, groupid, pgwid);
}

/**
   PGW - Ingress worker core interface  : destructor\n
   *******************************************************************************
   destructor\n
   *******************************************************************************

 */
MIXIPGW::CorePgwIngressWorker::~CorePgwIngressWorker(){}
/**
   get core type.\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return TYPE  return core type[worker]
 */
TYPE MIXIPGW::CorePgwIngressWorker::GetType(void){
    return(TYPE::PGW_INGRESS);
}
/**
   object name\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return TXT  object name 
 */
TXT MIXIPGW::CorePgwIngressWorker::GetObjectName(void){
    return("CorePgwIngressWorker");
}
/**
   convert GTPU capsul to GRE capsul.\n
   *******************************************************************************
   CoreEncapWorker process encapsulates GTPU packet with ipv4.\n
   *******************************************************************************
   @param[in]     inpkt change encapsulate target packet 
 */
RETCD MIXIPGW::CorePgwIngressWorker::ModifyPacket(struct rte_mbuf* inpkt){
    static const unsigned char nops[12] = {0x01,0x01,0x01,0x01, 0x01,0x01,0x01,0x01, 0x01,0x01,0x01,0x01};
    static const unsigned char greo[4] = {0x00,0x00,0x80,0x00};
    //
    auto ip = rte_pktmbuf_mtod_offset(inpkt, struct ipv4_hdr*, sizeof(ether_hdr));
    auto nop= rte_pktmbuf_mtod_offset(inpkt, char*, sizeof(ether_hdr) + sizeof(ipv4_hdr));
    auto gre= rte_pktmbuf_mtod_offset(inpkt, char*, sizeof(ether_hdr) + sizeof(ipv4_hdr) + sizeof(nops));
    auto gtph= rte_pktmbuf_mtod_offset(inpkt, struct gtpu_hdr*, sizeof(ether_hdr) + sizeof(ipv4_hdr) + sizeof(udp_hdr));

    lookup_t    find;

    if (Find(ntohl(gtph->tid), &find) != 0){
        rte_pktmbuf_free(inpkt);
        return(1);
    }

    // rewrite out-side destination ip address
    //  to GRE termination address. 
    ip->version_ihl         = 0x48;           // version4 + ip option 2
    ip->src_addr            = gresrcipv4_;
    ip->dst_addr            = greipv4_;
    ip->next_proto_id       = IPPROTO_GRE;    // gre
    ip->hdr_checksum        = 0;
    // fill udp header(was originally located) region to ip-option(0x01=nop) 8 octets.
    rte_memcpy(nop, nops, sizeof(nops));
    // set GTPU header (was originally located) region to GRE.
    rte_memcpy(gre, greo, sizeof(greo));
    //
    inpkt->packet_type = (RTE_PTYPE_L3_IPV4 | RTE_PTYPE_TUNNEL_GRE);
    //
    return(0);
}

