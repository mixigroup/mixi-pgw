/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       core.hpp
    @brief      core defined
*******************************************************************************
   core = implemented 1 instance equals 1 function\n
   worker core has logic\n
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
#ifndef __MIXIPGW_CORE_HPP
#define __MIXIPGW_CORE_HPP

#include "mixi_pgw_data_plane_def.hpp"

namespace MIXIPGW{

  /*! @name Core Defined */
  /* @{ */
  static const uint8_t COUNTER_INGRESS  = (0xfe);
  static const uint8_t COUNTER_EGRESS   = (0xff);
  static const uint8_t TUNNEL_INGRESS  = (0xfe);
  static const uint8_t TUNNEL_EGRESS   = (0xff);

  static const unsigned COUNT_MAP_SZ = 16;        /*!< count of maps */
  static const unsigned TRANSFER_RING_BURST = 64; /*!< burst size of transfer */
  static const unsigned TRANSFER_RING_SZ = 1024;  /*!< burst ring size of transfer */
  static const unsigned COUNT_TPS = (TRANSFER_RING_BURST*TRANSFER_RING_SZ); /*!< transaction per second(about enqueue) */
  /* @} */

  /*! @struct counting_item
      @brief
      counting item\n
      8 bytes\n
  */
  typedef struct counting_item {
      uint32_t    key:32;
      uint32_t    used:32;
      uint32_t    used_3g:32;
      uint32_t    used_low:32;
  } __attribute__ ((packed)) counting_item_t,*counting_item_ptr;
  static_assert(sizeof(struct counting_item)==16, "must 16 octet");
  /*! @struct counting
      @brief
      counting packet\n
      128 burst\n
  */
  typedef struct counting{
      uint32_t    type:8;
      uint32_t    len:24;
      uint32_t    count:32;
      uint32_t    reserved:32;
      counting_item_t   items[TRANSFER_RING_BURST];
  } __attribute__ ((packed)) counting_t,*counting_ptr;
  static_assert(sizeof(struct counting)==1024+12, "must 1024+12 octet");

  /*! @struct lookup
      @brief
      lookup item\n
      \n
  */
  typedef struct lookup{
      struct _stat{
          uint32_t  valid:1;
          uint32_t  active:1;
          uint32_t  mode:2;
          uint32_t  padd:28;
      }stat;
      uint32_t  ue_ipv4;
      uint32_t  ue_teid;
      uint32_t  sgw_gtpu_teid;
      uint32_t  sgw_gtpu_ipv4;
      uint32_t  pgw_gtpu_ipv4;
#if ARCH>=15
      //
      uint32_t    epoch_w;
      uint32_t    commit_rate;
      uint32_t    commit_burst_size;
      uint32_t    commit_information_rate;
      uint32_t    excess_rate;
      uint32_t    excess_burst_size;
      uint32_t    excess_information_rate;
#endif
  }__attribute__ ((packed)) lookup_t, *lookup_ptr;
#if ARCH>=15
  static_assert(sizeof(lookup_t)==52,   "must 52 octet");
#else
  static_assert(sizeof(lookup_t)==24,   "must 24 octet");
#endif

  /*! @name mixi_pgw_data_plane typedef */
  /* @{ */
  typedef struct rte_mbuf*              Buf;        /*!< mbuf alias */
  typedef struct rte_ring*              SingleRing; /*!< ring alias */
  typedef std::vector<struct rte_ring*> Ring;       /*!< vector<ring> alias */
  typedef std::unordered_map<uint32_t, counting_item_t> CountingMap;  /*!< counting map */
  /* @} */

  /** *****************************************************
  * @brief
  * instance of counting \n
  * \n
  */
  class CoreCounter{
  public:
      CoreCounter(){
          Clear();
      }
      CoreCounter(CoreCounter& cp){
          drop_bytes_ = cp.drop_bytes_;
          drop_count_ = cp.drop_count_;
          transfer_bytes_ = cp.transfer_bytes_;
          transfer_count_ = cp.transfer_count_;
      }
      bool HasDropped(){
          return((drop_bytes_+drop_count_)?true:false);
      }
      void Clear(void){
          drop_bytes_ = transfer_bytes_ = drop_count_ = transfer_count_ = 0;
      }
      void Inc(uint64_t drop_bytes, uint64_t drop_count, uint64_t transfer_bytes, uint64_t transfer_count){
          if (drop_bytes){ drop_bytes_ += drop_bytes; }
          if (drop_count){ drop_count_ += drop_count; }
          if (transfer_bytes) { transfer_bytes_ += transfer_bytes; }
          if (transfer_count) { transfer_count_ += transfer_count; }
      }
  public:
      uint64_t  drop_bytes_,drop_count_;            /*!< count(drop counting) */
      uint64_t  transfer_bytes_,transfer_count_;    /*!< count(transfer counting) */
  };


  /** *****************************************************
  * @brief
  * Core Interface\n
  * \n
  */
  class CoreInterface{
  protected:
      CoreInterface(PORTID, QUEUEID, COREID);
  public:   // must be implemented.
      virtual TXT   GetObjectName(void) = 0;
      virtual TYPE  GetType(void) = 0;
      virtual RETCD SetRingAddr(ORDER, void*) = 0;
      virtual RETCD BeforeCycle(void*);
      virtual RETCD Cycle(void*,uint64_t*) = 0;
      virtual RETCD AfterCycle(void*);
      virtual RETCD VirtualCycle(void*,uint64_t*);
  public:   // override.
      virtual VALUE GetN(KEY);
      virtual void SetN(KEY,VALUE);
      virtual void SetP(KEY,void*);
      virtual bool HasDropped(CoreCounter*);
  protected:
      RETCD IsAvailableCore(COREID);
  protected:
      void BurstFlush(void*);
      void SendOrBuffer(void* , struct rte_mbuf*);
  protected:
      PORTID  port_;        /*!< number of port */
      QUEUEID queue_;       /*!< number of queue */
      COREID  lcore_;       /*!< count of cores */
      VALUE   connected_;   /*!< connected flags */
      VALUE   socket_;
      SIZE    cnt_rx_;      /*!< count of Rx Cores */
      SIZE    cnt_tx_;      /*!< count of Tx Cores */
      SIZE    cnt_wk_;      /*!< count of Worker Cores */
      SIZE    cnt_pgw_in_;  /*!< count of PGW Ingress Cores  */
      SIZE    cnt_pgw_eg_;  /*!< count of PGW Egress Cores */
      SIZE    burst_fm_;    /*!< count of burst(ingress-side) */
      SIZE    burst_to_;    /*!< count of burst(egress-side) */
      SingleRing  ring_fm_; /*!< from: single ring(direction input) */
      SingleRing  ring_to_; /*!< to  : single ring(direction output) */
      Buf         input_[DEFAULT_MBUF_ARRAY_SIZE];  /*!< input buffer */
      Buf         output_[DEFAULT_MBUF_ARRAY_SIZE]; /*!< output buffer */
      SIZE        output_cnt_;  /*!< count of output */
      uint64_t    flush_delay_; /*!< delay(flush) */
      CoreCounter counter_; /*!< counting */
  };

 /** *****************************************************
  * @brief
  * Rx Core\n
  * \n
  */
  class CoreRx: public CoreInterface{
  public:
#if defined(COUNTER_RX) || defined(TUNNEL_RX)
      CoreRx(PORTID,QUEUEID,COREID,unsigned);
#else
      CoreRx(PORTID,QUEUEID,COREID);
#endif
      virtual ~CoreRx(){}
  public:
      virtual TXT   GetObjectName(void);
      virtual TYPE  GetType(void);
      virtual RETCD SetRingAddr(ORDER, void*);
      virtual RETCD Cycle(void*,uint64_t*);
  private:
      void BurstFlushRx(void*);
      void SendOrBufferRx(void* , unsigned, struct rte_mbuf*);
  private:
      unsigned     type_;            /*!< type */
#ifdef __TEST_MODE__
      SingleRing  ring_test_from_;                                     /*!< input packet ring for test compilation. */
#endif
      Ring ring_rx_;                                                   /*!< Rx -> Worker direction ring address, multiple */
      Buf  output_rx_[DEFAULT_MBUF_OUT_SIZE][DEFAULT_MBUF_ARRAY_SIZE]; /*!< Rx -> Worker output buffer */
      SIZE output_cnt_rx_[DEFAULT_MBUF_OUT_SIZE];                      /*!< Rx -> Worker count of output */
  };

  /** *****************************************************
   * @brief
   * Tx Core\n
   * \n
   */
  class CoreTx: public CoreInterface{
  public:
      CoreTx(PORTID,COREID);
      CoreTx(PORTID,QUEUEID,COREID);
#ifdef __TEST_MODE__
      CoreTx(PORTID,QUEUEID,COREID,unsigned);
#endif
      virtual ~CoreTx(){}
  public:
      virtual TXT  GetObjectName(void);
      virtual TYPE GetType(void);
      virtual RETCD SetRingAddr(ORDER, void*);
      virtual RETCD Cycle(void*,uint64_t*);
  private:
      void BurstFlushTx(void*);
  private:
#ifdef __TEST_MODE__
      SingleRing  ring_test_to_;                 /*!< output packet ring for test compilation */
      unsigned    debug_flag_;                   /*!< debug flags */
#endif
      Ring  ring_tx_;                            /*!< Worker->Tx direction ring address, multiple */
  };

  /** *****************************************************
   * @brief
   * EncapsulateWorker Core\n
   * \n
   */
  class CoreEncapWorker: public CoreInterface{
  public:
      CoreEncapWorker(COREID,unsigned);
      virtual ~CoreEncapWorker();
  public:
      virtual TXT  GetObjectName(void);
      virtual TYPE GetType(void);
      virtual RETCD SetRingAddr(ORDER, void*);
      virtual VALUE GetN(KEY);
      virtual RETCD Cycle(void*,uint64_t*);
  private:
      RETCD ModifyPacket(struct rte_mbuf*);
  private:
      unsigned    srcipv4_;     /*!< option value: source ip address at encapsulation */
      struct gtpu_hdr*  gtpuh_; /*!< default packet : gtpu header */
      struct udp_hdr*   udph_;  /*!< default packet : udp header */
      struct ipv4_hdr*  iph_;   /*!< default packet : ip header */
  };

  /** *****************************************************
   * @brief
   * Exchange Gre from Gtpu Worker Core\n
   * \n
   */
  class CoreGtpu2GreWorker: public CoreInterface{
  public:
      CoreGtpu2GreWorker(COREID,unsigned);
      virtual ~CoreGtpu2GreWorker();
  public:
      virtual TXT  GetObjectName(void);
      virtual TYPE GetType(void);
      virtual RETCD SetRingAddr(ORDER, void*);
      virtual void SetN(KEY,VALUE);
      virtual VALUE GetN(KEY);
      virtual RETCD Cycle(void*,uint64_t*);
  private:
      RETCD ModifyPacket(struct rte_mbuf*);
  private:
      unsigned    pgwipv4_;     /*!< option value, ipv4 address of PGW */
  };

 /** *****************************************************
  * @brief
  * Exchange Gre from Gtpu Worker Core\n
  * \n
  */
  class CoreGretermWorker: public CoreInterface{
  public:
      CoreGretermWorker(COREID,unsigned);
      virtual ~CoreGretermWorker();
  public:
      virtual TXT  GetObjectName(void);
      virtual TYPE GetType(void);
      virtual RETCD SetRingAddr(ORDER, void*);
      virtual RETCD Cycle(void*,uint64_t*);
  private:
      RETCD ModifyPacket(struct rte_mbuf*);
  private:
      unsigned    opt_;     /*!< option value */
  };

  /** *****************************************************
   * @brief
   * struct rte_mempool* reference interface\n
   * \n
   */
  class CoreMempool{
  public:
      virtual struct rte_mempool* Ref(COREID)=0; /*!< pooling reference interface */
  };

  /** *****************************************************
   * @brief
   * counter Worker Core\n
   * \n
   */
  class CoreCounterWorker: public CoreInterface{
  public:
      CoreCounterWorker(COREID,unsigned);
      virtual ~CoreCounterWorker();
  public:
      virtual TXT  GetObjectName(void);
      virtual TYPE GetType(void);
      virtual void SetP(KEY,void*);
      virtual RETCD SetRingAddr(ORDER, void*);
      virtual RETCD Cycle(void*,uint64_t*);
  protected:
      RETCD Counting(uint32_t,uint16_t,uint32_t);
      void ForwardTransfer(void*);
      void SendOrBufferPassThrough(void* , struct rte_mbuf*);
      void BurstFlushPassthrough(void* );
  protected:
      unsigned    type_;            /*!< type */
      unsigned    cur_;             /*!< current map */
      CountingMap map_[COUNT_MAP_SZ]; /*!< map of counter */
      CoreMempool* mpl_;            /*!< memory pool reference interface */
      uint64_t forward_cycle_;      /*!< cycle(forward) */
      uint64_t forward_count_;      /*!< count of forward(forward) */
      uint64_t virtual_cycle_;      /*!< cycle(virtual) */
      uint64_t virtual_count_;      /*!< count of virtual */
      uint64_t counting_cycle_;     /*!< counting(counting) */
      uint64_t counting_count_;     /*!< count of counting */
      uint64_t traffic_bytes_;      /*!< bytes of traffic */
      SingleRing ring_pass_through_;         /*!< Counter->Worker direction ring address, multiple */
      Buf      output_pass_through_[DEFAULT_MBUF_ARRAY_SIZE]; /*!< Counter -> Worker output buffer */
      SIZE     output_cnt_pass_through_;   /*!< Counter -> Worker count of output */

  };
  /** *****************************************************
   * @brief
   * CounterLog Worker Core\n
   * \n
   */
  class CoreCounterLogWorker: public CoreInterface{
  public:
      CoreCounterLogWorker(COREID,TXT,COREID);
      virtual ~CoreCounterLogWorker();
  public:
      virtual TXT   GetObjectName(void);
      virtual VALUE GetN(KEY);
      virtual TYPE GetType(void);
      virtual RETCD SetRingAddr(ORDER, void*);
      virtual RETCD Cycle(void*,uint64_t*);
      virtual RETCD VirtualCycle(void*,uint64_t*);
      virtual RETCD BeforeCycle(void*);
      virtual RETCD AfterCycle(void*);
  private:
      Buf             input_int_[DEFAULT_MBUF_ARRAY_SIZE];  /*!< input buffer(for internal ring use only) */
      unsigned        cpuid_;
      uint64_t        transfer_count_;  /*!< count of transfer(counter logging) */
      void*           dbhandle_;        /*!< database handle */
      std::string     dburi_;           /*!< dburi */
  };


  /** *****************************************************
   * @brief
   * transfer Worker Core\n
   * \n
   */
  class CoreTransferWorker: public CoreInterface{
  public:
      CoreTransferWorker(COREID,TXT,unsigned,TXT,unsigned,unsigned);
      virtual ~CoreTransferWorker();
  public:
      virtual TXT   GetObjectName(void);
      virtual VALUE GetN(KEY);
      virtual TYPE GetType(void);
      virtual RETCD SetRingAddr(ORDER, void*);
      virtual RETCD Cycle(void*,uint64_t*);
      virtual RETCD VirtualCycle(void*,uint64_t*);
  private:
      RETCD Send(void*,unsigned);
  private:
      Buf             input_int_[DEFAULT_MBUF_ARRAY_SIZE];  /*!< input buffer(for internal ring use only) */
      unsigned        diameteripv4_;
      unsigned        quit_;
      unsigned        cpuid_;
      int             sock_;
      pthread_t       thread_;
      uint64_t        transfer_count_;  /*!< count of transfer */
      uint64_t        transfer_bytes_;  /*!< bytes transfered(tcp) */
      uint64_t        drop_bytes_;      /*!< bytes dropped(tcp) */
  };

  /** *****************************************************
   * @brief
   * PGW Worker Core base\n
   * Common implementations Ingress, Egress\n
   */
  //
  typedef int (*OnFoundWithUpdateLock)(lookup_ptr, void*);
  //
  class CorePgwBaseWorker: public CoreInterface{
  public:
      CorePgwBaseWorker(COREID,TXT,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned);
      virtual ~CorePgwBaseWorker();
  public:
      virtual void SetN(KEY,VALUE);
      virtual VALUE GetN(KEY);
      virtual RETCD SetRingAddr(ORDER, void*);
      virtual RETCD BeforeCycle(void*);
      virtual RETCD Cycle(void*,uint64_t*);
      virtual RETCD VirtualCycle(void*,uint64_t*);
#ifdef __TEST_MODE__
  public:
#else
  protected:
#endif
      RETCD Find(uint32_t, lookup_ptr);
      RETCD Find(uint32_t, OnFoundWithUpdateLock, void*);
      virtual RETCD ModifyPacket(struct rte_mbuf*) = 0;
  protected:
      uint64_t      warmup_count_;    /*!< count of warmpu */
      unsigned      quit_;            /*!< flag of quiet */
      unsigned      cpuid_;           /*!< binlog-side core */
      unsigned      greipv4_;         /*!< ipv4 address of GRE termination */
      unsigned      gresrcipv4_;      /*!< source ipv4 address of GRE termination */
      unsigned      groupid_;         /*!< group id */
      unsigned      pgwid_;           /*!< PGW id */
      unsigned      findtype_;        /*!< find type */
      unsigned      coretype_;        /*!< core type */
      unsigned      serverid_;        /*!< server id */
      unsigned      findkey_;         /*!< find key in temporary */
      void*         binlog_;          /*!< mysql binlog, main interface */
      void*         lookup_;          /*!< lookup pointer */
      void*         map_;             /*!< hashmap */
      pthread_mutex_t mapmtx_;        /*!< hashmap, sync object(mutex mode)  */
      rte_spinlock_t maplock_;        /*!< hashmap, sync object */
      SingleRing    ring_err_ind_;    /*!< ingress -> tap direction single ring */
      SingleRing    ring_warmup_;     /*!< tap -> ingress direction single ring */
      Buf           input_warmup_[DEFAULT_MBUF_ARRAY_SIZE]; /*!< warmup input buffer */
      std::string   dburi_;           /*!< connection string */
      mbuf_userdat_ptr udat_;         /*!< user data  */
      lookup_t      lookup_itm_;      /*!< lookup item  */
      unsigned      now_;             /*!< current epoch */
      unsigned      policer_cir_;     /*!< policer CIR */
      unsigned      policer_cbs_;     /*!< policer CBS */
      unsigned      policer_eir_;     /*!< policer EIR */
      unsigned      policer_ebs_;     /*!< policer EBS */
  };

  /** *****************************************************
   * @brief
   * PGW Worker Core ingress \n
   * \n
   */
  //
  class CorePgwIngressWorker: public CorePgwBaseWorker{
  public:
      CorePgwIngressWorker(COREID,TXT,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned);
      virtual ~CorePgwIngressWorker();
  public:
      virtual TXT  GetObjectName(void);
      virtual TYPE GetType(void);
  protected:
      virtual RETCD ModifyPacket(struct rte_mbuf*);
  };
  /** *****************************************************
   * @brief
   * PGW Worker Core egress \n
   * \n
   */
  //
  class CorePgwEgressWorker: public CorePgwBaseWorker{
  public:
      CorePgwEgressWorker(COREID,TXT,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned);
      virtual ~CorePgwEgressWorker();
  public:
      virtual TXT  GetObjectName(void);
      virtual TYPE GetType(void);
  protected:
      virtual RETCD ModifyPacket(struct rte_mbuf*);
  };

  /** *****************************************************
   * @brief
   * PGW Worker Core Distributor ingress \n
   * \n
   */
  //
  class CorePgwIngressDistributorWorker: public CorePgwBaseWorker{
  public:
      CorePgwIngressDistributorWorker(COREID,TXT,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned);
      virtual ~CorePgwIngressDistributorWorker();
  public:
      virtual TXT  GetObjectName(void);
      virtual TYPE GetType(void);
  protected:
      virtual RETCD ModifyPacket(struct rte_mbuf*);
  };
  /** *****************************************************
   * @brief
   * PGW Worker Core Distributor egress \n
   * \n
   */
  //
  class CorePgwEgressDistributorWorker: public CorePgwBaseWorker{
  public:
      CorePgwEgressDistributorWorker(COREID,TXT,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned);
      virtual ~CorePgwEgressDistributorWorker();
  public:
      virtual TXT  GetObjectName(void);
      virtual TYPE GetType(void);
  protected:
      virtual RETCD ModifyPacket(struct rte_mbuf*);
  };
  /** *****************************************************
   * @brief
   * Distributor-Rx Core\n
   * \n
   */
  class CoreRxDistributor: public CoreInterface{
  public:
      CoreRxDistributor(PORTID,QUEUEID,COREID);
      virtual ~CoreRxDistributor(){}
  public:
      virtual TXT   GetObjectName(void);
      virtual TYPE  GetType(void);
      virtual RETCD SetRingAddr(ORDER, void*);
      virtual RETCD BeforeCycle(void*);
      virtual RETCD Cycle(void*,uint64_t*);
  private:
      void BurstFlushDistributeRx(void*);
      void SendOrBufferDistributeRx(void* , unsigned, struct rte_mbuf*);
      void SendTapInterface(void*, struct rte_mbuf*);
  private:
#ifdef __TEST_MODE__
      SingleRing  ring_test_from_;                                            /*!< input packet ring(for test compile) */
#endif
      Ring        ring_rx_;                                                   /*!< Rx -> Worker direction ring address, multiple */
      Buf         output_rx_[DEFAULT_MBUF_OUT_SIZE][DEFAULT_MBUF_ARRAY_SIZE]; /*!< Rx -> Worker output buffer */
      SIZE        output_cnt_rx_[DEFAULT_MBUF_OUT_SIZE];                      /*!< Rx -> Worker output count */
      SingleRing  ring_tap_tx_;                                               /*!< Rx -> tap direction single ring */
      Buf         input_tap_tx_[DEFAULT_MBUF_ARRAY_SIZE];                     /*!< Rx -> tap input buffer */
  };

  /** *****************************************************
  * @brief
  * PGW Worker Core ingress tunnel \n
  * \n
  */
  //
  class CorePgwIngressTunnelWorker: public CorePgwBaseWorker{
  public:
      CorePgwIngressTunnelWorker(COREID,TXT,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned);
      virtual ~CorePgwIngressTunnelWorker();
  public:
      virtual TXT  GetObjectName(void);
      virtual TYPE GetType(void);
  protected:
      virtual RETCD ModifyPacket(struct rte_mbuf*);
  };
  /** *****************************************************
   * @brief
   * PGW Worker Core egress tunnel \n
   * \n
   */
  //
  class CorePgwEgressTunnelWorker: public CorePgwBaseWorker{
  public:
      CorePgwEgressTunnelWorker(COREID,TXT,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned);
      virtual ~CorePgwEgressTunnelWorker();
  public:
      virtual TXT  GetObjectName(void);
      virtual TYPE GetType(void);
  protected:
      virtual RETCD ModifyPacket(struct rte_mbuf*);
      struct gtpu_hdr*  gtpuh_; /*!< default packet : gtpu header */
      struct udp_hdr*   udph_;  /*!< default packet : udp header */
      struct ipv4_hdr*  iph_;   /*!< default packet : ip header */
  };

/** *****************************************************
 * @brief
 * Tap Core Base\n
 * tap interface Core\n
 */
  class CoreTapBase{
  public:
      static RETCD SockCtl(int , void* );
      static RETCD UtapOpen(TXT);
  private:
      CoreTapBase();
  };


  /** *****************************************************
   * @brief
   * Tap Tx Core\n
   * forward packet to tap interface\n
   */
  class CoreTapTx: public CoreInterface{
  public:
      CoreTapTx(TXT,COREID,CoreMempool*,int);
      virtual ~CoreTapTx();
  public:
      virtual TXT  GetObjectName(void);
      virtual TYPE GetType(void);
      virtual RETCD SetRingAddr(ORDER, void*);
      virtual void SetP(KEY,void*);
      virtual RETCD Cycle(void*,uint64_t*);
  private:
      Buf   input_int_[DEFAULT_MBUF_ARRAY_SIZE];  /*!< input buffer */
      int       tap_fd_;               /*!< tap interface */
      SingleRing ring_tap_ingress_[TAPDUPLICTE_MAX];     /*!< ring address,multiple, ingress-side */
      SingleRing ring_tap_egress_[TAPDUPLICTE_MAX];      /*!< ring address,multiple, egress-side */
      SIZE      ingress_ring_count_;   /*!< count of ingress-side ring */
      SIZE      egress_ring_count_;    /*!< count of egress-side ring */
      uint64_t  cur_cycle_;            /*!< count of cycle */
      uint64_t  cur_count_;            /*!< count of packet processed */
      CoreMempool* mpl_;               /*!< memory pooling reference interface */
  };

  /** *****************************************************
   * @brief
   * TapRx Core\n
   * packet forwarding by tap interface\n
   */
  class CoreTapRx: public CoreInterface{
  public:
      CoreTapRx(TXT,COREID,CoreMempool*);
      virtual ~CoreTapRx();
  public:
      virtual RETCD BeforeCycle(void*);
      virtual TXT  GetObjectName(void);
      virtual TYPE GetType(void);
      virtual RETCD SetRingAddr(ORDER, void*);
      virtual void SetP(KEY,void*);
      virtual VALUE GetN(KEY);
      virtual RETCD Cycle(void*,uint64_t*);
  protected:
      char      tap_nm_[MIXIPGW_IFNAMSIZ];
      int       tap_fd_;               /*!< tap interface */
      SingleRing ring_test_from_;      /*!< input packet ring(for test compile) */
      SingleRing ring_tap_ingress_[TAPDUPLICTE_MAX];     /*!< ring address, multiple ingress-side */
      SingleRing ring_tap_egress_[TAPDUPLICTE_MAX];      /*!< ring address, multiple egress-side */
      SIZE      ingress_ring_count_;   /*!< count of ring(ingress-side) */
      SIZE      egress_ring_count_;    /*!< count of ring(egress-side) */
      CoreMempool* mpl_;               /*!< memory pooling reference interface */
  };

  /** *****************************************************
   * @brief
   * counter ingress Worker Core\n
   * \n
   */
  class CoreCounterIngressWorker: public CoreCounterWorker{
  public:
      CoreCounterIngressWorker(COREID);
      virtual ~CoreCounterIngressWorker();
  public:
      virtual RETCD Cycle(void*,uint64_t*);
      virtual TYPE GetType(void);
  };

  /** *****************************************************
   * @brief
   * counter egress Worker Core\n
   * \n
   */
  class CoreCounterEgressWorker: public CoreCounterWorker{
  public:
      CoreCounterEgressWorker(COREID);
      virtual ~CoreCounterEgressWorker();
  public:
      virtual RETCD Cycle(void*,uint64_t*);
      virtual TYPE GetType(void);
  };

}; // namespace MIXIPGW

#endif //__MIXIPGW_CORE_HPP
