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

    // Tapへ転送されるpacket 
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
    auto gtpu = rte_pktmbuf_mtod_offset(pkt, struct gtpu_hdr*, sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr));
    gtpu->tid = htonl(teid);
    gtpu->type = 0xff;
    auto ip = (struct ipv4_hdr*)(gtpu+1);

    ip->version_ihl     = 0x45;
    ip->src_addr        = saddr;
    ip->dst_addr        = daddr;
}
static inline int truncate_tunnel(void){
    Mysql   con;
    return(con.Query("DELETE FROM tunnel"));
}

static inline int insert_tunnel(const uint32_t teid, const uint32_t addr){
    Mysql   con;
    char    bf[128] = {0x00};
    std::string sql = "INSERT INTO tunnel (imsi,msisdn,ueipv4,pgw_teid,sgw_gtpu_teid,sgw_gtpu_ipv,active,pgw_gtpu_ipv,qos)VALUES";
    auto addrb = htonl(addr);
    sql += "(1,2,";
    sprintf(bf, "'%u.%u.%u.%u',",
            (uint8_t)((addrb>>24)&0xff),
            (uint8_t)((addrb>>16)&0xff),
            (uint8_t)((addrb>>8)&0xff),
            (uint8_t)(addrb&0xff));
    sql += bf;
    sprintf(bf, "%u,", (unsigned)teid);
    sql += bf;
    sql += bf;
    sql += "'2.2.2.2',1,'3.3.3.3',1) ON DUPLICATE KEY UPDATE pgw_teid = ";
    sql += bf;
    sql += "active = 1,updated_at=NOW(),pgw_gtpu_ipv=VALUES(`pgw_gtpu_ipv`)";
    //
    return(con.Query(sql.c_str()));
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
    gtpu->type = 0xff;
    auto ip = (struct ipv6_hdr*)(gtpu+1);
    ip->vtc_flow        = htonl(0x60000000);
    ip->payload_len     = rte_cpu_to_be_16(pktsize - 50);
    ip->hop_limits      = 0x40;

    for(auto n = 0;n < 16;n ++){
        ip->src_addr[n] = n;
        ip->dst_addr[15 - n] = n;
    }

    uint8_t x[16] = {0x00,0x00,0x00,0x00,
                     0xde,0xad,0xbe,0xef,
                     0x00,0x00,0x00,0x00,
                     0x00,0x00,0x00,0x00};
    memcpy(ip->src_addr,x,16);

    uint8_t y[16] = {0x00,0x00,0x00,0x00,
                     0x00,0x1f,0x3d,0xab,
                     0x00,0x00,0x00,0x00,
                     0x00,0x00,0x00,0x00};
    memcpy(ip->dst_addr,y,16);
}

static inline void printpkt(char *pkt, int len) {
    for(int i=0;i<=(len-1)/8;i++) {
        printf("%p ",pkt);
        for (int j = 0; j < 8; j++) {
            printf("0x%02hhx ",*pkt);
            pkt++;
        }
        printf("\n");
    }
}
