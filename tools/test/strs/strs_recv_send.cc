#include "strs.h"


#define NMRING(r)  ((struct netmap_ring*)r)
#define NMSLOT(s)  ((struct netmap_slot*)s)
#define NMDESC(n)  ((struct nm_desc*)n)
#define NMRING_SPACE(r)   nm_ring_space(NMRING(r))
#define NMRING_EMPTY(r)   nm_ring_empty(NMRING(r))
#define NMRING_NEXT(r,c)  nm_ring_next(NMRING(r), c);
#define WAIT_LINKSEC    (4)
#define NEED_SEND  (0x00000001)

static int process_ring(void* , void* , thread_arg_ptr );
static void swapmac(struct ether_header* );

//
void *strs_recv_send(void *data){
    thread_arg_ptr  t = (thread_arg_ptr)data;
    thread_arg_t    tcpy;
    memcpy(&tcpy, t, sizeof(tcpy));
    LOG("recv(response)  start.");
    struct pollfd pfd = { .fd = tcpy.fd, .events = POLLIN };
    struct netmap_ring *rxring;
    struct netmap_if *nifp;
    pcapf_ = start_pcap("/tmp/recv_send.pcap"); 


    // assign CPU to threads
    LOG("start.strs_recv_send, fd %d affinity %d burst  %d", tcpy.fd, tcpy.affinity, tcpy.burst);
    if (setaffinity(tcpy.thread, tcpy.affinity)){
        ERR("failed. setaffinity.");
        return(NULL);
    }
    // store start time.
    clock_gettime(CLOCK_REALTIME_PRECISE, &tcpy.tic);
    nifp = tcpy.nmd->nifp;
    while (!tcpy.cancel) {
        int n,m;
        u_int um = 0;
        uint64_t cur_space = 0,r,ttl_counter=0,e;
        //
        if (poll(&pfd, 1, 200) <0){
            if (pfd.revents & POLLERR){ break; }
        }
        if (!pfd.revents & POLLIN) { tcpy.cancel = t->cancel; continue; }
        auto rxringidx = tcpy.nmd->first_rx_ring;
        auto txringidx = tcpy.nmd->first_tx_ring;

        //
        while (rxringidx <= tcpy.nmd->last_rx_ring && txringidx <= tcpy.nmd->last_tx_ring) {
            void* rxr = NETMAP_RXRING(tcpy.nmd->nifp, rxringidx);
            void* txr = NETMAP_TXRING(tcpy.nmd->nifp, txringidx);
            if (NMRING_EMPTY(rxr)) {
                rxringidx++;
                continue;
            }
            if (NMRING_EMPTY(txr)) {
                txringidx++;
                continue;
            }
            process_ring(rxr, txr, &tcpy);
        }
        // burst synchronization with cores shared variables(report)
        t->pkts = tcpy.pkts;
        t->bytes = tcpy.bytes;
        t->recverr = tcpy.recverr;
        tcpy.cancel = t->cancel;
    }
    LOG("recv complete..");
    stop_pcap(pcapf_);

    return(NULL);
}


int process_ring(void* rxr, void* txr, thread_arg_ptr t){
    int ret = 0;
    int process_count = 0;
    uint32_t txring_space_work;
    auto rxring_cur_work = NMRING(rxr)->cur;                  // RX
    auto txring_cur_work = NMRING(txr)->cur;                  // TX
    auto limit = NMRING(rxr)->num_slots;

    /* print a warning if any of the ring flags is set (e.g. NM_REINIT) */
    if (NMRING(rxr)->flags || NMRING(txr)->flags){
        LOG("rxflags %x txflags %x", NMRING(rxr)->flags, NMRING(txr)->flags);
        NMRING(rxr)->flags = 0;
        NMRING(txr)->flags = 0;
    }
    //
    txring_space_work = NMRING_SPACE(rxr);
    if (txring_space_work < limit){
        limit = txring_space_work;
    }
    txring_space_work = NMRING_SPACE(txr);
    if (txring_space_work < limit){
        limit = txring_space_work;
    }
    while (limit-- > 0) {
        // init processed flag
        void *rxslot = &NMRING(rxr)->slot[rxring_cur_work];
        void *txslot = &NMRING(txr)->slot[txring_cur_work];
        char* req = NETMAP_BUF(NMRING(rxr), NMSLOT(rxslot)->buf_idx);
        char* res = NETMAP_BUF(NMRING(txr), NMSLOT(txslot)->buf_idx);
        uint16_t reslen = NMSLOT(rxslot)->len;
        uint16_t reqlen = NMSLOT(rxslot)->len;
        // prefetch the buffer for next loop.
        __builtin_prefetch(req);

        /* swap packets */
        if (NMSLOT(txslot)->buf_idx < 2 || NMSLOT(rxslot)->buf_idx < 2) {
            LOG("wrong index rx[%d] = %d  -> tx[%d] = %d",
                rxring_cur_work, NMSLOT(rxslot)->buf_idx, txring_cur_work, NMSLOT(txslot)->buf_idx);
            sleep(2);
        }
        /* copy the packet length. */
        if (NMSLOT(rxslot)->len > 2048) {
            LOG("wrong len %d rx[%d] -> tx[%d]", NMSLOT(rxslot)->len, rxring_cur_work, txring_cur_work);
            NMSLOT(rxslot)->len = 0;
        }
        // vlan == 4003
        auto vlanid = *(uint16_t*)(((struct ether_header*)req)+1);
        if (vlanid == htons(4003)){
            // just do response without counting, 1'st phase.
            swapmac((struct ether_header*)req);

            (*(uint16_t*)(((struct ether_header*)req)+1)) = htons(4004);
#if 1
#ifdef __IPV6__
            // ipv6
            struct in6_addr ta;
            auto tp = ((struct ip6_hdr*)(req+18));
            memcpy(ta.s6_addr, tp->ip6_src.s6_addr,sizeof(ta));
            memcpy(tp->ip6_src.s6_addr, tp->ip6_dst.s6_addr, sizeof(ta));
            memcpy(tp->ip6_dst.s6_addr, ta.s6_addr, sizeof(ta));
#else
            uint32_t ta;
            auto tp = (struct ip*)(req + 18);
            ta = tp->ip_src.s_addr;
//          LOG("src[%08x] dst[%08x]->", tp->ip_src.s_addr, tp->ip_dst.s_addr);
            tp->ip_src.s_addr = tp->ip_dst.s_addr;
            tp->ip_dst.s_addr = ta;
            tp->ip_sum = 0;
            tp->ip_sum = checksum(tp, sizeof(struct ip), 0);
            tp->ip_sum = htons(tp->ip_sum);

//          LOG("src[%08x] dst[%08x]\n", tp->ip_src.s_addr, tp->ip_dst.s_addr);
         
#endif 
#endif
            static uint64_t cap_count = 0;
            if (cap_count++ < 100){
                append_pcap(pcapf_, (const char*)req, reqlen);
            }

            auto tidx   = NMSLOT(txslot)->buf_idx;
            NMSLOT(txslot)->buf_idx = NMSLOT(rxslot)->buf_idx;
            NMSLOT(rxslot)->buf_idx = tidx;
            //
            NMSLOT(txslot)->len = reqlen;
            NMSLOT(txslot)->flags |= NS_BUF_CHANGED;
            NMSLOT(rxslot)->flags |= NS_BUF_CHANGED;
            //
            rxring_cur_work = NMRING_NEXT(rxr, rxring_cur_work);
            txring_cur_work = NMRING_NEXT(txr, txring_cur_work);
        }else{
            // counting
            t->pkts += 1;
            t->bytes += reqlen;
            t->recverr += 0;
            rxring_cur_work = NMRING_NEXT(rxr, rxring_cur_work);
        }
        process_count ++;
    }
    NMRING(rxr)->head = NMRING(rxr)->cur = rxring_cur_work;
    NMRING(txr)->head = NMRING(txr)->cur = txring_cur_work;

    return (process_count);
}

void swapmac(struct ether_header* eh){
    // swap dst <-> src
    uint8_t tmp_mac[ETHER_ADDR_LEN];
    memcpy(tmp_mac, eh->ether_dhost, ETHER_ADDR_LEN);
    memcpy(eh->ether_dhost, eh->ether_shost, ETHER_ADDR_LEN);
    memcpy(eh->ether_shost, tmp_mac, ETHER_ADDR_LEN);
}
