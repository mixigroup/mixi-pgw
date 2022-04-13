/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       app_v2.cc
    @brief      application implementation(version 2)
*******************************************************************************
   \n
*******************************************************************************
    @date       created(26/apr/2018)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 26/apr/2018 
      -# Initial Version
******************************************************************************/
#include "mixi_pgw_data_plane_def.hpp"
#include "app_v2.hpp"
#include "static_conf.hpp"
#include "core.hpp"

#include <mysql.h>
#include <signal.h>

using namespace MIXIPGW;
static pthread_mutex_t __mysql_local_lock;
static rte_spinlock_t s_spinlock = RTE_SPINLOCK_INITIALIZER;
static rte_atomic64_t s_status_ = RTE_ATOMIC64_INIT(APP_STAT_DC);
static rte_atomic64_t s_core_used_ = RTE_ATOMIC64_INIT(0);
static rte_atomic64_t s_core_prepared_ = RTE_ATOMIC64_INIT(0);
static rte_atomic64_t s_core_ack_ = RTE_ATOMIC64_INIT(0);
/** *****************************************************
* @brief
* memory pooling interface \n
* version 2 \n
*/
class MemPoolImplV2 : public CoreMempool{
public:
    MemPoolImplV2(struct rte_mempool* pool):pool_(pool){ }
    virtual struct rte_mempool* Ref(COREID coreid){
        return(pool_);
    }
private:
    struct rte_mempool* pool_;
private:
    MemPoolImplV2(){}
};


/**
   application  : version2  :  constructor \n
   *******************************************************************************
   construct\n
   *******************************************************************************
 */
MIXIPGW::AppV2::AppV2():App(){
    rte_atomic64_set(&s_status_, APP_STAT_DC);
}
/**
   application  : version2  :  constructor \n
   *******************************************************************************
   construct\n
   *******************************************************************************
 */
MIXIPGW::AppV2::AppV2(int argc, char** argv):App(){
    if (rte_eal_init(argc, argv) < 0){
        rte_exit(EXIT_FAILURE, "Invalid EAL parameters\n");
    }
    pthread_mutex_init(&__mysql_local_lock, 0);

#if ARCH==15
    #include "../arch/pgw_a05532_1gx_tunnel.cc"
#elif ARCH==16
    #include "../arch/pgw_a05832_tunnel_00_01.cc"
#elif ARCH==17
    #include "../arch/pgw_a05832_tunnel_02_03.cc"
#elif ARCH==18
    #include "../arch/pgw_a05832_tunnel_04_05.cc"
#elif ARCH==19
    #include "../arch/pgw_a05832_tunnel_06_07.cc"
#elif ARCH==20
    #include "../arch/pgw_a05832_tunnel_08_09.cc"
#elif ARCH==21
    #include "../arch/pgw_a05832_tunnel_10_11.cc"
#else
    #error not implemented architecture.(application version 2)
#endif
}

/**
   application  : version 2 : destructor\n
   *******************************************************************************
   destruct\n
   *******************************************************************************
 */
MIXIPGW::AppV2::~AppV2(){
}

/**
   application  : add version2 Core with group \n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]     coreid      socket
   @param[in]     cores       Core group 
   @return RETCD  0==success,0!=error
 */

void MIXIPGW::AppV2::AddCores(COREID coreid, VCores& cores){
    vcores_map_[coreid] = cores;
    // supported, version 1 interface
    // set socket id to core group
    for(auto itv = cores.begin();itv != cores.end();++itv){
        auto lcore = (*itv)->GetN(KEY::LCORE);
        cores_[lcore] = (*itv);
        (*itv)->SetN(KEY::SOCKET, rte_lcore_to_socket_id(coreid));
        //
        if ((*itv)->GetType() == TYPE::WORKER_TRANSFER ||
            (*itv)->GetType() == TYPE::PGW_EGRESS ||
            (*itv)->GetType() == TYPE::PGW_INGRESS){
            auto vlcore = (*itv)->GetN(KEY::OPT);
            vcores_[vlcore] =  (*itv);
        }
        auto sock = rte_lcore_to_socket_id(coreid);
        // when core is placed in new Numa Node, 
        // reserve a packet regions that allocated for that.
        if (mpools_.find(sock) == mpools_.end()){
            char nm[64] = {0};
            snprintf(nm, sizeof(nm)-1, "pktmbuf_pool_%u", sock);
            PGW_LOG(RTE_LOG_INFO, "mempool(%s/%u/%u/%u/%u)\n", nm, DEFAULT_MEMPOOL_BUFFERS, DEFAULT_MEMPOOL_CACHE_SIZE, DEFAULT_MBUF_DATA_SIZE, sock);

            // new node.
            auto proc_type = rte_eal_process_type();
            auto pool = (proc_type==RTE_PROC_SECONDARY)?
                     rte_mempool_lookup(nm):
                     rte_pktmbuf_pool_create(nm, DEFAULT_MEMPOOL_BUFFERS, DEFAULT_MEMPOOL_CACHE_SIZE, 0, DEFAULT_MBUF_DATA_SIZE, sock);
	    if (!pool) {
                rte_panic("Cannot create mbuf pool on socket %u\n", sock);
            }
            mpools_[sock] = new MemPoolImplV2(pool);
        }
        (*itv)->SetP(KEY::OPT, mpools_[sock]);
    }
}

/**
    software ring connects cores\n
   *******************************************************************************
    multiple CoreInterface Objects
    can be placed in 1 Core(version 2)\n
    \n
   *******************************************************************************
   @param[in]     core_fm     source core id
   @param[in]     core_to     destination core id
   @param[in]     size        ring size
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::AppV2::Connect(COREID core_fm, COREID core_to, SIZE size){
    return(App::Connect(core_fm, core_to, size));
}

/**
   application  : version2 main loop\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::AppV2::Run(void){
    unsigned lcore;

    signal(SIGUSR1, MIXIPGW::App::Signal);
    signal(SIGUSR2, MIXIPGW::App::Signal);
    signal(SIGINT,  MIXIPGW::App::Signal);
    //
    rte_eal_mp_remote_launch(MIXIPGW::AppV2::DispatchV2, this, CALL_MASTER);
    RTE_LCORE_FOREACH_SLAVE(lcore) {
        PGW_LOG(RTE_LOG_INFO, "rte_eal_mp_remote_launch(%u)..\n", lcore);
        if (rte_eal_wait_lcore(lcore) < 0) {
            return(-1);
        }
    }
    return(0);
}
/**
   dispatch thread(version 2)\n
   *******************************************************************************
   thread function invoked from dpdk\n
   sequential execution per Core groups\n
   APP , design pattern consumes more CPU cores, but better performance is expected.\n
   APPV2, design pattern consumes CPU cores at NICs assigned.\n
            PGW can placed in enclosure for number of CPUs.\n
            Cost-effective but no-good latency compared to APP design pattern.\n
   *******************************************************************************
   @param[in]     arg    thread parameter
   @return int    0==success,0!=error
 */
int MIXIPGW::AppV2::DispatchV2(void* arg){
    auto appv2  = (AppV2*)arg;
    auto lcore  = rte_lcore_id();
    auto cores  = appv2->vcores_map_.find(lcore);
    uint64_t    cycle_count[32] = {0};

    bzero(cycle_count, sizeof(cycle_count));

    if (lcore == 0){
        PGW_LOG(RTE_LOG_INFO, "run state changer.\n");
        //
        while(!App::Halt()){
            usleep(APP_STAT_DELAY<<12); 
            if (APP_STAT_STAT() == APP_STAT_DC){
                PGW_LOG(RTE_LOG_INFO, ">> status >> dont care.--> init.\n");
                APP_STAT_SET(APP_STAT_INIT);
            }else if (APP_STAT_STAT() == APP_STAT_INIT){
                if (APP_STAT_ISFIN_PREPARE() == 0){
                    PGW_LOG(RTE_LOG_INFO, ">> status >> init --> prepared..\n");
                    APP_STAT_SET(APP_STAT_PREPARE);
                }
            }else if (APP_STAT_STAT() == APP_STAT_PREPARE){
                PGW_LOG(RTE_LOG_INFO, ">> status >> prepare --> busyloop..\n");
                APP_STAT_RESET_ACK(); 
                APP_STAT_SET(APP_STAT_BUSYLOOP);
            }else if (APP_STAT_STAT() == APP_STAT_BUSYLOOP){
                if (APP_STAT_HAVE_SIGNAL()){
                    PGW_LOG(RTE_LOG_INFO, ">> status >> busyloop --> config change..\n");
                }
            }else{
                auto s = APP_STAT_STAT();
                rte_panic("unknown status.(%08x: %08x)\n", (unsigned)((s>>32)&0xFFFFFFFF),(unsigned)(s&0xFFFFFFFF));
            }
        } 
        return(0);
    }
    //
    if (cores == appv2->vcores_map_.end()){
        // vcore
        auto core = appv2->Find(lcore, true);
        if (core){
            while(!App::Halt()){
                auto stat = APP_STAT_STAT();
                if (likely(stat == APP_STAT_BUSYLOOP)){
                    core->VirtualCycle(arg, &cycle_count[0]);
                    cycle_count[0]++;
                }
                usleep(APP_STAT_DELAY);  
            }
        }
    }else{
        PGW_LOG(RTE_LOG_INFO, "entory loop(%u)\n", lcore);
    	// start event
        pthread_mutex_lock(&__mysql_local_lock);
        mysql_thread_init();
        auto core = (cores->second).begin();
        for(;core != (cores->second).end();++core){
            (*core)->BeforeCycle(arg);
        }
        pthread_mutex_unlock(&__mysql_local_lock);
        APP_STAT_SET_PREPARE(lcore);
        //
        auto core_cnt = (cores->second).size();
        while(!App::Halt()){
            auto stat = APP_STAT_STAT();
            if (likely(stat == APP_STAT_BUSYLOOP)){
                for(auto lcount = 0;lcount < APP_STAT_BURST;lcount++){
                    auto core = (cores->second).begin();
                    auto idx = 0;
                    for(;core != (cores->second).end();++core,idx++) {
                        (*core)->Cycle(arg, &cycle_count[idx]);
                        cycle_count[idx] += ((uint64_t)core_cnt)<<4;
                    }
                }
            }else{
                usleep(APP_STAT_DELAY);
            }
        }
        // stop event
        core =  (cores->second).begin();
        for(;core != (cores->second).end();++core){
            (*core)->AfterCycle(arg);
        }
    }
    return(0);
}

RETCD MIXIPGW::AppV2::Commit(StaticConf* cnf){
    for(auto it = vcores_map_.begin();it != vcores_map_.end();++it){
        APP_STAT_SET_USE((it->first));
    }
    return(App::Commit(cnf));
}
