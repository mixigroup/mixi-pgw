static inline void set_packet_udpip(struct rte_mbuf* pkt,
                                    const uint8_t smac,
                                    const uint8_t dmac,
                                    const uint32_t saddr,
                                    const uint32_t daddr,
                                    const uint16_t sport,
                                    const uint16_t dport,
                                    const uint16_t pktsize){
    //
    auto eh = rte_pktmbuf_mtod(pkt, struct ether_hdr*);
    auto ip = (struct ipv4_hdr*)(eh+1);
    auto udp= (struct udp_hdr*)(ip+1);
    bzero(eh, pktsize);
    //
    eh->ether_type      = rte_cpu_to_be_16(ETHER_TYPE_IPv4);
    memset(&eh->s_addr.addr_bytes,smac,ETHER_ADDR_LEN);
    memset(&eh->d_addr.addr_bytes,dmac,ETHER_ADDR_LEN);

    // ip version 4
    ip->version_ihl     = (IP_VERSION | 0x05);
    ip->type_of_service = 0;
    ip->packet_id       = 0;
    ip->fragment_offset = IP_DN_FRAGMENT_FLAG;
    ip->time_to_live    = IP_DEFTTL;
    ip->next_proto_id   = IPPROTO_UDP;
    ip->hdr_checksum    = 0;
    ip->src_addr        = saddr;
    ip->dst_addr        = daddr;
    ip->total_length    = rte_cpu_to_be_16(pktsize - sizeof(*eh));
    ip->hdr_checksum    = 0;
    //
    udp->src_port       = htons(sport);
    udp->dst_port       = htons(dport);
    //
    pkt->pkt_len = pkt->data_len = pktsize;
    pkt->nb_segs = 1;
    pkt->next = NULL;
    pkt->packet_type = (RTE_PTYPE_L3_IPV4 | RTE_PTYPE_L4_UDP);
}

static inline void set_packet_udpip6(struct rte_mbuf* pkt,
                                    const uint8_t smac,
                                    const uint8_t dmac,
                                    const uint8_t *saddr,
                                    const uint8_t *daddr,
                                    const uint16_t sport,
                                    const uint16_t dport,
                                    const uint16_t pktsize){
    //
    auto eh = rte_pktmbuf_mtod(pkt, struct ether_hdr*);
    auto ip = (struct ipv6_hdr*)(eh+1);
    auto udp= (struct udp_hdr*)(ip+1);
    bzero(eh, pktsize);
    //
    eh->ether_type      = rte_cpu_to_be_16(ETHER_TYPE_IPv6);
    memset(&eh->s_addr.addr_bytes,smac,ETHER_ADDR_LEN);
    memset(&eh->d_addr.addr_bytes,dmac,ETHER_ADDR_LEN);

    // ip version 6
    ip->vtc_flow = 0x60000000;
    ip->payload_len = rte_cpu_to_be_16(pktsize - sizeof(*ip) - sizeof(*eh));
    ip->proto   = IPPROTO_UDP;
    ip->hop_limits = 0x01;
    for(auto i=0; i<16;i++){
        ip->src_addr[i]        = saddr[i];
        ip->dst_addr[i]        = daddr[i];
    }
    //
    udp->src_port       = htons(sport);
    udp->dst_port       = htons(dport);
    //
    pkt->pkt_len = pkt->data_len = pktsize;
    pkt->nb_segs = 1;
    pkt->next = NULL;
    pkt->packet_type = (RTE_PTYPE_L3_IPV6 | RTE_PTYPE_L4_UDP);
}

static inline void set_packet_arp(struct rte_mbuf* pkt,
    const uint8_t smac,
    const uint8_t dmac,
    const uint16_t arp_hrd,   /* format of hardware address */
    const uint16_t arp_pro,   /* format of protocol address */
    const uint8_t  arp_hln,   /* length of hardware address */
    const uint8_t  arp_pln,   /* length of protocol address */
    const uint16_t arp_op,    /* ARP opcode */
    const uint16_t pktsize){

    auto eh = rte_pktmbuf_mtod(pkt, struct ether_hdr*);
    auto arp = (struct arp_hdr*)(eh+1);
    bzero(eh,pktsize);
    // ether
    eh->ether_type = rte_cpu_to_be_16(ETHER_TYPE_ARP);
    memset(&eh->s_addr.addr_bytes,smac,ETHER_ADDR_LEN);
    memset(&eh->d_addr.addr_bytes,dmac,ETHER_ADDR_LEN);
    
    // arp
    arp->arp_hrd = rte_cpu_to_be_16(arp_hrd);
    arp->arp_pro = rte_cpu_to_be_16(arp_pro);
    arp->arp_hln = arp_hln;
    arp->arp_pln = arp_pln;
    arp->arp_op = rte_cpu_to_be_16(arp_op);

    pkt->pkt_len = pkt->data_len = pktsize;
    pkt->nb_segs = 1;
    pkt->next = NULL;
    pkt->packet_type = (RTE_PTYPE_L2_ETHER_ARP);
}

static inline void set_packet_ipv6_gre(
    struct rte_mbuf* mbuf,
    const uint8_t smac,
    const uint8_t dmac,
    const uint32_t saddr,
    const uint32_t daddr){

    auto eh = rte_pktmbuf_mtod(mbuf, struct ether_hdr*);
    auto ip = (struct ipv4_hdr*)(eh+1);
    auto gre= (uint32_t*)(((char*)(ip+1)) + 12/*  ip option */);
    auto ipi= (struct ipv6_hdr*)(gre+1);
    auto payload = (char*)(ipi + 1);
    eh->ether_type      = rte_cpu_to_be_16(ETHER_TYPE_IPv4);
    memset(&eh->s_addr.addr_bytes,smac,ETHER_ADDR_LEN);
    memset(&eh->d_addr.addr_bytes,dmac,ETHER_ADDR_LEN);
    //
    ip->version_ihl     = (IP_VERSION | 0x08);
    ip->type_of_service = 0;
    ip->packet_id       = 0;
    ip->fragment_offset = IP_DN_FRAGMENT_FLAG;
    ip->time_to_live    = IP_DEFTTL;
    ip->next_proto_id   = IPPROTO_GRE;
    ip->hdr_checksum    = 0;
    ip->src_addr        = saddr;
    ip->dst_addr        = daddr;
    ip->total_length    = rte_cpu_to_be_16(TEST_BUFFER_M_SZ - sizeof(*eh));
    ip->hdr_checksum    = 0;
    //

    ipi->vtc_flow        = htonl(0x60000000);
    ipi->payload_len     = rte_cpu_to_be_16(TEST_BUFFER_M_SZ - sizeof(*eh) - 36);
    ipi->proto           = IPPROTO_UDP;
    ipi->hop_limits      = 0x40;

    for(auto n = 0;n < 16;n ++){
        ipi->src_addr[n] = n;
        ipi->dst_addr[15 - n] = n;
    }

    mbuf->pkt_len = mbuf->data_len = TEST_BUFFER_M_SZ;
    mbuf->nb_segs = 1;
    mbuf->next = NULL;
    mbuf->packet_type = (RTE_PTYPE_L3_IPV6 | RTE_PTYPE_TUNNEL_GRE);

}

static inline void set_packet_ipv4_gre(
    struct rte_mbuf* mbuf,
    const uint8_t smac,
    const uint8_t dmac,
    const uint32_t saddr,
    const uint32_t daddr){

    auto eh = rte_pktmbuf_mtod(mbuf, struct ether_hdr*);
    auto ip = (struct ipv4_hdr*)(eh+1);
    auto gre= (uint32_t*)(((char*)(ip+1)) + 12/*  ip option */);
    auto ipi= (struct ipv4_hdr*)(gre+1);
    auto payload = (char*)(ipi + 1);
    eh->ether_type      = rte_cpu_to_be_16(ETHER_TYPE_IPv4);
    memset(&eh->s_addr.addr_bytes,smac,ETHER_ADDR_LEN);
    memset(&eh->d_addr.addr_bytes,dmac,ETHER_ADDR_LEN);
    //
    ip->version_ihl     = (IP_VERSION | 0x08);
    ip->type_of_service = 0;
    ip->packet_id       = 0;
    ip->fragment_offset = IP_DN_FRAGMENT_FLAG;
    ip->time_to_live    = IP_DEFTTL;
    ip->next_proto_id   = IPPROTO_GRE;
    ip->hdr_checksum    = 0;
    ip->total_length    = rte_cpu_to_be_16(TEST_BUFFER_M_SZ - sizeof(*eh));
    ip->hdr_checksum    = 0;
    //

    ipi->version_ihl     = (IP_VERSION | 0x05);
    ipi->type_of_service = 0;
    ipi->packet_id       = 0;
    ipi->fragment_offset = IP_DN_FRAGMENT_FLAG;
    ipi->time_to_live    = IP_DEFTTL;
    ipi->next_proto_id   = IPPROTO_UDP;
    ipi->hdr_checksum    = 0;
    ipi->src_addr        = saddr;
    ipi->dst_addr        = daddr;
    ipi->total_length    = rte_cpu_to_be_16(TEST_BUFFER_M_SZ - sizeof(*eh) - 28);
    ipi->hdr_checksum    = 0;
        
    mbuf->pkt_len = mbuf->data_len = TEST_BUFFER_M_SZ;
    mbuf->nb_segs = 1;
    mbuf->next = NULL;
    mbuf->packet_type = (RTE_PTYPE_L3_IPV4 | RTE_PTYPE_TUNNEL_GRE);

}
static inline void set_packet_gtpu(struct rte_mbuf* pkt,
                     const uint8_t smac,
                     const uint8_t dmac,
                     const uint32_t saddr,
                     const uint32_t daddr,
                     const uint16_t sport,
                     const uint16_t dport,
                     const uint32_t teid,
                     const uint16_t pktsize){
    set_packet_udpip(pkt, smac, dmac, saddr, daddr, sport, dport, pktsize);
    auto gtpu = rte_pktmbuf_mtod_offset(pkt, struct MIXIPGW::gtpu_hdr*, sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr));
    gtpu->tid = htonl(teid);
    auto ip = (struct ipv4_hdr*)(gtpu+1);
    ip->version_ihl     = (IP_VERSION | 0x05);
    ip->src_addr        = saddr;
    ip->dst_addr        = daddr;
}

static inline void set_packet_gtpu_ipv6(struct rte_mbuf* pkt,
                     const uint8_t smac,
                     const uint8_t dmac,
                     const uint32_t saddr,
                     const uint32_t daddr,
                     const uint16_t sport,
                     const uint16_t dport,
                     const uint32_t teid,
                     const uint16_t pktsize){
    set_packet_udpip(pkt, smac, dmac, saddr, daddr, sport, dport, pktsize);
    auto gtpu = rte_pktmbuf_mtod_offset(pkt, struct MIXIPGW::gtpu_hdr*, sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr));
    gtpu->tid = htonl(teid);
    auto ip = (struct ipv6_hdr*)(gtpu+1);
    ip->vtc_flow        = htonl(0x60000000);
    ip->payload_len     = rte_cpu_to_be_16(pktsize - 50);
    ip->hop_limits      = 0x40;

    for(auto n = 0;n < 16;n ++){
        ip->src_addr[n] = n;
        ip->dst_addr[15 - n] = n;
    }
}

