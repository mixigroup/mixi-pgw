#include "strs.h"

int recv_packets(thread_arg_ptr targ, struct netmap_ring *ring, uint64_t *size, uint64_t *err){
    u_int cur, rx, limit;

    if (size == NULL || err == NULL){ return(-1); }
    cur = ring->cur;
    limit = nm_ring_space(ring);
    for (rx = 0; rx < limit; rx++) {
        struct netmap_slot *slot = &ring->slot[cur];
        char *p = NETMAP_BUF(ring, slot->buf_idx);
        (*size) += (uint64_t)slot->len;
        check_packet((struct pkt*)p, targ, slot->len);
#ifdef __IPV6__
        if((((struct pkt*)p)->ip.ip6_flow&0x03)==0x03){
            (*err) += 1;
        }
#else
        if((((struct pkt*)p)->ip.ip_tos&0x03)==0x03){
            (*err) += 1;
        }
#endif
        cur = nm_ring_next(ring, cur);
    }
    ring->head = ring->cur = cur;
    return (rx);
}
//
void check_packet(struct pkt* pkt, thread_arg_ptr targ, uint64_t size){
#ifdef __IPV6__
    uint32_t* pcnt = (uint32_t*)(pkt->body + 40/* ipv4-header size */+sizeof(struct udphdr) + sizeof(gtpu_header_t));
#else
    uint32_t* pcnt = (uint32_t*)(pkt->body + 20/* ipv4-header size */+sizeof(struct udphdr) + sizeof(gtpu_header_t));
#endif
    // packet length must be longer than 64 bytes
    if (size < (sizeof(struct ether_header)+4+sizeof(struct ip)*2+sizeof(struct udphdr)*2+sizeof(gtpu_header_t)+sizeof(uint32_t))){
        return;
    }

    // vlan for debug.
    if (targ->vlanid == 4002){ return; }
   
    if (targ->id < 0 || targ->id > 127){ return; }
    // start checking from first sequence number
    if (targ->counter == 0){
        targ->counter = (*pcnt)+1;
    }else{
        if (targ->counter != 0x7fffffff && targ->counter != (*pcnt)){
            targ->recverr ++;
            fprintf(stderr, "err?(%u - %u)", targ->counter, (*pcnt));
        }
        targ->counter = (*pcnt)+1;
        if (targ->counter >= 0x7fffffff){
            targ->counter = 0;
        }  
  } 
}
