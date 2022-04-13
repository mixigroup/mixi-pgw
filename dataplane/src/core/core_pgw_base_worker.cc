/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       core_pgw_base_worker.cc
    @brief      PGW - worker core base implement
*******************************************************************************
	PGW : basicaly : \n
	\n
    \n
    \n
*******************************************************************************
    @date       created(31/oct/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 31/oct/2017 
      -# Initial Version
******************************************************************************/

#ifdef GCC_VERSION
#undef GCC_VERSION
#endif
#include "mixi_pgw_data_plane_def.hpp"
#include "../../inc/core.hpp"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "lookup_tpl.hpp"
#include "mixi-binlog.h"

#define PGW_TBLNM       ("tunnel")

using namespace MIXIPGW;
namespace mx = MIXIBINLOG;

namespace MIXIBINLOG_CLIENT {
    typedef enum TUNNEL_COL_{
        T_INTIDX = 0,
        T_IMSI,         T_MSISDN,       T_UEIPV4,       T_PGW_TEID,
        T_PGW_GTPC_IPV, T_PGW_GTPU_IPV, T_DNS,          T_EBI,
        T_SGW_GTPC_TEID,T_SGW_GTPC_IPV, T_SGW_GTPU_TEID,T_SGW_GTPU_IPV,
        T_POLICY,       T_ACTIVE,       T_BITRATE_S5,   T_BITRATE_SGI,
        T_QCI,          T_QOS,          T_TEID_MASK,    T_PRIORITY,
        T_UPDATED_AT
    }TUNNEL_COL;

    // notify callback defined.
    class NotifyImpl:
        public mx::Notify
    {
    public:
        virtual int OnNotify(
            int event,
            int clmnid,
            int type,
            void* value,
            int len,
            void* data
        ) override;
    };
    typedef struct record {
        int evt;
        lookup_t lnk;
    } record_t, *record_ptr;
}; // namespace MIXIBINLOG_CLIENT


#define LOOKUP(c)       ((Lookup<lookup_t, COUNT_TEIDGROUP_PER_CORE, unsigned>*)c)
#define MAP(c)          ((std::unordered_map<uint32_t, lookup_t>*)c)

#ifdef __USE_SPINLOCK__
#define LOCK()   rte_spinlock_lock(&maplock_)
#define UNLOCK() rte_spinlock_unlock(&maplock_)
#else
#define LOCK()   pthread_mutex_lock(&mapmtx_)
#define UNLOCK() pthread_mutex_unlock(&mapmtx_)
#endif



/**
   PGW - worker core interface  :  constructor \n
   *******************************************************************************
   constructor \n
   *******************************************************************************
   @param[in]     lcore         core id
   @param[in]     dburi         databaseuri
   @parma[in]     serverid      database server id
   @param[in]     cpuid         thread for host stack(binlog) cpucore
   @param[in]     greipv4       gre terminated ip address 
   @param[in]     gresrcipv4    gre terminated ip source address
   @param[in]     groupid       group id
   @param[in]     pgwid         PGW id
   @param[in]     findtype      find data type
 */
MIXIPGW::CorePgwBaseWorker::CorePgwBaseWorker(COREID lcore, TXT dburi, unsigned serverid, unsigned cpuid, unsigned greipv4, unsigned gresrcipv4,unsigned groupid,unsigned pgwid, unsigned findtype, unsigned coretype)
        :CoreInterface(0, 0, lcore),quit_(0),cpuid_(cpuid),greipv4_(greipv4),gresrcipv4_(gresrcipv4),groupid_(groupid),pgwid_(pgwid),findtype_(findtype),ring_err_ind_(NULL), ring_warmup_(NULL),warmup_count_(0),coretype_(coretype),serverid_(serverid){

    rte_spinlock_init(&maplock_);
    pthread_mutex_init(&mapmtx_,NULL);
    lookup_     = Lookup<lookup_t, COUNT_TEIDGROUP_PER_CORE, unsigned>::Create();
    map_        = new std::unordered_map<uint32_t, lookup_t>();
    dburi_      = dburi?dburi:"";
    binlog_     = NULL;
}

/**
   PGW - worker core interface  : destructor\n
   *******************************************************************************
   destructor\n
   *******************************************************************************
 */
MIXIPGW::CorePgwBaseWorker::~CorePgwBaseWorker(){
    quit_ = 1;
    delete LOOKUP(lookup_);
    delete MAP(map_);
}
/**
   set key value.\n
   *******************************************************************************
   set typical 32bit unsigned digits, key - value\n
   *******************************************************************************
   @param[in]     key key
   @param[in]     v   value
 */
void MIXIPGW::CorePgwBaseWorker::SetN(KEY key,VALUE v){
    CoreInterface::SetN(key, v);

    switch(key){
        case KEY::POLICER_CIR:    policer_cir_ = v;   break;
        case KEY::POLICER_CBS:    policer_cbs_ = v;   break;
        case KEY::POLICER_EIR:    policer_eir_ = v;   break;
        case KEY::POLICER_EBS:    policer_ebs_ = v;   break;
    }
}
/**
  obtain value find by key.\n
 *******************************************************************************
 override, call CoreInterface::GetN ,except KEY::OPT\n
 *******************************************************************************
 @param[in]     key   key
 @return VALUE  return value :0xffffffff=error
 */
VALUE MIXIPGW::CorePgwBaseWorker::GetN(KEY t){
    if (t == KEY::OPT){
        return(cpuid_);
    }
    return(CoreInterface::GetN(t));
}

/**
   pgw , connect woker core  to specific ring \n
   *******************************************************************************
   [FROM]   = ring connected to Rx worder
   [TO]     = ring connected to Tx worker
   [INT]    = internal dedicated software rings\n
   \n
   *******************************************************************************
   @param[in]     order       connection direction 
   @param[in]     ring        ring address 
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CorePgwBaseWorker::SetRingAddr(ORDER order, void* ring){
    if (order == ORDER::FROM){                      // from = from rx
        ring_fm_ = (struct rte_ring*)ring;
    }else if (order == ORDER::TO){                  // to   = to tx
        ring_to_ = (struct rte_ring*)ring;
    }else if (order == ORDER::EXTEND){              // ingress -> tap:tx
        ring_err_ind_ = (struct rte_ring*)ring;
    }else if (order == ORDER::FROM_00){             // tap:rx  -> ingress
        ring_warmup_ = (struct rte_ring*)ring;
    }else{
        rte_exit(EXIT_FAILURE, "unknown ring order.(%u)\n", order);
    }
    return(0);
}
/**
  called before cycle loop at once.\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]     arg  application instance address
 @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CorePgwBaseWorker::BeforeCycle(void* arg){
    int err;
    char user[32] = {0};
    char pwd[32] = {0};
    char host[32] = {0};
    unsigned int port = 0;
    bool reconnect = 1;
    char binlog_file[64] = {0};
    unsigned binlog_pos = 4;
    mx::Event* event = NULL;

    parse_mysql(dburi_.c_str(), user, sizeof(user)-1, pwd, sizeof(pwd)-1, host, sizeof(host)-1,&port);
    if (!user[0] || !pwd[0] || !host[0] || !port){
        rte_panic("invalid dburi :// (%s)\n", dburi_.c_str());
    }
    // instantiate.
    err = mx::Event::CreateEvent(
        PGW_TBLNM,
        host,
        user,
        pwd,
        "",
        4,
        &event
    );
    if (err) {
        rte_panic("failed. CreateEvent");
    }
    binlog_ = event; 

    return(0);
}

/**
   find by (teid of ipv4)\n
   *******************************************************************************
   \n
   \n
   *******************************************************************************
   @param[in]     key   teid or ipv4
   @param[out]    out   find results
   @return RETCD , 0=success,0!=error
 */
RETCD MIXIPGW::CorePgwBaseWorker::Find(uint32_t key, lookup_ptr out){
    // Distributor type, std::unorderd_map + mutex
    if (findtype_){
        LOCK();
        auto it = MAP(map_)->find(ntohl(key));
        if (it == MAP(map_)->end()){
            UNLOCK();
            PGW_LOG(RTE_LOG_ERR, "%s: notfound:%08x\n", GetType()==TYPE::PGW_EGRESS?"egress":"ingress",ntohl(key));
            return(-1);
        }
        if (out){
            (*out) = (it->second);
        }
        UNLOCK();
        return(0);
    }else{
        // same group id.
        if (GROUPID(key) != groupid_){
            PGW_LOG(RTE_LOG_ERR,"invalid groupid(%08x != %08x)\n", (unsigned)ntohl(key), (unsigned)groupid_);
            return(-1);
        }
        // validate with teid
        auto it = LOOKUP(lookup_)->Find(GROUPVAL(ntohl(key)), 0);
        if (it == LOOKUP(lookup_)->End()){
//          printf("not found teid or ipv4(%08x)\n", (unsigned)ntohl(key));
            return(-1);
        }
        if (out){
            (*out) = (*it);
        }
        return(0);
    }
}

/**
   find by (teid of ipv4)\n
   *******************************************************************************
   for update\n
   \n
   *******************************************************************************
   @param[in]     key   teid or ipv4
   @param[out]    out   find results(modify available)
   @param[out]    arg   user data
   @return RETCD , 0=success,0!=error
 */
RETCD MIXIPGW::CorePgwBaseWorker::Find(uint32_t key, OnFoundWithUpdateLock callback, void* arg){
    RETCD   ret = 0;
    if (!findtype_){
        PGW_LOG(RTE_LOG_ERR,"not implemented(%u)\n", (unsigned)findtype_);
        return(-1);
    }
    LOCK();
    auto it = MAP(map_)->find(ntohl(key));
    if (it == MAP(map_)->end()){
        UNLOCK();
        PGW_LOG(RTE_LOG_ERR, "%s: notfound:%08x\n", GetType()==TYPE::PGW_EGRESS?"egress":"ingress",ntohl(key));
        return(-1);
    }
    if (callback){
        ret = callback(&(it->second), arg);
    }
    UNLOCK();
    //
    return(ret);
}


/**
   pgw worker core : virtual cycle : 1 cycle\n
   *******************************************************************************
   \n
   \n
   *******************************************************************************
   @param[in]     arg  application instance address
   @param[in]     cnt  number of cycle
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CorePgwBaseWorker::Cycle(void* arg, uint64_t* cnt){
    int n = 0;

    // receive packet 
    while(1){
        auto nburst = rte_ring_sc_dequeue_burst(ring_fm_, (void**)input_, burst_fm_, NULL);
        if (unlikely(nburst == -ENOENT)) { break; }
        if (unlikely(nburst == 0)){ break; }
        // prefetch first.
        for (n  = 0; n < PREFETCH_OFFSET && n < nburst; n++) {
            rte_prefetch0(rte_pktmbuf_mtod(input_[n], void *));
        }
        // enqueue, while prefetching.
        for (n = 0; n < (nburst - PREFETCH_OFFSET) && nburst > PREFETCH_OFFSET; n++) {
            rte_prefetch0(rte_pktmbuf_mtod(input_[n + PREFETCH_OFFSET], void *));
            if (ModifyPacket(input_[n]) == 0){
                SendOrBuffer(arg, input_[n]);
            }
        }
        // remained.
        for (; n < nburst; n++) {
            if (ModifyPacket(input_[n]) == 0){
                SendOrBuffer(arg, input_[n]);
            }
        }
        break;
    }
    // tunnel warmup packet (from tap rx)
    // notifications from tap rx, limitted 20K pps maxtap
    // data plane processing operates at up to 14 Mpps and burst dequeues every cycle.
    // avarage burst -> x packets and process warm-up ring every 14M / x times.
    // design requirement for .
    //      warm-up ring delay < mysql-binlog delay
    //      (typical mysql-binlog latency is N --> 10N sec(N=1,2,3))
    while((warmup_count_++ > (14000000/burst_fm_))&& ring_warmup_ != NULL){
        uint32_t    ncore, nkey;
        auto type  = GetType();
        if (type == TYPE::PGW_EGRESS){
            ncore = GetN(KEY::CNT_PGW_EGRESS);
        }else{
            ncore = GetN(KEY::CNT_PGW_INGRESS);
        }
        warmup_count_ = 0;
        lookup_t itm;
        struct private_extension_hdr*   pehdr;
        const uint32_t baselen = (sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr) +
                                  sizeof(struct udp_hdr) + sizeof(struct gtpc_hdr));
        auto nburst = rte_ring_sc_dequeue_burst(ring_warmup_, (void**)input_warmup_, burst_fm_, NULL);
        if (unlikely(nburst == -ENOENT)) { break; }
        if (unlikely(nburst == 0)){ break; }
        //
        for (n  = 0; n < nburst; n++) {
            auto pktlen = rte_pktmbuf_pkt_len(input_warmup_[n]);
            pehdr = rte_pktmbuf_mtod_offset(input_warmup_[n], struct private_extension_hdr*, (pktlen - GTPC_PRIVATE_EXT_SIZE));
            // extension value is set in private extension.
            itm.ue_ipv4 = pehdr->value[UE_IPV4];
            itm.ue_teid = pehdr->value[UE_TEID];
            itm.sgw_gtpu_ipv4 = pehdr->value[SGW_GTPU_IPV4];
            itm.sgw_gtpu_teid = pehdr->value[SGW_GTPU_TEID];
            itm.pgw_gtpu_ipv4 = pehdr->value[PGW_GTPU_IPV4];

            if(itm.ue_ipv4 == 0){
                PGW_LOG(RTE_LOG_ERR,"Invalid warmup: ue_ipv4 is 0x00000000 \n");
                continue;
            }
            if(itm.ue_teid == 0){
                PGW_LOG(RTE_LOG_ERR,"Invalid warmup: ue_teid is 0x00000000 \n");
                continue;
            }
            if(itm.sgw_gtpu_ipv4 == 0){
                PGW_LOG(RTE_LOG_ERR,"Invalid warmup: sgw_gtpu_ipv4 is 0x00000000 \n");
                continue;
            }
            if(itm.sgw_gtpu_teid == 0){
                PGW_LOG(RTE_LOG_ERR,"Invalid warmup: sgw_gtpu_teid is 0x00000000 \n");
                continue;
            }
            if(itm.pgw_gtpu_ipv4 == 0){
                PGW_LOG(RTE_LOG_ERR,"Invalid warmup: pgw_gtpu_ipv4 is 0x00000000 \n");
                continue;
            }
            rte_pktmbuf_free(input_warmup_[n]);
            //
            nkey  = (type==TYPE::PGW_EGRESS?itm.ue_ipv4:itm.ue_teid);
            if (findtype_){
                LOCK();
                (*MAP(map_))[htonl(nkey)] = itm;
                UNLOCK();
                //
                PGW_LOG(RTE_LOG_INFO, "warmup[%s/ue_ipv4: %08x/ue_teid: %08x/sgw_ipv4: %08x/sgw_teid: %08x/pgw_ipv4: %08x]\n",
                        type==TYPE::PGW_EGRESS?"egress":"ingress",
                        type==TYPE::PGW_EGRESS?htonl(nkey):itm.ue_ipv4,
                        type==TYPE::PGW_EGRESS?itm.ue_teid:htonl(nkey),
                        itm.sgw_gtpu_ipv4,
                        itm.sgw_gtpu_teid,
                        itm.pgw_gtpu_ipv4);
            }else{
                if (ncore){
                    // supported, only self group
                    if ((nkey%ncore) == groupid_){
                        LOOKUP(lookup_)->Add(GROUPVAL(itm.ue_teid), &itm, 0);
                    }
                }else{
                    rte_exit(EXIT_FAILURE, "invalid config..ncore\n");
                }
            }
        }
        break;
    }
    // every xxx K times(depends on environment), flush burst buffer.
    if (flush_delay_ && (unlikely((*cnt) >= flush_delay_))) {
        BurstFlush(arg);
        (*cnt) = 0;
    }
    return(0);
}


// notify callback implementation.
int
MIXIBINLOG_CLIENT::NotifyImpl::OnNotify(
    int event,      /* event */
    int clmnid,     /* index of columns  */
    int type,       /* type of value(TYPE_NUMERIC, TYPE_TEXT, etc) */
    void* value,    /* column value */
    int len,        /* length of value */
    void* data      /* user-data */
)
{
    auto rec = (record_ptr)data;
    if (type == mx::Notify::TYPE_NUMERIC) {
        switch(clmnid) {
        case T_PGW_TEID:        rec->lnk.ue_teid = *((uint64_t*)value); break;
        case T_SGW_GTPU_TEID:   rec->lnk.sgw_gtpu_teid = *((uint64_t*)value); break;
        case T_ACTIVE:          rec->lnk.stat.active = *((uint64_t*)value); break;
        case T_QOS:             rec->lnk.stat.mode = *((uint64_t*)value); break;
        }
    } else if (type == mx::Notify::TYPE_TEXT) {
        uint8_t ipv[16] = {0}; unsigned int ipv_len(16);

        switch(clmnid) {
        case T_SGW_GTPU_IPV:
            if (get_ipv4((const char*)value, ipv, &ipv_len) == 0) { memcpy(&rec->lnk.sgw_gtpu_ipv4, ipv, 4); }
            break;
        case T_UEIPV4:
            if (get_ipv4((const char*)value, ipv, &ipv_len) == 0) { memcpy(&rec->lnk.ue_ipv4, ipv, 4); }
            break;
        case T_PGW_GTPU_IPV:
            if (get_ipv4((const char*)value, ipv, &ipv_len) == 0) { memcpy(&rec->lnk.pgw_gtpu_ipv4, ipv, 4); }
            break;
        }
    }
    return(0);
}

/**
   execute on defferent virtual core except main core.\n
   *******************************************************************************
   build link information from binlog event, \n
   save it into shared area of main thread context.\n
   *******************************************************************************
   @param[in]     arg  application instance address
   @param[in]     cnt  number of cycle
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CorePgwBaseWorker::VirtualCycle(void* arg,uint64_t* cnt){
    int err;
    std::pair<unsigned char *, size_t> buffer_buflen;
    const char *msg= NULL;
    static uint64_t counter = 0;
    mx::Event* event = (mx::Event*)binlog_;
    MIXIBINLOG_CLIENT::NotifyImpl notify;
    MIXIBINLOG_CLIENT::record_t record;

    bzero(&record, sizeof(record));
    if (event->Read(&notify, &record) != 0) {
        return(0);
    }
#if ARCH>=15
    // policer values
    record.lnk.commit_information_rate = policer_cir_;
    record.lnk.commit_burst_size = policer_cbs_;
    record.lnk.excess_information_rate = policer_eir_;
    record.lnk.excess_burst_size = policer_ebs_;
#endif

    if (!record.lnk.ue_ipv4 ||
        !record.lnk.ue_teid ||
        !record.lnk.sgw_gtpu_ipv4 ||
        !record.lnk.sgw_gtpu_teid ||
        !record.lnk.pgw_gtpu_ipv4)
    {
        PGW_LOG(
            RTE_LOG_ERR,
            "invalid binlog: %08x,%08x,%08x,%08x,%08x\n",
            record.lnk.ue_ipv4,
            record.lnk.ue_teid,
            record.lnk.sgw_gtpu_ipv4,
            record.lnk.sgw_gtpu_teid,
            record.lnk.pgw_gtpu_ipv4
        );
        return(0);
    }
    printf("(ue_teid: %08x/sgw_gtpu_teid: %08x/stat.active: %02x/sgw_gtpu_ipve: %08x/pgw_gtpu_ipv4: %08x/ stat.mode: %02x)\n",
        record.lnk.ue_teid,
        record.lnk.sgw_gtpu_teid,
        record.lnk.stat.active,
        record.lnk.sgw_gtpu_ipv4,
        record.lnk.pgw_gtpu_ipv4,
        record.lnk.stat.mode
    );

    if (findtype_){
        auto node_txt = (GetType()==TYPE::PGW_EGRESS?"egress":"ingress");
        auto nkey  = (GetType()==TYPE::PGW_EGRESS?record.lnk.ue_ipv4:record.lnk.ue_teid);
        // 500K - 1M records
        // 20 Byte x 1M = 20MB(+meta information 16MB?)=36MB
        LOCK();
        if (record.evt == 30) {
            PGW_LOG(RTE_LOG_INFO,"<%s>key:%08x\n\tue_teid:%08x\n\tue_ipv4:%08x\n\tsgw_teid:%08x\n\tsgw_ipv4:%08x\n",
                    node_txt,
                    htonl(nkey),
                    record.lnk.ue_teid,
                    record.lnk.ue_ipv4,
                    record.lnk.sgw_gtpu_teid,
                    record.lnk.sgw_gtpu_ipv4);
            (*MAP(map_))[htonl(nkey)] = record.lnk;
        }else if (record.evt == 31) {
            if (record.lnk.stat.active == 0){
                PGW_LOG(RTE_LOG_INFO,"key:%08x\n\tremove\n", htonl(nkey));
                MAP(map_)->erase(htonl(nkey));
            }else{
                PGW_LOG(RTE_LOG_INFO,"<%s>update/key:%08x\n\tue_teid:%08x\n\tue_ipv4:%08x\n\tsgw_teid:%08x\n\tsgw_ipv4:%08x\n",
                        node_txt,
                        htonl(nkey),
                        record.lnk.ue_teid,
                        record.lnk.ue_ipv4,
                        record.lnk.sgw_gtpu_teid,
                        record.lnk.sgw_gtpu_ipv4);
                (*MAP(map_))[htonl(nkey)] = record.lnk;
            }
        }else if (record.evt == 32) {
            MAP(map_)->erase(htonl(nkey));
            PGW_LOG(RTE_LOG_INFO,"key:%08x\n\tremove\n", htonl(nkey));
        }
        UNLOCK();
    }else{
        // target group on this worker instance.
        // PGW id , Group id
        /*
         *  +--------+--------+--------+--------+--------+--------+--------+--------+
         *  |    fix          | pgwid  |groupid |  value =(teid table index)        |
         *  +--------+--------+--------+--------+--------+--------+--------+--------+
         *  |   0    |   A    |    B   |   5    |   0    |   1    |    2   |    3   |
         *  +--------+--------+--------+--------+--------+--------+--------+--------+
         *
         */
        if (PGWID(record.lnk.ue_teid) == pgwid_ && GROUPID(record.lnk.ue_teid) == groupid_){
            if (record.evt == 30) {
                LOOKUP(lookup_)->Add(GROUPVAL(record.lnk.ue_teid), &record.lnk, 0);
            }else if (record.evt == 31) {
                if (record.lnk.stat.active == 0){
                    LOOKUP(lookup_)->Del(GROUPVAL(record.lnk.ue_teid), 0);
                }else{
                    LOOKUP(lookup_)->Add(GROUPVAL(record.lnk.ue_teid), &record.lnk, 0);
                }
            }else if (record.evt == 32) {
                LOOKUP(lookup_)->Del(GROUPVAL(record.lnk.ue_teid), 0);
            }
        }
    }


    return(0);
}
