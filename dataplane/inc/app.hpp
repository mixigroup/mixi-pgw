/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       app.hpp
    @brief      application define
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
#ifndef __APP_MIXIPGW_HPP
#define __APP_MIXIPGW_HPP
#include "mixi_pgw_data_plane_def.hpp"

#define APP_STAT_DC             (1ULL<<0)
#define APP_STAT_INIT           (1ULL<<1)
#define APP_STAT_PREPARE        (1ULL<<2)
#define APP_STAT_BUSYLOOP       (1ULL<<3)
#define APP_STAT_CFGLOAD        (1ULL<<4)
#define APP_STAT_CFGACK         (1ULL<<5)
#define APP_STAT_CFGSWAP        (1ULL<<6)
#define APP_STAT_SIGUSR1        (1ULL<<62)
#define APP_STAT_SIGUSR2        (1ULL<<63)
#define APP_STAT_DELAY          (4)
#define APP_STAT_BURST          (1<<18)
#define APP_STAT_BURST_PER_LOG  (1<<4)
#define APP_STAT_EMASK          (0xFFFFFFFFFFFFFF00)
#define APP_STAT_MASK           (0x00000000000000FF)        // status code
#define APP_STAT_ESIGMASK       (0x3FFFFFFFFFFFFFFF)
#define APP_STAT_SIGMASK        (0xC000000000000000)
#define APP_STAT_HAVE_SIGNAL()  (APP_STAT_SIGMASK&rte_atomic64_read(&s_status_))
#define APP_STAT_RESET_SIGNAL() {rte_spinlock_lock(&s_spinlock);\
                                    auto __v = rte_atomic64_read(&s_status_);\
                                    __v &= APP_STAT_ESIGMASK;\
                                    rte_atomic64_set(&s_status_, __v);\
                                rte_spinlock_unlock(&s_spinlock);}

#define APP_STAT_SET_SIGNAL(s)  {rte_spinlock_lock(&s_spinlock);\
                                    auto __v = rte_atomic64_read(&s_status_);\
                                    __v = ((__v&APP_STAT_ESIGMASK)|(s&APP_STAT_SIGMASK));\
                                    rte_atomic64_set(&s_status_, __v);\
                                rte_spinlock_unlock(&s_spinlock);}

#define APP_STAT_STAT()         (APP_STAT_MASK&rte_atomic64_read(&s_status_))
#define APP_STAT_ISFIN_PREPARE()((rte_atomic64_read(&s_core_used_)==rte_atomic64_read(&s_core_prepared_))?0:1)
#define APP_STAT_SET_PREPARE(c) {rte_spinlock_lock(&s_spinlock);\
                                    auto __v = rte_atomic64_read(&s_core_prepared_);\
                                    __v |= (1ULL<<c);\
                                    rte_atomic64_set(&s_core_prepared_, __v);\
                                rte_spinlock_unlock(&s_spinlock);}
#define APP_STAT_SET_USE(c)     {rte_spinlock_lock(&s_spinlock);\
                                    auto __v = rte_atomic64_read(&s_core_used_);\
                                    __v |= (1ULL<<c);\
                                    rte_atomic64_set(&s_core_used_, __v);\
                                rte_spinlock_unlock(&s_spinlock);}
#define APP_STAT_SET(c)         {rte_spinlock_lock(&s_spinlock);\
                                    auto __v = rte_atomic64_read(&s_status_);\
                                    __v = ((__v&APP_STAT_EMASK)|(c&APP_STAT_MASK));\
                                    rte_atomic64_set(&s_status_, __v);\
                                rte_spinlock_unlock(&s_spinlock);}
#define APP_STAT_SET_ACK(c)     {rte_spinlock_lock(&s_spinlock);\
                                    auto __v = rte_atomic64_read(&s_core_ack_);\
                                    __v |= (1ULL<<c);\
                                    rte_atomic64_set(&s_core_ack_, __v);\
                                rte_spinlock_unlock(&s_spinlock);}
#define APP_STAT_ISFIN_ACK()    ((rte_atomic64_read(&s_core_used_)==rte_atomic64_read(&s_core_ack_))?0:1)
#define APP_STAT_RESET_ACK()    {rte_spinlock_lock(&s_spinlock);\
                                    rte_atomic64_set(&s_core_ack_, 0);\
                                rte_spinlock_unlock(&s_spinlock);}

namespace MIXIPGW{
  class StaticConf;
  class CoreInterface;
  class CoreMempool;
  typedef std::unordered_map<SOCKID, CoreMempool*>        Mpool;
  typedef std::unordered_map<COREID, CoreInterface*>      Cores;

  /** *****************************************************
  * @brief
  * Application\n
  * \n
  */
  class App{
  public:
      virtual ~App();
  public:
      static App* Init(int , char **);   /*!< Initialize */
  public:
      virtual RETCD Run(void);           /*!< main loop */
      virtual RETCD Connect(COREID, COREID, SIZE);
      virtual RETCD Commit(StaticConf*);
      CoreInterface* Find(COREID,bool);
      RETCD Add(CoreInterface*);
      RETCD BindNic(COREID, SIZE, SIZE, SIZE, SIZE, COREID*);
  protected:
      static int Dispatch(void*);        /*!< function every cores */
      static void Signal(int);           /*!< signal event */
      static int  Halt(void);            /*!< complete flags */
#ifdef __TEST_MODE__
  public:
#elif ARCH>=15
  public:
#else
  protected:
#endif
      void WaitLinkup(PORTID);
      void LoadConfig(void);
      void SwapConfig(void);

      Cores       cores_;                /*!< core map */
      Cores       vcores_;               /*!< core map(extend) */
      Mpool       mpools_;               /*!< memory pool */
      RETCD       stat_;                 /*!< application status */
      SIZE        cnt_rx_;               /*!< count of Rx Cores */
      SIZE        cnt_wk_;               /*!< count of Worker Cores */
      SIZE        cnt_tx_;               /*!< count of Tx Cores */
      SIZE        cnt_pgw_ingress_;      /*!< count of PGW ingress Cores */
      SIZE        cnt_pgw_egress_;       /*!< count of PGW egress Cores */
      StaticConf* conf_;                 /*!< static config */
      std::string dburi_;                /*!< database URI */
      static struct rte_eth_conf s_port_conf_;
  protected:
      App();
  };
}; // namespace MIXIPGW

#endif //__APP_MIXIPGW_HPP
