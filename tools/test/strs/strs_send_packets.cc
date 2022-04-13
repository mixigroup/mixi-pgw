#include "strs.h"

int send_packets(thread_arg_ptr targ, struct netmap_ring *ring, struct pkt *pkt, int size, u_int count){
    u_int n = 0, sent = 0, cur = ring->cur;
    // maximum of free transmission buffers.
    count = MIN(nm_ring_space(ring), count);
    //
    for (sent = 0; sent < count; sent++) {
        struct netmap_slot *slot = &ring->slot[cur];
        char *p = NETMAP_BUF(ring, slot->buf_idx);
        //
        slot->flags = 0;
        update_packet(pkt, targ);
        nm_pkt_copy(pkt, p, size);
     // append_pcap(pcapf_, (const char*)pkt, size);
//      memcpy(p, pkt, size);
        slot->len = size;
        slot->flags |= NS_REPORT;
        cur = nm_ring_next(ring, cur);
    }
    ring->head = ring->cur = cur;
    //
    return (sent);
}
//
void update_packet(struct pkt* pkt, thread_arg_ptr targ){
    uint32_t* pcnt = (uint32_t*)(pkt->body + sizeof(pkt->ip)+sizeof(struct udphdr) + sizeof(gtpu_header_t));
    if (targ->id < 0 || targ->id > 127){ return; }
    // send counter
    if (targ->counter != 0 && ((*pcnt)+1) != targ->counter){
       ERR("%u != %u", (*pcnt), (uint32_t)targ->counter);
    }
    //
    (*pcnt) = targ->counter;
    if ((targ->counter++) > 0x7fffffff){
        targ->counter = 0;
    }
    uint16_t rnd,oct,rndport;
    uint32_t ipt;
    //
    if (targ->seq_rand==0){
        rnd = (uint16_t)(targ->randidx[(targ->counter%targ->ip_range)]);
        oct = ((uint16_t)(*pcnt)&0x0000ffff)+rnd;
        oct += (oct%10?0:1);
    }else{
        rnd = (uint16_t)(targ->counter%targ->ip_range)+1;
        oct = rnd;
    }
//  ipt = (0x0A1E0000) | oct;
    ipt = (0x0A000000) | oct;

#ifdef __IPV6__
    ipt = htonl(ipt);
    uint8_t ip6dst[16] = {
        0xfc,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,
    };
    memcpy(&ip6dst[4], &ipt, 4);
    memcpy(pkt->ip.ip6_dst.s6_addr, ip6dst, sizeof(ip6dst));
//    memcpy(pkt->ip.ip6_src.s6_addr, ip6dst, sizeof(ip6dst));
    
#else
    pkt->ip.ip_dst.s_addr = htonl(ipt);
    pkt->ip.ip_src.s_addr = htonl(ipt+1);
    pkt->ip.ip_sum = 0;
    pkt->ip.ip_sum = checksum(&pkt->ip, sizeof(struct ip), 0);
    pkt->ip.ip_sum = htons(pkt->ip.ip_sum);
#endif
    // add port
    if (targ->port_range){
        rndport = 10000+(targ->counter%targ->port_range);
    }else{
        rndport = 11234;
    }
    pkt->udp.uh_dport = rndport+1;
    pkt->udp.uh_sport = rndport+2;
}
struct timespec wait_time(struct timespec ts){
    for (;;) {
        struct timespec w, cur;
        clock_gettime(CLOCK_REALTIME_PRECISE, &cur);
        w = timespec_sub(ts, cur);
        if (w.tv_sec < 0){
            return cur;
        }else if (w.tv_sec > 0 || w.tv_nsec > 1000000){
            poll(NULL, 0, 1);
        }
    }
}

void init_randidx(void){
    for(int n = 0;n < 16;n++){
        thread_arg_ptr t = ref_thread_arg(n);
        for(int m = 0;m < 65536;m++){
            t->randidx[m] = (rand()%65534);
        }
    }
}
