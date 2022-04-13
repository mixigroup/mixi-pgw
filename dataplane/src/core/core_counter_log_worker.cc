/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       core_counter_log_worker.cc
    @brief      counter logging worker core implemented
*******************************************************************************
	logging : \n
    receive traffic aggregate values from counter worker every 1 second.\n
    buffer records and bulk save those every 128 records\n
*******************************************************************************
    @date       created(13/feb/2018)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 13/feb/2018 
      -# Initial Version
******************************************************************************/
#ifdef GCC_VERSION
#undef GCC_VERSION
#endif
#include "../../inc/core.hpp"
#include "mixi_pgw_data_plane_def.hpp"
#include <mysql.h>

using namespace MIXIPGW;


/**
   counter log worker core interface  :  constructor \n
   *******************************************************************************
   counter logging.\n
   *******************************************************************************
   @param[in]     lcore         core id
   @param[in]     dburi         databaseuri
   @param[in]     cpuid         host stack DB thread cpu - core
 */
MIXIPGW::CoreCounterLogWorker::CoreCounterLogWorker(COREID lcore, TXT dburi, COREID cpuid)
        :CoreInterface(0, 0, lcore),cpuid_(cpuid){
    PGW_LOG(RTE_LOG_INFO, "CoreCounterLogWorker::CoreCounterLogWorker(%u/%s/%u)\n", lcore, dburi,cpuid_);
    //
    dburi_ = dburi;
    transfer_count_ = 0;
}
/**
   counter logging worker core interface  : destructor\n
   *******************************************************************************
   destructor\n
   *******************************************************************************

 */
MIXIPGW::CoreCounterLogWorker::~CoreCounterLogWorker(){
}
/**
   get core type.\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return TYPE  return core type[worker]
 */
TYPE MIXIPGW::CoreCounterLogWorker::GetType(void){
    return(TYPE::WORKER_TRANSFER);
}
/**
   object name\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return TXT  object name 
 */
TXT MIXIPGW::CoreCounterLogWorker::GetObjectName(void){
    return("CoreCounterLogWorker");
}
/**
  obtain value find by key.\n
 *******************************************************************************
 override, call CoreInterface::GetN ,except KEY::OPT\n
 *******************************************************************************
 @param[in]     key   key
 @return VALUE  return value :0xffffffff=error
 */
VALUE MIXIPGW::CoreCounterLogWorker::GetN(KEY t){
    if (t == KEY::OPT){
        return(cpuid_);
    }
    return(CoreInterface::GetN(t));
}

/**
   Counter logging , connect woker core  to specific ring \n
   *******************************************************************************
   counter logging in worker core  , [TO]direction=internal specific software ring\n
   \n
   *******************************************************************************
   @param[in]     order       connection direction 
   @param[in]     ring        ring address 
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CoreCounterLogWorker::SetRingAddr(ORDER order, void* ring){
    if (order == ORDER::FROM){              // from = counter -> counter log between Indicates ring 
        ring_fm_ = (struct rte_ring*)ring;
    }else if (order == ORDER::TO){          // to   = internal
        ring_to_ = (struct rte_ring*)ring;
    }else{
        rte_exit(EXIT_FAILURE, "unknown ring order.(%u)\n", order);
    }
    return(0);
}
#define DBH(h)  ((MYSQL*)h)
RETCD MIXIPGW::CoreCounterLogWorker::BeforeCycle(void* arg){
    bool reconnect = 1;
    PGW_LOG(RTE_LOG_INFO, "CoreCounterLogWorker::BeforeCycle\n");

    char user[32] = {0};
    char pswd[32] = {0};
    char host[32] = {0};
    unsigned int port = 0;
    //
    parse_mysql(dburi_.c_str(), user, sizeof(user)-1, pswd, sizeof(pswd)-1, host, sizeof(host)-1,&port);
    if (!user[0] || !pswd[0] || !host[0] || !port){
        rte_panic("invalid dburi :// (%s)\n", dburi_.c_str());
    }
    // first, SHOW MASTER STATUS (binlog file , position from master database).
    dbhandle_ = mysql_init(NULL);
    if (!dbhandle_){
        rte_panic("mysql_init (%d: %s)\n", errno, strerror(errno));
    }
    //
    mysql_options(DBH(dbhandle_), MYSQL_OPT_RECONNECT, &reconnect);
    if (!mysql_real_connect(DBH(dbhandle_),
                            host,
                            user,
                            pswd,
                            "mixipgw",
                            port, NULL, 0)) {
        rte_panic("mysql_real_connect (%s)\n", mysql_error(DBH(dbhandle_)));
    }
    mysql_set_character_set(DBH(dbhandle_), "utf8");

    return(0);
}
RETCD MIXIPGW::CoreCounterLogWorker::AfterCycle(void* arg){
    mysql_close(DBH(dbhandle_));
    return(0);
}

/**
   counter logging worker core : virtual cycle : 1 cycle\n
   *******************************************************************************
   1 batch processing, bulk + set dequeued packets to host stack buffer\n
   release mbuf after send completed\n
   *******************************************************************************
   @param[in]     arg  application instance address
   @param[in]     cnt  number of cycle
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CoreCounterLogWorker::Cycle(void* arg, uint64_t* cnt){
    int n = 0;

    // debug output
    if (cnt && (*cnt) && ((*cnt) == (uint64_t)-1 || (*cnt)%(512*1024*1024)==0)){
        PGW_LOG(RTE_LOG_INFO, "performance(%p):core counter log total : %lu\n", (void*)pthread_self(), transfer_count_);
        transfer_count_ = 0;
        BurstFlush(arg);
        return(0);
    }
    /* input buffer (used main cycle processing) = input_ */
    auto nburst = rte_ring_sc_dequeue_burst(ring_fm_, (void**)input_, burst_fm_, NULL);
    if (unlikely(nburst == -ENOENT)) {
        return(0);
    }
    if (unlikely(nburst == 0)){
        return(0);
    }
    // prefetch first.
    for (n  = 0; n < PREFETCH_OFFSET && n < nburst; n++) {
        rte_prefetch0(rte_pktmbuf_mtod(input_[n], void *));
    }
    // enqueue to host stack thread while prefetching.
    for (n = 0; n < (nburst - PREFETCH_OFFSET) && nburst > PREFETCH_OFFSET; n++) {
        rte_prefetch0(rte_pktmbuf_mtod(input_[n + PREFETCH_OFFSET], void *));
        SendOrBuffer(arg, input_[n]);
    }
    // remained.
    for (; n < nburst; n++) {
        SendOrBuffer(arg, input_[n]);
    }
    return(0);
}
/**
   execute on defferent virtual core except main core.\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]     arg  application instance address
   @param[in]     cnt  number of cycle
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CoreCounterLogWorker::VirtualCycle(void* arg,uint64_t* cnt){
    // dequeue from internal-connected ring
    /* input buffer (used internal ring) = input_int_ */
    auto nburst = rte_ring_sc_dequeue_burst(ring_to_, (void**)input_int_, burst_to_, NULL);

    // database operation is bottleneck,
    // so no compile hints are needed.
    if (nburst == -ENOENT || nburst == 0) {
        return(0);
    }

    unsigned counter = 0;
    std::string sql = "INSERT INTO traffic_log(`teid`,`used_s5`,`used_sgi`,`type`)VALUES";

    PGW_LOG(RTE_LOG_INFO, ">> to store database(%u:%p)\n", nburst, (void*)pthread_self());
    for (auto n = 0; n < nburst; n++) {
        auto payload = rte_pktmbuf_mtod(input_int_[n], void*);
        auto payload_len = rte_pktmbuf_pkt_len(input_int_[n]);
        //
        if (payload_len != sizeof(counting_t)){
            return(-1);
        }
        auto c = (counting_ptr)payload;

        // prepare Bulk Insert SQL
        for(auto m = 0;m < c->count;m++){
            char bf[128] = {0};
            uint32_t used_s5 = 0;
            uint32_t used_sgi= 0;
            // lte
	    if (c->type == COUNTER_EGRESS){
                used_sgi = c->items[m].used;
            }else{
                used_s5 = c->items[m].used;
            }
            if (used_s5 || used_sgi){
                snprintf(bf, sizeof(bf) - 1,"%s(%u,%u,%u,0)",
                         counter==0?"":",",
                         (unsigned)c->items[m].key,
                         (unsigned)used_s5,
                         (unsigned)used_sgi);
                sql += bf;
                counter++;
                transfer_count_ ++;
            }
            // 3g
            used_sgi = used_s5 = 0;
            if (c->type == COUNTER_EGRESS){
                used_sgi = c->items[m].used_3g;
            }else{
                used_s5 = c->items[m].used_3g;
            }
            if (used_s5 || used_sgi){
                snprintf(bf, sizeof(bf) - 1,"%s(%u,%u,%u,1)",
                         counter==0?"":",",
                         (unsigned)c->items[m].key,
                         (unsigned)used_s5,
                         (unsigned)used_sgi);
                sql += bf;
                counter++;
                transfer_count_ ++;
            }
            // low
            used_sgi = used_s5 = 0;
            if (c->type == COUNTER_EGRESS){
                used_sgi = c->items[m].used_low;
            }else{
                used_s5 = c->items[m].used_low;
            }
            if (used_s5 || used_sgi){
                snprintf(bf, sizeof(bf) - 1,"%s(%u,%u,%u,2)",
                         counter==0?"":",",
                         (unsigned)c->items[m].key,
                         (unsigned)used_s5,
                         (unsigned)used_sgi);
                sql += bf;
                counter++;
                transfer_count_ ++;
            }
	}
        rte_pktmbuf_free(input_int_[n]);
    }
    if (mysql_query(DBH(dbhandle_),sql.c_str()) != 0){
        PGW_LOG(RTE_LOG_ERR, "failed. query(%s/%s)\n", mysql_error(DBH(dbhandle_)), sql.c_str());
        return(-1);
    }
    return(0);
}
