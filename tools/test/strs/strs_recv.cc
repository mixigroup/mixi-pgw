#include "strs.h"
//
void *strs_recv(void *data){
    thread_arg_ptr  t = (thread_arg_ptr)data;
    thread_arg_t    tcpy;
    memcpy(&tcpy, t, sizeof(tcpy));
    LOG("recv  start.");
    struct pollfd pfd = { .fd = tcpy.fd, .events = POLLIN };
    struct netmap_ring *rxring;
    struct netmap_if *nifp;

    // assign CPU to threads 
    LOG("start.strs_recv, fd %d affinity %d burst  %d", tcpy.fd, tcpy.affinity, tcpy.burst);
    if (setaffinity(tcpy.thread, tcpy.affinity)){
        ERR("failed. setaffinity.");
        return(NULL);
    }        
    // store start time.  
    clock_gettime(CLOCK_REALTIME_PRECISE, &tcpy.tic);
    nifp = tcpy.nmd->nifp;
    while (!tcpy.cancel) {
        int n,m;
        uint64_t cur_space = 0,r,ttl_counter=0,e;
        // 
        if (poll(&pfd, 1, 200) <0){
            if (pfd.revents & POLLERR){ break; }
        }
        for (n = tcpy.nmd->first_rx_ring; n <= tcpy.nmd->last_rx_ring; n++) {
            rxring = NETMAP_RXRING(nifp, n);
            m = rxring->head + rxring->num_slots - rxring->tail;
            if (m >=(int) rxring->num_slots){
                m -= rxring->num_slots;
            }
            cur_space += m;
            if (nm_ring_empty(rxring)){
                continue;
            }
            m = recv_packets(&tcpy, rxring, &r, &e);
            tcpy.pkts += m;
            tcpy.bytes = r;
            tcpy.recverr = e;
        }
        // burst synchronization with cores shared variables(report)
        t->pkts = tcpy.pkts;
        t->bytes = tcpy.bytes;
        t->recverr = tcpy.recverr;
        tcpy.cancel = t->cancel;
    }

    LOG("recv complete..");
    return(NULL);
}
