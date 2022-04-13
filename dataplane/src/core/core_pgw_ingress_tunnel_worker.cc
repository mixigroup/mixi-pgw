/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       core_pgw_ingress_tunnel_worker.cc
    @brief      PGW - ingress tunnle worker core implemented
*******************************************************************************
	PGW : ingress tunnel mode : \n
*******************************************************************************
    @date       created(27/apr/2018)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 27/apr/2018 
      -# Initial Version
******************************************************************************/
#include "mixi_pgw_data_plane_def.hpp"
#include "../../inc/core.hpp"

using namespace MIXIPGW;

/**
   PGW - Ingress tunnel mode worker core interface  :  constructor \n
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
MIXIPGW::CorePgwIngressTunnelWorker::CorePgwIngressTunnelWorker(COREID lcore, TXT dburi, unsigned serverid, unsigned cpuid, unsigned greipv4, unsigned gresrcipv4,unsigned groupid,unsigned pgwid)
        :CorePgwBaseWorker(lcore, dburi, serverid, cpuid, greipv4, gresrcipv4, groupid, pgwid,1,TYPE::PGW_INGRESS){
    //
    PGW_LOG(RTE_LOG_INFO, "lcore: %u/dburi: %s/serverid: %u/cpuid: %u/greipv4: %08x/gresrcipv4: %08x/groupid: %u/pgwid: %u\n", lcore, dburi, serverid, cpuid, greipv4, gresrcipv4, groupid, pgwid);
}

/**
   PGW - Ingress tunnle mode worker core interface  : destructor\n
   *******************************************************************************
   destructor\n
   *******************************************************************************

 */
MIXIPGW::CorePgwIngressTunnelWorker::~CorePgwIngressTunnelWorker(){}
/**
   get core type.\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return TYPE  return core type[worker]
 */
TYPE MIXIPGW::CorePgwIngressTunnelWorker::GetType(void){
    return(TYPE::PGW_INGRESS);
}
/**
   object name\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return TXT  object name 
 */
TXT MIXIPGW::CorePgwIngressTunnelWorker::GetObjectName(void){
    return("CorePgwIngressTunnelWorker");
}

/**
   tunnel mode PGW - ingress \n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]     inpkt change encapsulate target packet 
 */
RETCD MIXIPGW::CorePgwIngressTunnelWorker::ModifyPacket(struct rte_mbuf* inpkt){
    auto eh = rte_pktmbuf_mtod_offset(inpkt, struct ether_hdr*, 0);
    auto ip = rte_pktmbuf_mtod_offset(inpkt, struct ipv4_hdr*, sizeof(ether_hdr));
    auto gtph= rte_pktmbuf_mtod_offset(inpkt, struct gtpu_hdr*, sizeof(ether_hdr) + sizeof(ipv4_hdr) + sizeof(udp_hdr));
    auto inip_ofst  = (sizeof(ether_hdr) + sizeof(ipv4_hdr) + sizeof(udp_hdr) + sizeof(gtpu_hdr));
    auto iniph = rte_pktmbuf_mtod_offset(inpkt, struct ipv4_hdr*, inip_ofst);
    lookup_t itm;
    uint32_t ipkey  = 0;
    auto gtph_tid = gtph->tid;
    udat_ = (mbuf_userdat_ptr)&(inpkt->udata64);
    bzero(udat_, sizeof(*udat_));
    //
    auto opr = is_pdu(inpkt);
    if (opr == PACKET_TYPE::PASS){
        udat_->flag = MBFU_FLAG_DC;
        return(0);
    }
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
    //6rd
    if ((iniph->version_ihl&0xf0)==0x60){
        auto len = sizeof(struct udp_hdr) + sizeof(struct gtpu_hdr);
        if (!rte_pktmbuf_adj(inpkt, len)){
            counter_.Inc(rte_pktmbuf_pkt_len(inpkt), 1, 0, 0);
            rte_pktmbuf_free(inpkt);
            PGW_LOG(RTE_LOG_ERR, "failed. decap\n");
            return(1);
        }
        eh->ether_type = rte_cpu_to_be_16(ETHER_TYPE_IPv4);
        inpkt->packet_type &= ~RTE_PTYPE_L3_IPV6;
        inpkt->packet_type |= RTE_PTYPE_L3_IPV4;
        memcpy(rte_pktmbuf_mtod(inpkt, char*), eh, sizeof(*eh));

        auto ip_cap = rte_pktmbuf_mtod_offset(inpkt, struct ipv4_hdr*, sizeof(ether_hdr));
        bzero(ip_cap,sizeof(*ip_cap));
        ip_cap->version_ihl = 0x45;
        ip_cap->type_of_service = 0x00;
        ip_cap->total_length = htons(gtph->length + sizeof(struct ipv4_hdr));
        ip_cap->packet_id = 0;
        ip_cap->fragment_offset = 0;
        ip_cap->time_to_live = 0xff;
        ip_cap->next_proto_id = IPPROTO_IPV6;
        ip_cap->hdr_checksum = 0;
        ip_cap->src_addr = (ipkey&0xffffff00)|(0x0000000a) ;
        ip_cap->dst_addr = htonl(SIXRD_ENCAPPER_ADDR);
        rte_ipv4_cksum(ip_cap);
        udat_->size = (rte_pktmbuf_pkt_len(inpkt) - sizeof(ether_hdr) - sizeof(ipv4_hdr));
    }else{
        auto len = sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr) + sizeof(struct gtpu_hdr);
        if (!rte_pktmbuf_adj(inpkt, len)){
            counter_.Inc(rte_pktmbuf_pkt_len(inpkt), 1, 0, 0);
            rte_pktmbuf_free(inpkt);
            PGW_LOG(RTE_LOG_ERR, "failed. decap\n");
            return(1);
        }
        memcpy(rte_pktmbuf_mtod(inpkt, char*), eh, sizeof(*eh));

        inpkt->packet_type |= RTE_PTYPE_L3_IPV4;
        PGW_LOG(RTE_LOG_DEBUG, "ipv4[%02x : %02x]\n", ip->version_ihl, ip->version_ihl&0xf0);

        udat_->size = (rte_pktmbuf_pkt_len(inpkt) - sizeof(ether_hdr));
    }
    udat_->color= POLICER_COLOR_GREEN;
    udat_->mode = POLICER_MODE_LTE;
    findkey_    = ipkey;
    now_        = time(0);
    //
    if (Find(ntohl(gtph_tid),
             [](lookup_ptr itm, void* arg){
                 auto pthis = (CorePgwIngressTunnelWorker*)arg;
                 // current source is callback inside mutex lock.
                 // be careful with nested mutex locks.
                 auto ue_ipv4_cmp = (itm->ue_ipv4&0xffffff00);
                 auto src_addr_cmp= (pthis->findkey_&0xffffff00);
                 if (ue_ipv4_cmp != src_addr_cmp){
                     return(-1);
                 }
                 /*
                              +---------------------------------+
                              |periodically every T sec.        |
                              | Tc(t+)=MIN(CBS, Tc(t-)+CIR*T)   |
                              | Te(t+)=MIN(EBS, Te(t-)+EIR*T)   |
                              +---------------------------------+
                     Packet of size
                         B arrives   /----------------\
                    ---------------->|color-blind mode|
                                     |       OR       |YES  +---------------+
                                     |  green packet  |---->|packet is green|
                                     |      AND       |     |Tc(t+)=Tc(t-)-B|
                                     |    B <= Tc(t-) |     +---------------+
                                     \----------------/
                                             |
                                             | NO
                                             v
                                     /----------------\
                                     |color-blind mode|
                                     |       OR       |YES  +----------------+
                                     | NOT red packet |---->|packet is yellow|
                                     |      AND       |     |Te(t+)=Te(t-)-B |
                                     |    B <= Te(t-) |     +----------------+
                                     \----------------/
                                             |
                                             | NO
                                             v
                                     +---------------+
                                     |packet is red  |
                                     +---------------+
                  *
                  *
                  * */
                 // recalculation counter has advanced since last calculation
                 //     -> refill tokens for seconds elapsed.
                 auto dt = (pthis->now_ - itm->epoch_w);
                 if (dt){
                     itm->commit_rate = MIN(itm->commit_burst_size, itm->commit_rate + (itm->commit_information_rate * dt));
                     itm->excess_rate = MIN(itm->excess_burst_size, itm->excess_rate + (itm->excess_information_rate * dt));
                     itm->epoch_w = pthis->now_;
                 }
#ifdef __TEST_MODE__
		 PGW_LOG(RTE_LOG_ERR, "CorePgwIngressTunnelWorker::ModifyPacket/any color(%u - %u:%u) %u /%u\n",
                        dt,
                        itm->epoch_w,
                        pthis->udat_->size,
                        itm->commit_rate,
                        itm->excess_rate);
#endif
                 if (pthis->udat_->size <= itm->commit_rate){
                     // green
                     itm->commit_rate = (itm->commit_rate - pthis->udat_->size);
                     pthis->udat_->color = POLICER_COLOR_GREEN;
                 }else if (pthis->udat_->size <= itm->excess_rate){
                     // yellow
                     itm->excess_rate = (itm->excess_rate - pthis->udat_->size);
                     pthis->udat_->color = POLICER_COLOR_YELLOW;
                 }else{
                     // red
                     pthis->udat_->color = POLICER_COLOR_RED;
                 }
                 pthis->udat_->teid = ue_ipv4_cmp;
                 pthis->lookup_itm_ = (*itm);
                 return(0);
             }, this) != 0){
        counter_.Inc(rte_pktmbuf_pkt_len(inpkt), 1, 0, 0);
        // forward GTPU packet to tap send-side ring.
        // enable saving of error packets with common tools
        // such as tcpdump in userland processes.
        if (ring_err_ind_){
            if (rte_ring_sp_enqueue(ring_err_ind_, inpkt) < 0){
                rte_pktmbuf_free(inpkt);
                PGW_LOG(RTE_LOG_ERR, "rte_ring_sp_enqueue - find(%p)\n", ring_err_ind_);
            }
            return(1);
        }
        udat_->flag = MBFU_FLAG_DC;
        return(0);
    }
    // use rte_mbuf.userdata
    udat_->flag = MBFU_FLAG_PDU;
    udat_->mode = lookup_itm_.stat.mode;

    return(0);
}

