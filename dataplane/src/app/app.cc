/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       app.cc
    @brief      application  : implement
*******************************************************************************
   application keeping multiple cores under control\n
   thinly wrapped dpdk, no logic included.\n
   dpdk initialization, NIC binding, environment setup\n
   ,software gin connection, thread dispathing, etc\n 
*******************************************************************************
    @date       created(27/sep/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 27/sep/2017 
      -# Initial Version
******************************************************************************/
#include "mixi_pgw_data_plane_def.hpp"
#include "app.hpp"
#include "static_conf.hpp"
#include "core.hpp"

#include <signal.h>

#if ARCH==1
#elif ARCH==9
#elif ARCH==99
#elif ARCH==15 || ARCH==16 || ARCH==17 || ARCH==18 || ARCH==19 || ARCH==20 || ARCH==21
#include "app_v2.hpp"
#endif

#if defined(COUNTER_RX) && defined(TUNNEL_RX)
#error cannot define both COUNTER_RX and TUNNEL_RX
#endif

using namespace MIXIPGW;
int MIXIPGW::PGW_LOG_LEVEL[PGW_LOG_CORES] = {RTE_LOG_DEBUG+1};

static int         s_PGW_LOG_LEVEL = RTE_LOG_DEBUG+1;  /*!< log level */
static StaticConf  s_CONF_TMP_;                      /*!< config reagion, for temporary */
static uint32_t    s_FLUSH_COUNTER = _FLUSH_WORKER_; /*!< counter value, for temporary */
static uint32_t    s_FLUSH_ENCAP   = _FLUSH_TX_;     /*!< .. : encap */
static uint32_t    s_FLUSH_GRETERM = _FLUSH_TX_;     /*!< .. : greterm */
static uint32_t    s_FLUSH_PGW_IE  = _FLUSH_TX_;     /*!< .. : pgw ie */
static uint32_t    s_FLUSH_RX      = _FLUSH_RX_;     /*!< .. : rx */
static uint32_t    s_FLUSH_RX_DIST = _FLUSH_RX_;     /*!< .. : rx distribute */
static uint32_t    s_FLUSH_TX      = _FLUSH_TX_;     /*!< .. : tx */
static uint32_t    s_POLICER_CIR   = POLICER_CIR_DEFAULT;    /*!< policer : CIR */
static uint32_t    s_POLICER_CBS   = POLICER_CBS_DEFAULT;    /*!< policer : CBS */
static uint32_t    s_POLICER_EIR   = POLICER_EIR_DEFAULT;    /*!< policer : EIR */
static uint32_t    s_POLICER_EBS   = POLICER_EBS_DEFAULT;    /*!< policer : EBS */

/*
 * state machine.
 * +--------------++--------+--------+--------+--------+--------+--------+--------+
 * |     \  stat  || DC     |  INIT  |  PRPR  |  LOOP  | CFGLD  | CFGSWP |CFGACK  |
 * |event \       ||        |        |        |        |        |        |        |
 * +==============++========+========+========+========+========+========+========+
 * |[boot]        ||        |        |        |        |        |        |        |
 * |              ||>INIT   |        |        |        |        |        |        |
 * +--------------++--------+--------+--------+--------+--------+--------+--------+
 * |[init         ||        |        |        |        |        |        |        |
 * |   complete]  ||        |>PRPR   |        |        |        |        |        |
 * | load database||        |        |        |        |        |        |        |
 * +--------------++--------+--------+--------+--------+--------+--------+--------+
 * |[delay.]      ||        |        |[delay] |        |        |        |        |
 * |   xx us      ||        |        |>LOOP   |        |        |>PRPR   |        |
 * +--------------++--------+--------+--------+--------+--------+--------+--------+
 * |busy loop     ||        |        |        | >>  << |        |        |        |
 * +--------------++--------+--------+--------+--------+--------+--------+--------+
 * |[signal]      ||        |        |        |        |        |        |        |
 * |              ||        |        |        |>CFGLD  |        |        |        |
 * +--------------++--------+--------+--------+--------+--------+--------+--------+
 * |[cfg load     ||        |        |        |        |        |        |        |
 * |     finish]  ||        |        |        |        |>CFGACK |        |        |
 * +--------------++--------+--------+--------+--------+--------+--------+--------+
 * |[can cfg swap]||        |        |        |        |        |        |        |
 * |              ||        |        |        |        |        |        |>CFGSWP |
 * +--------------++--------+--------+--------+--------+--------+--------+--------+
 *
 */

static rte_spinlock_t s_spinlock = RTE_SPINLOCK_INITIALIZER;
static rte_atomic64_t s_status_ = RTE_ATOMIC64_INIT(APP_STAT_DC);
static rte_atomic64_t s_core_used_ = RTE_ATOMIC64_INIT(0);
static rte_atomic64_t s_core_prepared_ = RTE_ATOMIC64_INIT(0);
static rte_atomic64_t s_core_ack_ = RTE_ATOMIC64_INIT(0);
static int s_quit_ = 0;


/** *****************************************************
* @brief
* memory pooling interface \n
* \n
*/
class MemPoolImpl : public CoreMempool{
public:
    /**
       constructor \n
      *******************************************************************************
      construct\n
      *******************************************************************************
    */
    MemPoolImpl(struct rte_mempool* pool):pool_(pool){ }
    /**
      interface \n
      *******************************************************************************
      Core : memory pool matching socket\n
      *******************************************************************************
      @param[in]     coreid count of arguments.
      @return struct rte_mempool* memory pool address  , NULL==error
    */
    virtual struct rte_mempool* Ref(COREID coreid){
        return(pool_);
    }
private:
    struct rte_mempool* pool_;
private:
    MemPoolImpl(){}
};

/**
   application  : usage.\n
   *******************************************************************************
   display usage\n
   *******************************************************************************
 */
static void usage(const char *prgname) {
    printf("%s [EAL options] -- -l <log level>\n", prgname);
}
struct rte_eth_conf MIXIPGW::App::s_port_conf_;

/**
   application  :  constructor \n
   *******************************************************************************
   construct\n
   *******************************************************************************
 */
MIXIPGW::App::App():conf_(new StaticConf()),stat_(0),cnt_rx_(0),cnt_wk_(0),cnt_tx_(0),
                  cnt_pgw_ingress_(0),cnt_pgw_egress_(0){
    bzero(&s_port_conf_, sizeof(s_port_conf_));
    rte_atomic64_set(&s_status_, APP_STAT_DC);
#if  ARCH==16 || ARCH==17 || ARCH==18 || ARCH==19 || ARCH==20 || ARCH==21
    s_port_conf_.rxmode.mq_mode             = ETH_MQ_RX_NONE;
#else
    s_port_conf_.rxmode.mq_mode             = ETH_MQ_RX_RSS;
#endif
    s_port_conf_.rxmode.split_hdr_size      = 0;
    s_port_conf_.rxmode.header_split        = 0;
    s_port_conf_.rxmode.hw_ip_checksum      = 1;
    s_port_conf_.rxmode.hw_vlan_filter      = 0;
    s_port_conf_.rxmode.jumbo_frame         = 0;
    s_port_conf_.rxmode.hw_strip_crc        = 1;
    s_port_conf_.rx_adv_conf.rss_conf.rss_key= NULL;
    s_port_conf_.rx_adv_conf.rss_conf.rss_hf= ETH_RSS_IP | ETH_RSS_TCP | ETH_RSS_UDP;
#if ARCH==8
    s_port_conf_.txmode.mq_mode             = ETH_MQ_TX_VMDQ_ONLY;
#else
    s_port_conf_.txmode.mq_mode             = ETH_MQ_TX_NONE;
#endif
#if ARCH==13 || ARCH==15 || ARCH==16 || ARCH==17 || ARCH==18 || ARCH==19 || ARCH==20 || ARCH==21
    s_port_conf_.rxmode.hw_vlan_filter      = 1;
    s_port_conf_.rxmode.hw_vlan_strip       = 1;
#endif
}
/**
   application  : destructor\n
   *******************************************************************************
   destruct\n
   *******************************************************************************
 */
MIXIPGW::App::~App(){
    if (conf_){ delete conf_; }
    conf_ = NULL;
}
/**
   application initialize\n
   *******************************************************************************
   once in process\n
   *******************************************************************************
   @param[in]     argc   count of arguments.
   @param[in]     argv   arguments
   @return App*   application instance pointer
 */
MIXIPGW::App* MIXIPGW::App::Init(int argc, char **argv){
    static struct option lgopts[] = { {NULL, 0, 0, 0} };
    char **argvopt;
    int opt,option_index,skip_optid = -1;

    for(auto n = 0;n < argc;n++){
        if (strncmp("--", argv[n], 2) == 0){
            skip_optid = n;
        }
    }
    // initialize eal
    if (rte_eal_init(argc, argv) < 0){
        rte_exit(EXIT_FAILURE, "Invalid EAL parameters\n");
    }
    PGW_LOG(RTE_LOG_INFO, "%s", "args ---- \n");
    for(auto n = 0;n < argc;n++){
        PGW_LOG(RTE_LOG_INFO, "%d:%s\n", n, argv[n]);
    }
    // startup parameter on local
    if (skip_optid != -1){
        argvopt = &(argv[skip_optid]);
        while ((opt = getopt_long((argc - skip_optid), argvopt, "l:", lgopts, &option_index)) != EOF) {
            switch (opt) {
                case 'l':   // log-level
                    if (optarg){
                        PGW_LOG(RTE_LOG_INFO, "set loglevel %s\n", optarg);
                        for(auto n = 0;n < PGW_LOG_CORES;n++){
                            MIXIPGW::PGW_LOG_LEVEL[n] = atoi(optarg);
                        }
                    }
                    break;
                case 0:
                default:
                    usage(argv[0]);
                    rte_exit(EXIT_FAILURE, "Invalid parameters\n");
                    break;
            }
        }
    }

    auto app = new App();
    if (!app){
        rte_exit(EXIT_FAILURE, "Invalid new class.\n");
    }
    // tunnel_def.hpp
    // prepare each cores, ring inter-core connections,
    // and state machine regions from dynamic parameters.\n

    /*
     * =======================================
     * [TODO] following define tables should be generated by scripts.
     * excluding non-essential implementations that is only for configuration.
     * =======================================
     * */
#ifdef ARCH
    #if ARCH==0
        #include "../arch/encap_vbox_4.cc"
    #elif ARCH==1
        #include "../arch/encap_a05533.cc"
    #elif ARCH==2
        #include "../arch/count_a05533.cc"
    #elif ARCH==3
        #include "../arch/pgw_ingress_a05533.cc"
    #elif ARCH==4
        #include "../arch/pgw_egress_a05533.cc"
    #elif ARCH==6
        #include "../arch/pgw_ingress_distributor_a05536_1port.cc"
    #elif ARCH==7
        #include "../arch/pgw_a05404_1port.cc"
    #elif ARCH==8
        #include "../arch/pgw_a05404_2port.cc"
    #elif ARCH==9
        #include "../arch/tunnel_a05406.cc"
    #elif ARCH==10
        #include "../arch/pgw_a05404_2port_x_warmup.cc"
    #elif ARCH==12
        #include "../arch/count_a05534_direct_db.cc"
    #elif ARCH==13
        #include "../arch/count_a05411_direct_db.cc"
    #elif ARCH==14
        #include "../arch/count_a05411_multi_worker.cc"
    #elif ARCH==15 || ARCH==16 || ARCH==17 || ARCH==18 || ARCH==19 || ARCH==20 || ARCH==21
    #elif ARCH==99
        #warning no architecture.
    #else
        #error not implemented architecture.
    #endif
#else
#error not found architecture.
#endif
    return(app);
}
/**
   application main loop\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]     argc   count of arguments.
   @param[in]     argv   arguments
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::App::Run(void){
    unsigned lcore;

    signal(SIGUSR1, MIXIPGW::App::Signal);
    signal(SIGUSR2, MIXIPGW::App::Signal);
    signal(SIGINT,  MIXIPGW::App::Signal);
    //
    rte_eal_mp_remote_launch(MIXIPGW::App::Dispatch, this, CALL_MASTER);
    RTE_LCORE_FOREACH_SLAVE(lcore) {
        if (rte_eal_wait_lcore(lcore) < 0) {
            return(-1);
        }
    }
    return(0);
}
/**
   dispatch thread\n
   *******************************************************************************
   thread function invoked from dpdk\n
   *******************************************************************************
   @param[in]     arg    thread parameter
   @return int    0==success,0!=error
 */
int MIXIPGW::App::Dispatch(void* arg){
    auto app    = (App*)arg;
    auto lcore  = rte_lcore_id();
    auto core   = app->Find(lcore, false);
    uint64_t cycle_count = 0,burst_count = 0;
    //
    if (core){
        printf("App::Dispatch[%p] (%u)\n",(void*)pthread_self(), lcore);
        //
        core->BeforeCycle(arg);
        // CPU bit-on with inialize processing completed.
        APP_STAT_SET_PREPARE(lcore);
        //
        while(!s_quit_){
           auto stat = APP_STAT_STAT();
           if (likely(stat == APP_STAT_BUSYLOOP || stat == APP_STAT_CFGLOAD)){
               for(auto lcount = 0;lcount< APP_STAT_BURST;lcount++){
                   core->Cycle(arg, &cycle_count);
                   cycle_count++;
               }
            }else{
                // in 14.88Mpps packet processing,
                // 10 usec x 2 delay is 300 packets delay.
                // 1024 dpdk ring buffers are available and sufficient.
                usleep(APP_STAT_DELAY);
                // turn on config changeable bits
                if (stat == APP_STAT_CFGACK){
                    APP_STAT_SET_ACK(lcore);
                }
            }
            if ((burst_count++) > APP_STAT_BURST_PER_LOG) {
                CoreCounter cnt;
                if (core->HasDropped(&cnt)){
                    PGW_LOG(RTE_LOG_CRIT,"packet dropped.(%llu/%llu) on [%s]\n",
                            cnt.drop_bytes_,
                            cnt.drop_count_,
                            core->GetObjectName()
                    );
                }
                burst_count = 0;
            }
        }
        core->AfterCycle(arg);
    }else{
        if (core = app->Find(lcore, true)){
            printf("App::Dispatch-ext[%p] (%u)\n",(void*)pthread_self(), lcore);
            //
            while(!s_quit_){
                auto stat = APP_STAT_STAT();
                if (likely(stat == APP_STAT_BUSYLOOP)){
                    for(auto lcount = 0;lcount< APP_STAT_BURST;lcount++){ 
                        core->VirtualCycle(arg, &cycle_count);
                        cycle_count++;
                    }
                }
            }
        }else if (lcore == 0){
            // change state.
            PGW_LOG(RTE_LOG_INFO, "run state changer.\n");
            //
            while(!s_quit_){
                // state switching every 10 ms
                usleep(APP_STAT_DELAY<<12); 
                if (APP_STAT_STAT() == APP_STAT_DC){                        // initialzie
                    PGW_LOG(RTE_LOG_INFO, ">> status >> dont care.--> init.\n");
                    APP_STAT_SET(APP_STAT_INIT);
                }else if (APP_STAT_STAT() == APP_STAT_INIT){                // complete initialize(fixed interval : halt)
                    if (APP_STAT_ISFIN_PREPARE() == 0){                     //   wait for initialize completed on all CPUs.
                        PGW_LOG(RTE_LOG_INFO, ">> status >> init --> prepared..\n");
                        APP_STAT_SET(APP_STAT_PREPARE);
                    }
                }else if (APP_STAT_STAT() == APP_STAT_PREPARE){             // completed preparing -> loop
                    PGW_LOG(RTE_LOG_INFO, ">> status >> prepare --> busyloop..\n");
                    APP_STAT_RESET_ACK(); 
                    APP_STAT_SET(APP_STAT_BUSYLOOP);
                }else if (APP_STAT_STAT() == APP_STAT_BUSYLOOP){            // loop -> set config
                    if (APP_STAT_HAVE_SIGNAL()){                            //   signal bit off + wait modify config
                        PGW_LOG(RTE_LOG_INFO, ">> status >> busyloop --> config change..\n");
                        APP_STAT_SET(APP_STAT_CFGLOAD);
                    }
                }else if (APP_STAT_STAT() == APP_STAT_CFGLOAD){             // modify config -> change config
                    PGW_LOG(RTE_LOG_INFO, ">> status >> config load --> config ack wait..\n");
                    app->LoadConfig();
                    APP_STAT_RESET_SIGNAL();
                    APP_STAT_RESET_ACK(); 
                    APP_STAT_SET(APP_STAT_CFGACK);
                }else if (APP_STAT_STAT() == APP_STAT_CFGACK){              // modify config ack wait -> change config
                    if (APP_STAT_ISFIN_ACK() == 0){
                        PGW_LOG(RTE_LOG_INFO, ">> status >> config ack wait --> config swap..\n");
                        APP_STAT_SET(APP_STAT_CFGSWAP);
                    }
                }else if (APP_STAT_STAT() == APP_STAT_CFGSWAP){             // change config -> completed preparing
                    PGW_LOG(RTE_LOG_INFO, ">> status >> config swap --> prepared..\n");
                    app->SwapConfig();
                    APP_STAT_SET(APP_STAT_PREPARE);
                }else{
                    auto s = APP_STAT_STAT();
                    rte_panic("unknown status.(%08x: %08x)\n", (unsigned)((s>>32)&0xFFFFFFFF),(unsigned)(s&0xFFFFFFFF));
                }
            }
        }
    }
    return(0);
}
/**
   App::Add , App::Connect commit\n
   *******************************************************************************
   after Commit, can not  App::Add , App::Connect\n
   *******************************************************************************
   @param[in]     cnf    config parameter fixed at compile time
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::App::Commit(StaticConf* cnf){
    // set number of inbound, worker, and outbound cores
    // to scope variable for each cores
    for(auto it = cores_.begin();it != cores_.end();++it){
        auto type = (it->second)->GetType();
        auto opt  = (it->second)->GetN(KEY::OPT);
        switch(type){
            case TYPE::RX:      cnt_rx_ ++; break;
            case TYPE::TX:      cnt_tx_ ++; break;
            case TYPE::PGW_INGRESS:
                cnt_pgw_ingress_ ++; break;
            case TYPE::PGW_EGRESS:
                cnt_pgw_egress_ ++; break;
        }
    }
    for(auto it = cores_.begin();it != cores_.end();++it){
        (it->second)->SetN(KEY::CNT_RX, cnt_rx_);
        (it->second)->SetN(KEY::CNT_TX, cnt_tx_);
        (it->second)->SetN(KEY::CNT_PGW_INGRESS, cnt_pgw_ingress_);
        (it->second)->SetN(KEY::CNT_PGW_EGRESS,  cnt_pgw_egress_);
        // list of cores to use
        APP_STAT_SET_USE((it->first));
    }
    if (cnf && conf_ != cnf){
        (*conf_) = (*cnf);
    }
    s_CONF_TMP_ = (*conf_);

    // apply config
    //   CPU core threads has not yet been activated.
    SwapConfig();
    // TODO, validate state machine regions.
    stat_ ++;

    return(0);
}
/**
   find core instance\n
   *******************************************************************************
   core instance find by core id\n
   *******************************************************************************
   @param[in]     lcore   core id
   @param[in]     ext     extend core
   @return CoreInterface*  NULL!=success , NULL==error
 */
CoreInterface* MIXIPGW::App::Find(COREID lcore, bool ext){
    if (!ext){
        auto it = cores_.find(lcore);
        if (it == cores_.end()){
            return(NULL);
        }
        return((it->second));
    }else{
        auto it = vcores_.find(lcore);
        if (it == vcores_.end()){
            return(NULL);
        }
        return((it->second));
    }
}
/**
   register core instance to application\n
   *******************************************************************************
    Core id must not be duplicated\n
   *******************************************************************************
   @param[in]     corei   core : instance pointer
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::App::Add(CoreInterface* corei){
    // can not add after commit.
    if (stat_ || !corei){
        rte_panic("already commited.(add) %u\n", stat_);
    }
    auto lcore = corei->GetN(KEY::LCORE);
    if (Find(lcore, false) != NULL || !lcore){
        rte_panic("duplicate coreid or cpu[0]. %u\n", lcore);
    }
    cores_[lcore] = corei;

    // - transfer worker
    // - pgw [ingress/egress] 
    //  2 lcore required
    // [0] = dpdkoperation core
    // [1] = protocol host stack : tcp operation core
    if (corei->GetType() == TYPE::WORKER_TRANSFER ||
        corei->GetType() == TYPE::PGW_EGRESS ||
        corei->GetType() == TYPE::PGW_INGRESS){
        auto vlcore = corei->GetN(KEY::OPT);
        vcores_[vlcore] = corei;
        // externd core, required same socket
    }
    // when core is placed in new Numa Node, 
    // reserve a packet regions that allocated for that.
    auto sock = rte_lcore_to_socket_id(lcore);
    if (mpools_.find(sock) == mpools_.end()){
        char nm[64] = {0};
        snprintf(nm, sizeof(nm)-1, "pktmbuf_pool_%u", sock);
    	printf("%s : %u /%u/ %u\n",nm, DEFAULT_MEMPOOL_BUFFERS, DEFAULT_MEMPOOL_CACHE_SIZE, DEFAULT_MBUF_DATA_SIZE);
        // new node
        auto proc_type = rte_eal_process_type();
        auto pool = (proc_type==RTE_PROC_SECONDARY)?
            rte_mempool_lookup(nm):
            rte_pktmbuf_pool_create(nm, DEFAULT_MEMPOOL_BUFFERS, DEFAULT_MEMPOOL_CACHE_SIZE, 0, DEFAULT_MBUF_DATA_SIZE, sock);
        if (!pool) {
            rte_panic("Cannot create mbuf pool on socket %u\n", sock);
        }
        mpools_[sock] = new MemPoolImpl(pool);
    }
    corei->SetP(KEY::OPT, mpools_[sock]);
    return(0);
}
/**
   wait for NIC link-up\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]     port    port id = NIC index
 */
void MIXIPGW::App::WaitLinkup(PORTID port){
    uint8_t portid, all_ports_up = 0;
    struct rte_eth_link link;
    uint32_t n_rx_queues, n_tx_queues, count;

    PGW_LOG(RTE_LOG_INFO, "\nChecking link status(%u)\n", port);
    fflush(stdout);
    for (count = 0; count <= 300; count++) {
        memset(&link, 0, sizeof(link));
        rte_eth_link_get_nowait(port, &link);
        if (link.link_status == ETH_LINK_DOWN) {
            PGW_LOG(RTE_LOG_INFO,". link(%u)\n", count);
            fflush(stdout);
            usleep(1000000);
            continue;
        }
        all_ports_up = 1;
        break;
    }
    PGW_LOG(RTE_LOG_INFO,"\ndone\n");
    fflush(stdout);
    if (all_ports_up == 1) {
        if (link.link_status){
            PGW_LOG(RTE_LOG_INFO,"Port %d Link Up - speed %u Mbps - %s\n",
                   (uint8_t)port, (unsigned)link.link_speed,
                   (link.link_duplex == ETH_LINK_FULL_DUPLEX) ? ("full-duplex") : ("half-duplex\n"));
        } else{
            rte_panic("port %d Link Down\n", (uint8_t)port);
        }
    }else{
        rte_panic("timeout wait linkup %u\n", port);
    }
}
/**
   connect Core and Nic those already commited\n
   *******************************************************************************
   after Nic connected ,  wait for NIC link-up\n
   *******************************************************************************
   @param[in]     core        core id
   @param[in]     rxq         receive queue size
   @param[in]     txq         send queue size
   @param[in]     rxringsize  receive ring descriptor size
   @param[in]     txringsize  send ring descriptor size
   @param[in]     cores       core id array/core , cores is exclusive use
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::App::BindNic(COREID core , SIZE rxq, SIZE txq, SIZE rxringsize, SIZE txringsize, COREID *cores){
    // bind available after commit.
    if (!stat_){
        rte_panic("nothing commited.(for bindnic) %u\n", stat_);
    }
    // primary core
    if (core == (COREID)-1){
        if (cores == NULL || cores[0] == (COREID)-1){
            rte_panic("invalid core configure.(bind nic) %u : %p\n", core, (void*)cores );
        }
        core = cores[0];
    }else if (core != (COREID)-1 && cores != NULL){
        rte_panic("core configure.(bind nic, duplicate) %u : %p\n", core, (void*)cores );
    }else if (core != (COREID)-1 && rxq < 1){
        rte_panic("core configure.(bind nic, rxq is less than 1) %u : %p\n", core, (void*)cores );
    }
    // find object from primary core.
    auto it = cores_.find(core);
    if (it == cores_.end()){
        rte_exit(EXIT_FAILURE, "not found(%u)\n", core);
    }
    auto pcore = (it->second);
    auto type  = pcore->GetType();
    auto lcore = pcore->GetN(KEY::LCORE);
    auto port  = pcore->GetN(KEY::PORT);
    auto queue = pcore->GetN(KEY::QUEUE);
    auto sock  = rte_lcore_to_socket_id(lcore);
#if ARCH==15 || ARCH==16 || ARCH==17 || ARCH==18 || ARCH==19 || ARCH==20 || ARCH==21
    sock = pcore->GetN(KEY::SOCKET);

    if (rte_eal_process_type() == RTE_PROC_SECONDARY){
        WaitLinkup(port);
        return 0;
    }
#endif

    // only Rx and Tx type cores could bind to NIC.
    if (type != TYPE::RX && type != TYPE::TX){
        rte_exit(EXIT_FAILURE, "invalid type(%u)\n", type);
    }
    // initialize NIC.
    auto ret = rte_eth_dev_configure(port, (uint8_t) rxq, (uint8_t) txq, &s_port_conf_);
    if (ret < 0) {
        rte_panic("Cannot init NIC port %u (%d)\n", (unsigned) port, ret);
    }
    // adjust ringsize
    auto nb_rxd = (uint16_t)rxringsize;
    auto nb_txd = (uint16_t)txringsize;
    ret = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
    if (ret != 0){
        rte_panic("failed. adjust nb %u (%d)\n", (unsigned) port, ret);
    }
    rxringsize = nb_rxd;
    txringsize = nb_txd;
    PGW_LOG(RTE_LOG_INFO, "Adjust Nb(port:%u/rx:%u=>%u/tx:%u->%u)\n", port, (unsigned)rxringsize, nb_rxd, (unsigned)txringsize, nb_txd);


    rte_eth_promiscuous_enable(port);
    // search memory buffer pool for each NumaNode.
    auto itm = mpools_.find(sock);
    if (itm == mpools_.end()){
        rte_panic("not found mpool %u \n", (unsigned) sock);
    }
    struct rte_eth_dev_info dev_info;
    auto dev = &rte_eth_devices[port];
    auto conf = &dev->data->dev_conf;
    rte_eth_dev_info_get(port, &dev_info);
    auto rxq_conf = dev_info.default_rxconf;
#if RTE_VER_YEAR > 16
    rxq_conf.offloads = conf->rxmode.offloads;
#endif
    // assign receive queue
    ret = rte_eth_rx_queue_setup(port, queue, (uint16_t)rxringsize, sock, &rxq_conf , (itm->second)->Ref(0));
    if (ret < 0) {
        rte_panic("Cannot init RX queue %u for port %u (%d)\n", (unsigned) queue, (unsigned) port, ret);
    }
    // RSS queue core is not empty(rxq > 1)
    // memory buffer pool are used from same pools
    for(auto n = 1; n < rxq; n++){
        it = cores_.find(cores[n]);
        if (it == cores_.end()){
            rte_exit(EXIT_FAILURE, "not found subcore(%u)\n", cores[n]);
        }
        pcore = (it->second);
        type  = pcore->GetType();
        lcore = pcore->GetN(KEY::LCORE);
        port  = pcore->GetN(KEY::PORT);
        queue = pcore->GetN(KEY::QUEUE);
        sock  = rte_lcore_to_socket_id(lcore);
#if ARCH==15 || ARCH==16 || ARCH==17 || ARCH==18 || ARCH==19 || ARCH==20 || ARCH==21
        sock  = pcore->GetN(KEY::SOCKET);
#endif
        //
        ret = rte_eth_rx_queue_setup(port, queue, (uint16_t)rxringsize, sock, &rxq_conf, (itm->second)->Ref(0));
        if (ret < 0) {
            rte_panic("Cannot init RX queue %u for port %u (%d)\n", (unsigned) queue, (unsigned) port, ret);
        }
    }
    if ((ret = rte_eth_dev_set_vlan_offload(port, ETH_VLAN_STRIP_OFFLOAD)) != 0){
        rte_panic("rte_eth_dev_set_vlan_offload(%d)\n", ret);
    }

    // assign send queue
    for(auto n = 0; n < txq; n++){
        ret = rte_eth_tx_queue_setup(port, (uint8_t)n, (uint16_t) txringsize, sock, NULL);
        if (ret < 0) {
            rte_panic("Cannot init TX queue 0 for port %d (%d)\n", port, ret);
        }
    }
    // Nic Link Up.
    ret = rte_eth_dev_set_mtu(port, 1554);
    if (ret < 0) {
        rte_panic("Cannot set mtu %d (%d)\n", port, ret);
    }
    ret = rte_eth_dev_start(port);
    if (ret < 0) {
        rte_panic("Cannot start port %d (%d)\n", port, ret);
    }
    // Wait Link up.
    WaitLinkup(port);

    return(0);
}
/**
   software ring connects cores\n
   *******************************************************************************
    in dpdk - examples, software ring connection between Rx -> worker -> Tx is
    automatically determined by core configuration
    (incoming port, workers, outgoing ports, etc.)

    those complicate sample source and reduce readability.

    this application focuses on good source code readability
        by restricting settings to compile time.
   *******************************************************************************
   @param[in]     core_fm     source core id
   @param[in]     core_to     destination core id
   @param[in]     size        ring size
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::App::Connect(COREID core_fm, COREID core_to, SIZE size){
    char nm[128] = {0};
    struct rte_ring* ring = NULL;
    // can not connect after commit.
    if (stat_){
        rte_panic("already commited.(for connect) %u\n", stat_);
    }
    auto ita = cores_.find(core_fm);
    auto itb = cores_.find(core_to);
    if (ita == cores_.end() || itb == cores_.end()){
        rte_exit(EXIT_FAILURE, "not found(%u - %u)\n", core_fm, core_to);
    }
    auto pcore_fm = (ita->second);
    auto pcore_to = (itb->second);
    auto type_fm  = pcore_fm->GetType();
    auto type_to  = pcore_to->GetType();
    auto lcore_fm = pcore_fm->GetN(KEY::LCORE);
    auto lcore_to = pcore_to->GetN(KEY::LCORE);
    auto sock_fm  = rte_lcore_to_socket_id(lcore_fm);
    auto sock_to  = rte_lcore_to_socket_id(lcore_to);

#if ARCH==15 || ARCH==16 || ARCH==17 || ARCH==18 || ARCH==19 || ARCH==20 || ARCH==21
    sock_fm = pcore_fm->GetN(KEY::SOCKET);
    sock_to = pcore_to->GetN(KEY::SOCKET);
#endif
    PGW_LOG(RTE_LOG_INFO, "Connect(sock-fm:%u, sock-to:%u,"\
                            "lcore_fm:%u, lcore-to:%u,"\
                            "type_fm:%u, type_to:%u,"\
                            "core_nm_fm:%s, corenm_to:%s)",
        sock_fm, sock_to, lcore_fm, lcore_to,
        type_fm, type_to,
        pcore_fm->GetObjectName(),
        pcore_to->GetObjectName());

    // can not connect same type.
    if (type_fm == type_to){
        rte_exit(EXIT_FAILURE, "same type.(%u - %u)\n", type_fm, type_to);
    }
    // Rx -> Tx, cannot direct connection
    if (type_fm == TYPE::RX && type_to == TYPE::TX){
        rte_exit(EXIT_FAILURE, "cannot direct connect. rx to tx.(%u - %u)\n", type_fm, type_to);
    }
    // connection processing is [Rx] -> [Worker] -> [Tx] forward only.
    if (type_fm == TYPE::RX && type_to == TYPE::WORKER){
        // generate ring
        snprintf(nm, sizeof(nm)-1, "node_%u_rx_%u_worker_%u", sock_fm, lcore_fm, lcore_to);
        ring = rte_ring_create(nm, size, sock_fm, RING_F_SP_ENQ | RING_F_SC_DEQ);
        if (!ring){
            rte_exit(EXIT_FAILURE, "rte_ring_create..(%u - %u:%s)\n", size, sock_fm, nm);
        }
        // connect Rx to Worker
        pcore_fm->SetRingAddr(ORDER::TO, ring);
        pcore_to->SetRingAddr(ORDER::FROM, ring);
    }else if (type_fm == TYPE::WORKER && type_to == TYPE::TX){
        // generate ring
        snprintf(nm, sizeof(nm)-1, "node_%u_worker_%u_tx_%u", sock_to, lcore_fm, lcore_to);
        ring = rte_ring_create(nm, size, sock_to, RING_F_SP_ENQ | RING_F_SC_DEQ);
        if (!ring){
            rte_exit(EXIT_FAILURE, "rte_ring_create..(%u - %u:%s)\n", size, sock_to, nm);
        }
        // connect Worker to Tx
        pcore_fm->SetRingAddr(ORDER::TO, ring);
        pcore_to->SetRingAddr(ORDER::FROM, ring);

    }else if ((type_fm == TYPE::WORKER && type_to == TYPE::WORKER_TRANSFER) ||
              (type_fm == TYPE::COUNTER && type_to == TYPE::WORKER_TRANSFER)){
        // generate ring
        snprintf(nm, sizeof(nm)-1, "node_%u_worker_%u_trans_%u", sock_to, lcore_fm, lcore_to);
        ring = rte_ring_create(nm, size, sock_to, RING_F_SP_ENQ | RING_F_SC_DEQ);
        snprintf(nm, sizeof(nm)-1, "node_%u_trans_%u_int_%u", sock_to, lcore_fm, lcore_to);
        auto ring_int = rte_ring_create(nm, size, sock_to, RING_F_SP_ENQ | RING_F_SC_DEQ);
        if (!ring || !ring_int){
            rte_exit(EXIT_FAILURE, "rte_ring_create..(%u - %u:%s)\n", size, sock_to, nm);
        }
        // connect Worker to Worker Transfer
        pcore_fm->SetRingAddr(ORDER::TO, ring);
        pcore_to->SetRingAddr(ORDER::FROM, ring);
        pcore_to->SetRingAddr(ORDER::TO, ring_int);
    // [PGW:ingress] -> [tap:tx]
    }else if (type_fm == TYPE::PGW_INGRESS && type_to == TYPE::TAPTX){
        // generate ring
        snprintf(nm, sizeof(nm)-1, "node_%u_pgw_i_%u_tap_tx_%u", sock_to, lcore_fm, lcore_to);
        ring = rte_ring_create(nm, size, sock_to, RING_F_SP_ENQ | RING_F_SC_DEQ);
        if (!ring){
            rte_exit(EXIT_FAILURE, "rte_ring_create..(%u - %u:%s)\n", size, sock_to, nm);
        }
        // connect pgw:ingress to tap:tx
        pcore_fm->SetRingAddr(ORDER::EXTEND, ring);
        pcore_to->SetRingAddr(ORDER::FROM_00, ring);
    // [PGW:ingress] -> [tx]
    }else if (type_fm == TYPE::PGW_INGRESS && type_to == TYPE::TX){
        // generate ring
        snprintf(nm, sizeof(nm)-1, "node_%u_pgw_i_%u_tx_%u", sock_to, lcore_fm, lcore_to);
        ring = rte_ring_create(nm, size, sock_to, RING_F_SP_ENQ | RING_F_SC_DEQ);
        if (!ring){
            rte_exit(EXIT_FAILURE, "rte_ring_create..(%u - %u:%s)\n", size, sock_to, nm);
        }
        // connect pgw:ingress to tx
        pcore_fm->SetRingAddr(ORDER::TO, ring);
        pcore_to->SetRingAddr(ORDER::FROM, ring);
    // [tap:rx] -> [PGW:ingress]
    }else if (type_fm == TYPE::TAPRX && type_to == TYPE::PGW_INGRESS){
        // ※warmup ring
        snprintf(nm, sizeof(nm)-1, "node_%u_tap_rx_%u_pgw_i_%u", sock_to, lcore_fm, lcore_to);
        ring = rte_ring_create(nm, size, sock_to, RING_F_SP_ENQ | RING_F_SC_DEQ);
        if (!ring){
            rte_exit(EXIT_FAILURE, "rte_ring_create..(%u - %u:%s)\n", size, sock_to, nm);
        }
        // connect tap:rx to pgw:ingress
        pcore_fm->SetRingAddr(ORDER::TO_00, ring);
        pcore_to->SetRingAddr(ORDER::FROM_00, ring);
    // [distributor rx] -> [PGW:ingress]
    }else if (type_fm == TYPE::RX && type_to == TYPE::PGW_INGRESS){
        snprintf(nm, sizeof(nm)-1, "node_%u_rx_%u_pgw_i_%u", sock_to, lcore_fm, lcore_to);
        ring = rte_ring_create(nm, size, sock_to, RING_F_SP_ENQ | RING_F_SC_DEQ);
        if (!ring){
            rte_exit(EXIT_FAILURE, "rte_ring_create..(%u - %u:%s)\n", size, sock_to, nm);
        }
        // connect distributor:rx to pgw:ingress
        pcore_fm->SetRingAddr(ORDER::TO, ring);
        pcore_to->SetRingAddr(ORDER::FROM, ring);
    // [distributor rx] -> [tap:tx]
    }else if (type_fm == TYPE::RX && type_to == TYPE::TAPTX){
        // ※warmup ring(!= gtpu pass through)
        snprintf(nm, sizeof(nm)-1, "node_%u_rx_%u_tap_tx_%u", sock_to, lcore_fm, lcore_to);
        ring = rte_ring_create(nm, size, sock_to, RING_F_SP_ENQ | RING_F_SC_DEQ);
        if (!ring){
            rte_exit(EXIT_FAILURE, "rte_ring_create..(%u - %u:%s)\n", size, sock_to, nm);
        }
        // connect distributor :rx to tap : tx
        pcore_fm->SetRingAddr(ORDER::EXTEND, ring);
        pcore_to->SetRingAddr(ORDER::FROM_00, ring);
    // [tap:rx] -> [tx]
    }else if (type_fm == TYPE::TAPRX && type_to == TYPE::TX){
        // other gtpu -> tx
        snprintf(nm, sizeof(nm)-1, "node_%u_tap_rx_%u_tx_%u", sock_to, lcore_fm, lcore_to);
        ring = rte_ring_create(nm, size, sock_to, RING_F_SP_ENQ | RING_F_SC_DEQ);
        if (!ring){
            rte_exit(EXIT_FAILURE, "rte_ring_create..(%u - %u:%s)\n", size, sock_to, nm);
        }
        // connect tap : rx to tx
        pcore_fm->SetRingAddr(ORDER::TO, ring);
        pcore_to->SetRingAddr(ORDER::FROM, ring);
    // [tap:rx] -> [PGW:egress]
    }else if (type_fm == TYPE::TAPRX && type_to == TYPE::PGW_EGRESS){
        // ※warmup ring
        snprintf(nm, sizeof(nm)-1, "node_%u_tap_rx_%u_pgw_e_%u", sock_to, lcore_fm, lcore_to);
        ring = rte_ring_create(nm, size, sock_to, RING_F_SP_ENQ | RING_F_SC_DEQ);
        if (!ring){
            rte_exit(EXIT_FAILURE, "rte_ring_create..(%u - %u:%s)\n", size, sock_to, nm);
        }
        // connect tap:rx to pgw:egress
        pcore_fm->SetRingAddr(ORDER::TO_01, ring);
        pcore_to->SetRingAddr(ORDER::FROM_00, ring);
    // [distributor rx] -> [PGW:egress]
    }else if (type_fm == TYPE::RX && type_to == TYPE::PGW_EGRESS){
        snprintf(nm, sizeof(nm)-1, "node_%u_rx_%u_pgw_e_%u", sock_to, lcore_fm, lcore_to);
        ring = rte_ring_create(nm, size, sock_to, RING_F_SP_ENQ | RING_F_SC_DEQ);
        if (!ring){
            rte_exit(EXIT_FAILURE, "rte_ring_create..(%u - %u:%s)\n", size, sock_to, nm);
        }
        // connect distributor:rx to pgw:ingress
        pcore_fm->SetRingAddr(ORDER::TO, ring);
        pcore_to->SetRingAddr(ORDER::FROM, ring);
    // [PGW:egress] -> [tx]
    }else if (type_fm == TYPE::PGW_EGRESS && type_to == TYPE::TX){
        // generate ring
        snprintf(nm, sizeof(nm)-1, "node_%u_pgw_e_%u_tx_%u", sock_to, lcore_fm, lcore_to);
        ring = rte_ring_create(nm, size, sock_to, RING_F_SP_ENQ | RING_F_SC_DEQ);
        if (!ring){
            rte_exit(EXIT_FAILURE, "rte_ring_create..(%u - %u:%s)\n", size, sock_to, nm);
        }
        // connect pgw:egress to tx
        pcore_fm->SetRingAddr(ORDER::TO, ring);
        pcore_to->SetRingAddr(ORDER::FROM, ring);

    // [PGW:egress] -> [tap:tx]
    }else if (type_fm == TYPE::PGW_EGRESS && type_to == TYPE::TAPTX){
        // generate ring
        snprintf(nm, sizeof(nm)-1, "node_%u_pgw_e_%u_tap_tx_%u", sock_to, lcore_fm, lcore_to);
        ring = rte_ring_create(nm, size, sock_to, RING_F_SP_ENQ | RING_F_SC_DEQ);
        if (!ring){
            rte_exit(EXIT_FAILURE, "rte_ring_create..(%u - %u:%s)\n", size, sock_to, nm);
        }
        // connect pgw:egress to tap:tx
        pcore_fm->SetRingAddr(ORDER::EXTEND, ring);
        pcore_to->SetRingAddr(ORDER::FROM_00, ring);
    // [PGW:ingress] -> [counter:ingress]
    }else if (type_fm == TYPE::PGW_INGRESS && type_to == TYPE::COUNTER){
        // generate ring
        snprintf(nm, sizeof(nm)-1, "node_%u_pgw_i_%u_counter_i_%u", sock_to, lcore_fm, lcore_to);
        ring = rte_ring_create(nm, size, sock_to, RING_F_SP_ENQ | RING_F_SC_DEQ);
        if (!ring){
            rte_exit(EXIT_FAILURE, "rte_ring_create..(%u - %u:%s)\n", size, sock_to, nm);
        }
        // connect pgw:ingress to counter:ingress
        pcore_fm->SetRingAddr(ORDER::TO, ring);
        pcore_to->SetRingAddr(ORDER::FROM, ring);

    // [PGW:egress] -> [counter:egress]
    }else if (type_fm == TYPE::PGW_EGRESS && type_to == TYPE::COUNTER){
        // generate ring
        snprintf(nm, sizeof(nm)-1, "node_%u_pgw_e_%u_counter_e_%u", sock_to, lcore_fm, lcore_to);
        ring = rte_ring_create(nm, size, sock_to, RING_F_SP_ENQ | RING_F_SC_DEQ);
        if (!ring){
            rte_exit(EXIT_FAILURE, "rte_ring_create..(%u - %u:%s)\n", size, sock_to, nm);
        }
        // connect pgw:egress to counter:egress.
        pcore_fm->SetRingAddr(ORDER::TO, ring);
        pcore_to->SetRingAddr(ORDER::FROM, ring);
    // [counter:egress/ingress] -> [nic:TX]
    }else if (type_fm == TYPE::COUNTER && type_to == TYPE::TX){
        snprintf(nm, sizeof(nm)-1, "node_%u_counter_%u_nictx_%u", sock_to, lcore_fm, lcore_to);
        ring = rte_ring_create(nm, size, sock_to, RING_F_SP_ENQ | RING_F_SC_DEQ);
        if (!ring){
            rte_exit(EXIT_FAILURE, "rte_ring_create..(%u - %u:%s)\n", size, sock_to, nm);
        }
        // counter:ingress/egress -> nic:Tx
        pcore_fm->SetRingAddr(ORDER::EXTEND, ring);
        pcore_to->SetRingAddr(ORDER::FROM, ring);

    }else{
        rte_exit(EXIT_FAILURE, "not support ordering.(%u:%u - %u:%u)\n", type_fm, lcore_fm , type_to, lcore_to);
    }
    printf("%s : %u ring entries are now free\n",nm, rte_ring_free_count(ring));

    return(0);
}
/**
   signal handler\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]     signo       signal number 
 */
void MIXIPGW::App::Signal(int signo){
    if (signo == SIGUSR1){
        // reload config.
        printf("MIXIPGW::App::Signal/reload:%d\n", signo);
        APP_STAT_SET_SIGNAL(APP_STAT_SIGUSR1);
    }else if (signo == SIGUSR2){
        printf("MIXIPGW::App::Signal/usr2:%d\n", signo);
        APP_STAT_SET_SIGNAL(APP_STAT_SIGUSR2);
    }else if (signo == SIGINT){
        printf("MIXIPGW::App::Signal/interupt:%d\n", signo);
        s_quit_ = 1;
    }
}
/**
   get exit status\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return RETCD  0==not terminated , 0!=should be terminated
 */
int  MIXIPGW::App::Halt(void){
    return(s_quit_);
}

/**
   read config(reload)\n
   *******************************************************************************
   \n
   *******************************************************************************
 */
void MIXIPGW::App::LoadConfig(void){
    PGW_LOG(RTE_LOG_INFO, "LoadConfig\n");
    //
    char user[32] = {0};
    char pwd[32] = {0};
    char host[32] = {0};
    unsigned int port = 0;

#define CFG_LOGLEVEL            ("loglevel")
#define CFG_BURST_RX_READ       ("burst_size_rx_read")
#define CFG_BURST_RX_ENQ        ("burst_size_rx_enq")
#define CFG_BURST_RX_DEQ        ("burst_size_rx_deq")
#define CFG_BURST_TX_ENQ        ("burst_size_tx_enq")
#define CFG_BURST_TX_DEQ        ("burst_size_tx_deq")
#define CFG_BURST_TX_WRITE      ("burst_size_tx_write")
//
#define CFG_FLUSH_COUNTER       ("flush_counter")
#define CFG_FLUSH_ENCAP         ("flush_encap")
#define CFG_FLUSH_GRETERM       ("flush_greterm")
#define CFG_FLUSH_PGW_IE        ("flush_pgw_ie")
#define CFG_FLUSH_RX            ("flush_rx")
#define CFG_FLUSH_RX_DIST       ("flush_rx_dist")
#define CFG_FLUSH_TX            ("flush_tx")
#define CFG_POLICER_CIR         ("policer_cir")
#define CFG_POLICER_CBS         ("policer_cbs")
#define CFG_POLICER_EIR         ("policer_eir")
#define CFG_POLICER_EBS         ("policer_ebs")

#ifndef USE_MYSQL_CONFIG
    // file interface 
    char f[128] = {0};
    //
    snprintf(f, sizeof(f)-1, "/tmp/%u.cnf", getpid());
    auto fd = fopen(f, "r");
    if (fd){
        bzero(f, sizeof(f));
        while (fgets(f, sizeof(f), fd)) {
            printf("%s", f);
            auto p = strchr(f, '=');
            if (p != NULL){
                (*p) = 0x00;
                const char *val = (p + 1);
                auto p2 = strchr(f, '\n');
                if (p2 != NULL){
                    (*p2) = 0x00;
                }
                PGW_LOG(RTE_LOG_INFO, "LoadConfig from file(%s : %s)\n", f, val);
                if (strcmp(f, CFG_LOGLEVEL) == 0){                  s_PGW_LOG_LEVEL = atoi(val);
                }else if (strcmp(f, CFG_BURST_RX_READ) == 0){       s_CONF_TMP_.burst_size_rx_read_ = (SIZE)atoi(val);
                }else if (strcmp(f, CFG_BURST_RX_ENQ) == 0){        s_CONF_TMP_.burst_size_rx_enq_  = (SIZE)atoi(val);
                }else if (strcmp(f, CFG_BURST_RX_DEQ) == 0){        s_CONF_TMP_.burst_size_rx_deq_  = (SIZE)atoi(val);
                }else if (strcmp(f, CFG_BURST_TX_ENQ) == 0){        s_CONF_TMP_.burst_size_tx_enq_  = (SIZE)atoi(val);
                }else if (strcmp(f, CFG_BURST_TX_DEQ) == 0){        s_CONF_TMP_.burst_size_tx_deq_  = (SIZE)atoi(val);
                }else if (strcmp(f, CFG_BURST_TX_WRITE) == 0){      s_CONF_TMP_.burst_size_tx_write_= (SIZE)atoi(val);
                }else if (strcmp(f, CFG_FLUSH_COUNTER) == 0){       s_FLUSH_COUNTER     = (uint32_t)atoi(val);
                }else if (strcmp(f, CFG_FLUSH_ENCAP) == 0){         s_FLUSH_ENCAP       = (uint32_t)atoi(val);
                }else if (strcmp(f, CFG_FLUSH_GRETERM) == 0){       s_FLUSH_GRETERM     = (uint32_t)atoi(val);
                }else if (strcmp(f, CFG_FLUSH_PGW_IE) == 0){        s_FLUSH_PGW_IE      = (uint32_t)atoi(val);
                }else if (strcmp(f, CFG_FLUSH_RX) == 0){            s_FLUSH_RX          = (uint32_t)atoi(val);
                }else if (strcmp(f, CFG_FLUSH_RX_DIST) == 0){       s_FLUSH_RX_DIST     = (uint32_t)atoi(val);
                }else if (strcmp(f, CFG_FLUSH_TX) == 0){            s_FLUSH_TX          = (uint32_t)atoi(val);
                }else if (strcmp(f, CFG_POLICER_CIR) == 0){         s_POLICER_CIR       = (uint32_t)atoi(val);
                }else if (strcmp(f, CFG_POLICER_CBS) == 0){         s_POLICER_CBS       = (uint32_t)atoi(val);
                }else if (strcmp(f, CFG_POLICER_EIR) == 0){         s_POLICER_EIR       = (uint32_t)atoi(val);
                }else if (strcmp(f, CFG_POLICER_EBS) == 0){         s_POLICER_EBS       = (uint32_t)atoi(val);
                }
            }
            bzero(f, sizeof(f));
        }
        //
        fclose(fd);
    }
#endif
}
/**
   copy config to core scope variables \n
   *******************************************************************************
   \n
   *******************************************************************************
 */
void MIXIPGW::App::SwapConfig(void){
    PGW_LOG_LEVEL[rte_lcore_id()] = RTE_LOG_DEBUG+1;
    PGW_LOG(RTE_LOG_INFO, "SwapConfig(%u/%p/%d)\n", (unsigned)cores_.size(), (void*)pthread_self(), s_PGW_LOG_LEVEL);

    // copy global scope configurations to core private scope variables.
    for(auto it = cores_.begin();it != cores_.end();++it){
        auto type = (it->second)->GetType();
        switch(type){
            case TYPE::RX:
            case TYPE::TAPRX:
            case TYPE::PGW_INGRESS:
                (it->second)->SetN(KEY::BURST_FROM, s_CONF_TMP_.burst_size_rx_read_);
                (it->second)->SetN(KEY::BURST_TO, s_CONF_TMP_.burst_size_rx_enq_);
                break;
            case TYPE::WORKER:
            case TYPE::COUNTER:
            case TYPE::WORKER_TRANSFER:
                (it->second)->SetN(KEY::BURST_FROM, s_CONF_TMP_.burst_size_rx_deq_);
                (it->second)->SetN(KEY::BURST_TO, s_CONF_TMP_.burst_size_tx_enq_);
                break;
            case TYPE::TX:
            case TYPE::TAPTX:
            case TYPE::PGW_EGRESS:
                (it->second)->SetN(KEY::BURST_FROM, s_CONF_TMP_.burst_size_tx_deq_);
                (it->second)->SetN(KEY::BURST_TO, s_CONF_TMP_.burst_size_tx_write_);
                break;
        }
        // flush delay. 
        auto objnm = (it->second)->GetObjectName();
        if (strcmp(objnm, "CoreCounterWorker") == 0){           (it->second)->SetN(KEY::FLUSH_DELAY, s_FLUSH_COUNTER); 
        }else if (strcmp(objnm, "CoreEncapWorker") == 0){       (it->second)->SetN(KEY::FLUSH_DELAY, s_FLUSH_ENCAP); 
        }else if (strcmp(objnm, "CoreGretermWorker") == 0){     (it->second)->SetN(KEY::FLUSH_DELAY, s_FLUSH_GRETERM); 
        }else if (strcmp(objnm, "CoreGtpu2GreWorker") == 0){    (it->second)->SetN(KEY::FLUSH_DELAY, s_FLUSH_COUNTER); 
        }else if (strcmp(objnm, "CorePgwEgressDistributorWorker") == 0 || 
                  strcmp(objnm, "CorePgwEgressWorker") == 0 ||
                  strcmp(objnm, "CorePgwIngressDistributorWorker") == 0 ||
                  strcmp(objnm, "CorePgwEgressTunnelWorker") == 0 ||
                  strcmp(objnm, "CorePgwIngressTunnelWorker") == 0 ||
                  strcmp(objnm, "CorePgwIngressWorker") == 0){  (it->second)->SetN(KEY::FLUSH_DELAY, s_FLUSH_PGW_IE); 
        }else if (strcmp(objnm, "CoreRx") == 0){                (it->second)->SetN(KEY::FLUSH_DELAY, s_FLUSH_RX); 
        }else if (strcmp(objnm, "CoreRxDistributor") == 0){     (it->second)->SetN(KEY::FLUSH_DELAY, s_FLUSH_RX); 
        }else if (strcmp(objnm, "CoreTapRx") == 0){             (it->second)->SetN(KEY::FLUSH_DELAY, s_FLUSH_RX); 
        }else if (strcmp(objnm, "CoreTapTx") == 0){             (it->second)->SetN(KEY::FLUSH_DELAY, s_FLUSH_TX); 
        }else if (strcmp(objnm, "CoreTransferWorker") == 0){    (it->second)->SetN(KEY::FLUSH_DELAY, s_FLUSH_COUNTER); 
        }else if (strcmp(objnm, "CoreTx") == 0){                (it->second)->SetN(KEY::FLUSH_DELAY, s_FLUSH_TX);
        }
        // policer
        (it->second)->SetN(KEY::POLICER_CIR, s_POLICER_CIR);
        (it->second)->SetN(KEY::POLICER_CBS, s_POLICER_CBS);
        (it->second)->SetN(KEY::POLICER_EIR, s_POLICER_EIR);
        (it->second)->SetN(KEY::POLICER_EBS, s_POLICER_EBS);
    }
    // logging global config values
    PGW_LOG(RTE_LOG_INFO, "FLUSH_COUNTER => %u\n", s_FLUSH_COUNTER);
    PGW_LOG(RTE_LOG_INFO, "FLUSH_ENCAP   => %u\n", s_FLUSH_ENCAP);
    PGW_LOG(RTE_LOG_INFO, "FLUSH_GRETERM => %u\n", s_FLUSH_GRETERM);
    PGW_LOG(RTE_LOG_INFO, "FLUSH_PGW_IE  => %u\n", s_FLUSH_PGW_IE);
    PGW_LOG(RTE_LOG_INFO, "FLUSH_RX      => %u\n", s_FLUSH_RX);
    PGW_LOG(RTE_LOG_INFO, "FLUSH_RX_DIST => %u\n", s_FLUSH_RX_DIST);
    PGW_LOG(RTE_LOG_INFO, "FLUSH_TX      => %u\n", s_FLUSH_TX);
    PGW_LOG(RTE_LOG_INFO, "POLICER_CIR   => %u\n", s_POLICER_CIR);
    PGW_LOG(RTE_LOG_INFO, "POLICER_CBS   => %u\n", s_POLICER_CBS);
    PGW_LOG(RTE_LOG_INFO, "POLICER_EIR   => %u\n", s_POLICER_EIR);
    PGW_LOG(RTE_LOG_INFO, "POLICER_EBS   => %u\n", s_POLICER_EBS);
    for(auto n = 0;n < PGW_LOG_CORES;n++){
        MIXIPGW::PGW_LOG_LEVEL[n] = s_PGW_LOG_LEVEL;
    }
}

