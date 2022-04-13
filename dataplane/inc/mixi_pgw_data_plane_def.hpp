/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       mixi_pgw_data_plane_def.h
    @brief      mixi_pgw_data_plane cpp common header
*******************************************************************************
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
#ifndef __MIXIPGW_DEF_HPP
#define __MIXIPGW_DEF_HPP

#include "mixi_pgw_data_plane_inc.h"
// stl
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

/*! @name basic defineded... */
/* @{ */
#define NIC_RXRING                  (1024)      /*!< NIC receive-side : default : ring size */
#define NIC_TXRING                  (1024)      /*!< NIC send-side    : default : ring size */
#define WORKER_RXRING               (4096)      /*!< Worker Core  : receive-side: default : ring size */
#define WORKER_TXRING               (4096)      /*!< Worker Core  : send-side   : default : ring size */
#define NIC_RXBURST                 (128)       /*!< NIC receive-side : default : burst size  */
#define NIC_TXBURST                 (128)       /*!< NIC send-side    : default : burst size  */
#define WORKER_RX_ENQ_BURST         (128)       /*!< RX Core : case enqueue default : burst size  */
#ifdef __DEBUG__    // no - numa
#define WORKER_RX_DEQ_BURST         (4)         /*!< Worker Core  : case dequeue default : burst size (compile with debug flags) */
#define WORKER_TX_ENQ_BURST         (4)         /*!< Worker Core  : case enqueue default : burst size (compile with debug flags) */
#define WORKER_TX_DEQ_BURST         (4)         /*!< TX Core  : case dequeue default : burst size (compile with debug flags) */
#else
#define WORKER_RX_DEQ_BURST         (16)        /*!< Worker Core  : case dequeue default : burst size  */
#define WORKER_TX_ENQ_BURST         (4)         /*!< Worker Core  : case enqueue default : burst size  */
#define WORKER_TX_DEQ_BURST         (4)         /*!< TX Core  : case dequeue default : burst size  */
#endif
#define _FLUSH_RX_                  (100000)    /*!< RX Core  : elapsed burst flush */
#define _FLUSH_TX_                  (100000)    /*!< TX Core  : elapsed burst flush */
#define _FLUSH_WORKER_              (100000)    /*!< Worker Core  : elapsed burst flush */
#define TAPDUPLICTE_MAX             (2)         /*!< TapRx warm-up duplicate count of max ring */
/* @} */

#define PGW_TUNNEL_IP               (0xdeadbeaf)    // xxx.xxx.xxx.xxx
#define PGW_TUNNEL_IP_S             (0xdeadc0de)    // xxx.xxx.xxx.xxx

#define SIXRD_ENCAPPER_ADDR         (0x0a0a0a0a)    // TODO SET

/*! @name memory defineded... */
/* @{ */
#define DEFAULT_MBUF_DATA_SIZE      (RTE_MBUF_DEFAULT_BUF_SIZE)     /*!< mbuf buffer size */
#if ARCH==16
#define DEFAULT_MEMPOOL_BUFFERS     (8192 *4)  /*!< memory pool size */
#else
#define DEFAULT_MEMPOOL_BUFFERS     (8192 *4)  /*!< memory pool size */
#endif

#define DEFAULT_MEMPOOL_CACHE_SIZE  (256)       /*!< memory pool cache size */
#define DEFAULT_MBUF_ARRAY_SIZE     (512)       /*!< max mbuf in burst */
#define DEFAULT_MBUF_OUT_SIZE       (16)        /*!< max connected rings to core */
#define PREFETCH_OFFSET	            (3)         /*!< prefetching offset */
/* @} */

/*! @name network/packet header defineded... */
/* @{ */
#define PROMISCUOUS                 (1)         /*!< promiscuous mode:ON */
#define GTPU_VERSION_1              (1)         /*!< GTPU version */
#define GTPU_PROTO_GTP              (1)         /*!< GTPU protocol */
#define GTPU_ERROR_INDICATION       (26)        /*!< GTPU error indication */
#define GTPU_TEIDI_TYPE             (16)        /*!< GTPU Tunnel Endpoint Identifier Data I  */
#define GTPU_PEER_ADDRESS           (133)       /*!< GTPU Peer Address */
#define GTPU_G_PDU                  (255)       /*!< GTPU gdu */
#define GTPU_PORT                   (2152)      /*!< GTPU number of udp port */
#define CHECK_INTERVAL              (100)       /*!< link-up check elapsed: 100ms */
#define MAX_CHECK_TIME              (90)        /*!< link-up check : 9s (90 * 100ms) in total */
#define GTPC_PRIVATE_EXT_SIZE       (32)        /*!< internal use gtpc item size */
#define GTPC_PRIVATE_EXT_MAGIC      (0xdeadbeaf)/*!< internal use gtpc item magic code */
#define GTPC_PRIVATE_EXT_VENDERID   (00000)     /*!< private mib number */

#ifndef GTPU_ECHO_REQ
#define GTPU_ECHO_REQ               (1)         /*!< gtpu echo request */
#endif
#ifndef GTPU_ECHO_RES
#define GTPU_ECHO_RES               (2)         /*!< gtpu echo response */
#endif
/* @} */

#define MAX_PACKET_SZ               (2048)
#define GTPC_PORT                   (2123)
#define GTPC_VERSION_2              (0x02)
#define GTPC_PIGGY_OFF              (0)
#ifndef MIXIPGW_IFNAMSIZ
#define MIXIPGW_IFNAMSIZ              (16)
#endif

#define GTPC_CREATE_SESSION_RES     (33)
#define GTPC_MODIFY_BEARER_RES      (35)
#define MAXPACKET_PER_SECOND        (1000000)     /*!< limit of host stack forwarding count.(1Mpps) */



/*! @name network/packet header defineded... */
/* @{ */
#ifndef THRESHOLD_TRANSFER
#define THRESHOLD_TRANSFER          (1*1024*1024) /*!< count aggregate values notify: threshold. */
#endif
/* @} */

/*! @name gropu id/value access */
/* @{ */
#define COUNT_TEIDGROUP_PER_CORE    (65536)                 /*!< count of stored teid, every 1 Worker Core */
#define PGWID(c)                    ((c&0x00f00000)>>20)    /*!< PGW id access */
#define GROUPID(c)                  ((c&0x000f0000)>>16)    /*!<  group id access */
#define GROUPVAL(c)                 (c &0x0000ffff)         /*!<  group value access */
#define PGWEGRESSPORT               (50999)                 /*!< PGW Egress udp port number */
/* @} */

/*! @name IPPROTO defineded... */
/* @{ */
#ifndef IPPROTO_OSPFIGP
#define IPPROTO_OSPFIGP             (89)
#endif
/* @} */

/*! @name counter defineded... */
/* @{ */
#define DCOUNTER_LIMIT              (10000)  /*!< ForwardTransfer, limit on number of users processed continuously.(depends on machine performance) */
/* @} */

/*! @name policer color defineded... */
/* @{ */
#define  POLICER_COLOR_GREEN    (0)
#define  POLICER_COLOR_YELLOW   (1)
#define  POLICER_COLOR_RED      (2)
/* @} */

/*! @name policer mode defineded... */
/* @{ */
#define  POLICER_MODE_LTE       (0)
#define  POLICER_MODE_3G        (1)
#define  POLICER_MODE_LOW       (2)
/* @} */

/*! @name policer parameters defineded... */
// referenced value :  => 128Kb/s
// https://www.theseus.fi/bitstream/handle/10024/110115/QoS%20Implementaion%20and%20Testing%20Anton%20Machakov%20Bachelor%20Thesis.pdf?sequence=1
/* @{ */
#define  POLICER_CIR_DEFAULT    (128)
#define  POLICER_CBS_DEFAULT    (12500)
#define  POLICER_EIR_DEFAULT    (128)
#define  POLICER_EBS_DEFAULT    (12500)
/* @} */


namespace MIXIPGW{
    /*! @struct gtpu_hdr
        @brief
        gtp-u header\n
        128 bits\n
    */
  typedef struct gtpu_hdr {	/* According to 3GPP TS 29.060. */
      union _u{
          struct _v1_flags{
              uint8_t npdu:1;
              uint8_t sequence:1;
              uint8_t extension:1;
              uint8_t reserve:1;
              uint8_t proto:1;
              uint8_t version:3;
          }v1_flags;
          uint8_t flags;
      }u;
      uint8_t     type;
      uint16_t    length;
      uint32_t    tid;
  } __attribute__ ((packed)) gtpu_hdr_t,*gtpu_hdr_ptr;

  /*! @struct gtpu_teid_i
    @brief
    tunnel endpoint identifier data i\n
    \n
   */
  typedef struct gtpu_teid_i{
      uint8_t       type;
      uint32_t      val;
  }__attribute__ ((packed)) gtpu_teid_i_t,*gtpu_teid_i_ptr;

  /*! @struct gtpu_peer_address
    @brief
    gtpu peer address\n
    \n
   */
  typedef struct gtpu_peer_address{
      uint8_t       type;
      uint16_t      length;
      uint32_t      val;
  }__attribute__ ((packed)) gtpu_peer_address_t,*gtpu_peer_address_ptr;

  /*! @struct gtpu_err_indication
    @brief
    Error Indication\n
    \n
   */
  typedef struct gtpu_err_indication{
      gtpu_teid_i_t         teid;
      gtpu_peer_address_t   peer;
  }__attribute__ ((packed)) gtpu_err_indication_t,*gtpu_err_indication_ptr;



  /*! @struct gtpc_hdr
      @brief
      gtp-c header\n
      128 bits\n
  */
  typedef struct gtpc_hdr {
      union _u{
          struct _v2_flags{
              uint8_t spare:3;
              uint8_t teid:1;
              uint8_t piggy:1;
              uint8_t version:3;
          }v2_flags;
          uint8_t flags;
      }u;
      uint8_t	type;
      uint16_t	length;
      union _t{
          uint32_t	teid;
          struct _sq{
              uint32_t seqno:24;
              uint32_t padd:8;
          }sq;
      }t;
      union _q{
          uint32_t    seqno;
          struct _sq_t{
              uint32_t seqno:24;
              uint32_t padd:8;
          }sq_t;
      }q;
  } __attribute__ ((packed)) gtpc_hdr_t,*gtpc_hdr_ptr;

  /*! @struct gtpc_inst
      @brief
      gtpc basic header  : instance\n
      \n
  */
  typedef struct gtpc_inst{
      uint8_t       instance:4;
      uint8_t       spare:4;
  }__attribute__ ((packed)) gtpc_inst_t,*gtpc_inst_ptr;

  /*! @struct gtpc_comm_header
      @brief
      gtpc basic header  : common header\n
      \n
  */
  typedef struct gtpc_comm_header{
      uint8_t       type;
      uint16_t      length;
      gtpc_inst_t   inst;
  }__attribute__ ((packed)) gtpc_comm_header_t,*gtpc_comm_header_ptr;

  /*! @enum PEID
    @brief
    gtpc private extension index\n
    1 index=32bit\n
  */
  enum GTPC_PEID{
      SGW_GTPU_IPV4 = 0,    /*!< sgw gtpu ipv4 */
      SGW_GTPU_TEID,        /*!< sgw gtpu teid */
      UE_IPV4,              /*!< ue ipv4 */
      UE_TEID,              /*!< ue teid */
      PGW_GTPU_IPV4,

      GTPC_PEID_MAX = 5     /*!< max index value:5x4=20 + 4(common header) + venderid(2) + padding(2) + 4(magic) = 32 */
  };

  /*! @struct private_extension_hdr
      @brief
      user extend item defined\n
      total=32 bytes fixed length\n
  */
  typedef struct private_extension_hdr{
      gtpc_comm_header_t    head;       /*!< common header */
      uint16_t              venderid;   /*!< vendor id */
      uint16_t              padd;       /*!< padding */
      uint32_t              magic;      /*!< magic */
      uint32_t              value[GTPC_PEID_MAX]; /*!< values */
  }__attribute__ ((packed)) private_extension_hdr_t,*private_extension_hdr_ptr;

  static_assert(sizeof(private_extension_hdr_t)==GTPC_PRIVATE_EXT_SIZE,   "must equol");

  typedef struct pcap_fheader{
      uint32_t    magic;
      uint16_t    major;
      uint16_t    minor;
      int32_t     zone;
      uint32_t    sig;
      uint32_t    snap;
      uint32_t    ltype;
  }__attribute__ ((packed)) pcap_fheader_t,*pcap_fheader_ptr;

  typedef struct pcap_pkthdr{
      uint32_t    tm_sec;
      uint32_t    tm_usec;
      uint32_t    pcaplen;
      uint32_t    len;
  }__attribute__ ((packed)) pcap_pkthdr_t,*pcap_pkthdr_ptr;

  enum MBFU_FLAG{
      MBFU_FLAG_DC = 0,
      MBFU_FLAG_PDU,
      MBFU_FLAG_MAX
  };

  typedef struct mbuf_userdat{
      union{
          struct{
              uint32_t    flag:8;
              uint32_t    size:12;
              uint32_t    color:2;
              uint32_t    mode:2;
              uint32_t    padd:8;
              uint32_t    teid;
          };
          uint64_t  userdata;
      };
  }__attribute__ ((packed)) mbuf_userdat_t,*mbuf_userdat_ptr;

  /*! @name mixi_pgw_data_plane typedef */
  /* @{ */
  typedef int       RETCD;      /*!< result code */
  typedef unsigned  VALUE;      /*!< key-value/value */
  typedef unsigned  COREID;     /*!< number of core */
  typedef unsigned  SOCKID;     /*!< number of socket(numa node) */
  typedef unsigned  PORTID;     /*!< number of port (dpdk-nic interface index) */
  typedef unsigned  QUEUEID;    /*!< number of queue (dpdk-nic queue index) */
  typedef unsigned  SIZE;       /*!< size any */
  typedef const char* DIR;      /*!< directory string */
  typedef const char* TXT;      /*!< any string */
  /* @} */

  /*! @enum TYPE
    @brief
    Core Type[RX,TX,WORKER]
  */
  enum TYPE{
      RX,           /*!< Receive Core */
      TX,           /*!< Transfer Core */
      WORKER,       /*!< Worker Core  */
      WORKER_TRANSFER, /*!< Worker Transfer */
      COUNTER,
      PGW_INGRESS,  /*!< PGW ingress */
      PGW_EGRESS,   /*!< PGW egress */
      TAPRX,        /*!< tap rx */
      TAPTX         /*!< tap tx */
  };
  /*! @enum ORDER
    @brief
    direction [FROM, TO]
  */
  enum ORDER{
      FROM,         /*!< source */
      FROM_00,      /*!< source : 1'st item(when managed multiple sources) */
      FROM_01,      /*!< destination : .. second. */
      TO,           /*!< destination */
      TO_00,        /*!< destination : 1'st item(when managed multiple destinations) */
      TO_01,        /*!< destination : .. second. */
      EXTEND        /*!< destination : extend */
  };
  /*! @enum KEY
    @brief
    key - value parameter
  */
  enum KEY{
      CNT_RX,       /*!< count of RX Cores */
      CNT_TX,       /*!< count of TX Cores */
      CNT_WORKER,   /*!< count of Worker Cores */
      BURST_FROM,   /*!< number of burst[from] */
      BURST_TO,     /*!< number of burst[to] */
      OPT,          /*!< option:any */
      LCORE,        /*!< core id */
      PORT,         /*!< port id */
      QUEUE,        /*!< queue id */
      CNT_PGW_INGRESS, /*!< pgw ingress count of cores */
      CNT_PGW_EGRESS,  /*!< pgw egress count of cores */
      FLUSH_DELAY,  /*!< flush delay */
      SOCKET,       /*!< socket id */
      POLICER_CIR,  /*!<  Commit Information Rate(Kb/s) */
      POLICER_CBS,  /*!<  Commit Burst Rate(B) */
      POLICER_EIR,  /*!<  Excess Information Rate(Kb/s) */
      POLICER_EBS,  /*!<  Excess Busrst Size(B) */
      RESERVED
  };
  /*! @enum PACKET_TYPE
    @brief
    packet type
  */
  enum PACKET_TYPE{
      NOTIP = 0,    /*!< except ip packet */
      PASS,         /*!< pass-through target packet(arp,internal icmp) */
      ENCAP,        /*!< encap packet */
      DECAP,        /*!< decap packet */
      ARP,          /*!< arp packet */
      VLAN,         /*!< vlan packet */
      MAX
  };
/**
   vlan flag on ether packet, get next protocol.\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]     eth_hdr   ether packet header
   @param[out]    proto     next protocol
   @return unsigned vlan length : execpt tag vlan==0
 */
  static inline unsigned get_vlan_offset(struct ether_hdr *eth_hdr, uint16_t *proto) {
    unsigned vlan_offset = 0;

    if (rte_cpu_to_be_16(ETHER_TYPE_VLAN) == *proto) {
      auto vlan_hdr   = (struct vlan_hdr *)(eth_hdr + 1);
      vlan_offset     = sizeof(struct vlan_hdr);
      (*proto)        = vlan_hdr->eth_proto;

      if (rte_cpu_to_be_16(ETHER_TYPE_VLAN) == *proto) {
        vlan_hdr    = (vlan_hdr + 1);
        (*proto)    = vlan_hdr->eth_proto;
        vlan_offset += sizeof(struct vlan_hdr);
      }
    }
    return vlan_offset;
  }
/**
   is arp , or internal icmp\n
   *******************************************************************************
   true  = pass-through packets without encapsulation.\n
   false = encapsulate\n
   *******************************************************************************
   @param[in]     m     mbuf address
   @return PACKET_TYPE packet type
 */
  static inline PACKET_TYPE is_arp_or_innericmp(struct rte_mbuf* m){
    auto eth        = rte_pktmbuf_mtod(m, struct ether_hdr *);
    //
    if (likely( ((RTE_PTYPE_L2_MASK & m->packet_type) == RTE_PTYPE_L2_ETHER))){
      ;;
    }else if ((RTE_PTYPE_L2_MASK & m->packet_type) == RTE_PTYPE_L2_ETHER_VLAN){   // VLAN
      return(PACKET_TYPE::VLAN);
    }else if ((RTE_PTYPE_L2_MASK & m->packet_type) != RTE_PTYPE_UNKNOWN){         // is ARP, LLDP
      return(PACKET_TYPE::PASS);
    }else{                                                                        // is RTE_PTYPE_UNKNOWN
      ;;
    }

    auto eth_type   = eth->ether_type;
    //auto vlan_off   = get_vlan_offset(eth, &eth_type);
    unsigned vlan_off = 0;
    //
    if (likely(eth_type == rte_cpu_to_be_16(ETHER_TYPE_IPv4))){
      auto ip4 = (struct ipv4_hdr*)((char*)(eth + 1) + vlan_off);
      if (ip4->next_proto_id == IPPROTO_TCP){
        auto tcp = (struct tcp_hdr*)((char*)(eth + 1) + ((ip4->version_ihl & 0x0f)<<2));
        if (unlikely(((rte_cpu_to_be_16(tcp->dst_port) == 179) || (rte_cpu_to_be_16(tcp->src_port) == 179)) && ((ip4->dst_addr & 0x000000ff) != 0x0a))){
          rte_log(RTE_LOG_DEBUG, RTE_LOGTYPE_USER3, "ebgp..(%08x) \n", ip4->dst_addr);
          return(PACKET_TYPE::PASS);
        }else {
          return(PACKET_TYPE::ENCAP);
        }
      }else if (ip4->next_proto_id == IPPROTO_UDP){
      auto udp = (struct udp_hdr*)((char*)(eth + 1) + ((ip4->version_ihl & 0x0f)<<2));
        if (unlikely((rte_cpu_to_be_16(udp->dst_port) == 3784) && (ip4->time_to_live == 255))){
          rte_log(RTE_LOG_DEBUG, RTE_LOGTYPE_USER3, "bfd..(%08x) \n", ip4->dst_addr);
          return(PACKET_TYPE::PASS);
        }else {
          return(PACKET_TYPE::ENCAP);
        }
      }else if (ip4->next_proto_id == IPPROTO_OSPFIGP){
        rte_log(RTE_LOG_INFO, RTE_LOGTYPE_USER3, "ospf..(%08x) \n", ip4->dst_addr);
        return(PACKET_TYPE::PASS);
      }else if (unlikely(ip4->next_proto_id == IPPROTO_ICMP)){
        rte_log(RTE_LOG_DEBUG, RTE_LOGTYPE_USER3, "icmp..(%08x) \n", ip4->dst_addr);
        if ((ip4->dst_addr & 0x000000ff) != 0x0a){
          rte_log(RTE_LOG_DEBUG, RTE_LOGTYPE_USER3, "icmp.. NOT user packet. passthrough(%08x)\n", ip4->dst_addr);
          return(PACKET_TYPE::PASS);
        }else{
          return(PACKET_TYPE::ENCAP);
        }
      }else {
        return(PACKET_TYPE::ENCAP);
      }
    }else if(eth_type == rte_cpu_to_be_16(ETHER_TYPE_IPv6)){
      auto ip6 = (struct ipv6_hdr*)((char*)(eth + 1) + vlan_off);
      if (unlikely(ip6->proto  == IPPROTO_ICMPV6)){
        rte_log(RTE_LOG_DEBUG, RTE_LOGTYPE_USER3, "icmp.v6.(%02x : %02x : %02x : %02x) \n",
                ip6->dst_addr[4],
                ip6->dst_addr[5],
                ip6->dst_addr[6],
                ip6->dst_addr[7]
               );
        // when icmpv6, hoplimit==0xff -> internal use -> pass through to next
        // else encapsulate/decapsulate
        if (ip6->hop_limits==0xff){
            rte_log(RTE_LOG_DEBUG, RTE_LOGTYPE_USER3, "inner icmp.v6.(%02x : %02x : %02x : %02x) \n",
                    ip6->dst_addr[4],
                    ip6->dst_addr[5],
                    ip6->dst_addr[6],
                    ip6->dst_addr[7]
                   );
          return(PACKET_TYPE::PASS);
        }else {
          return(PACKET_TYPE::ENCAP);
        }
      }else {
        return(PACKET_TYPE::ENCAP);
      }
    }else if (eth_type == rte_cpu_to_be_16(ETHER_TYPE_ARP)){
      rte_log(RTE_LOG_INFO, RTE_LOGTYPE_USER3, "packet type (ARP:%u)\n", eth_type);
      return(PACKET_TYPE::PASS);
    }else if (eth_type == rte_cpu_to_be_16(ETHER_TYPE_VLAN)){
      rte_log(RTE_LOG_INFO, RTE_LOGTYPE_USER3, "packet type (VLAN:%u)\n", eth_type);
      return(PACKET_TYPE::VLAN);
    }else {
      rte_log(RTE_LOG_INFO, RTE_LOGTYPE_USER3, "packet type (ANOTHER_ETHER_TYPE:%u)\n", eth_type);
      return(PACKET_TYPE::PASS);
    }
  }

/**
   is gtpu->type == 255 (Ingress user packet)\n
   *******************************************************************************
   PACKET_TYPE::PASS  = pass through packet without decapsulate\n
   PACKET_TYPE::DECAP = decapsulate\n
   *******************************************************************************
   @param[in]     m     mbuf address
   @return PACKET_TYPE packet type
 */
    static inline PACKET_TYPE is_pdu(struct rte_mbuf* m){
        auto eth = rte_pktmbuf_mtod(m, struct ether_hdr*);
        if (eth->ether_type == rte_cpu_to_be_16(ETHER_TYPE_IPv4)){
            auto ip4 = (struct ipv4_hdr*)(eth + 1);
            if (ip4->next_proto_id == IPPROTO_UDP){
                auto udp = (struct udp_hdr*)(ip4 + 1);
                if (udp->dst_port == htons(GTPU_PORT)){
                    auto gtpu = (struct gtpu_hdr*)(udp +1);
                    if (gtpu->type == GTPU_G_PDU){
                        return PACKET_TYPE::DECAP;
                    }
                }
            }
        }
        return PACKET_TYPE::PASS;
    }

/**
   is ip->dst_addr & 0x000000ff == 0x0000000a (Egress user packet)\n
   *******************************************************************************
   PACKET_TYPE::PASS  = pass-through packet without encapsulate.\n
   PACKET_TYPE::ENCAP = encapsulate.\n
   *******************************************************************************
   @param[in]     m     mbuf address
   @return PACKET_TYPE packet type
 */
    static inline PACKET_TYPE is_user(struct rte_mbuf* m){
        auto eth = rte_pktmbuf_mtod(m, struct ether_hdr *);
        if (eth->ether_type == rte_cpu_to_be_16(ETHER_TYPE_IPv4)){
            auto ip4 = (struct ipv4_hdr*)(eth + 1);
            if ((ip4->dst_addr & 0x000000ff) == 0x0000000a) {
                return PACKET_TYPE::ENCAP;
            }
        }
        return PACKET_TYPE::PASS;
    }

/**
   GretermWorker , is target\n
   *******************************************************************************
   true  = pass-through packet without decapsulate.\n
   false = decapsulate.\n
   *******************************************************************************
   @param[in]     m     mbuf address
   @return PACKET_TYPE packet type
 */
  static inline PACKET_TYPE check_decapability(struct rte_mbuf* m){
    auto eth        = rte_pktmbuf_mtod(m, struct ether_hdr *);
    //
    if (likely( ((RTE_PTYPE_L2_MASK & m->packet_type) == RTE_PTYPE_L2_ETHER))){
      ;;
    }else if ((RTE_PTYPE_L2_MASK & m->packet_type) == RTE_PTYPE_L2_ETHER_VLAN){   // VLAN
      return(PACKET_TYPE::VLAN);
    }else if ((RTE_PTYPE_L2_MASK & m->packet_type) != RTE_PTYPE_UNKNOWN){         // is ARP, or LLDP
      return(PACKET_TYPE::PASS);
    }else{                                                                        // is RTE_PTYPE_UNKNOWN
      ;;
    }

    auto eth_type   = eth->ether_type;
    //auto vlan_off   = get_vlan_offset(eth, &eth_type);
    unsigned vlan_off = 0;
    //
    if (likely(eth_type == rte_cpu_to_be_16(ETHER_TYPE_IPv4))){
      auto ip4 = (struct ipv4_hdr*)((char*)(eth + 1) + vlan_off);
      if (ip4->next_proto_id == IPPROTO_UDP){
#ifdef __CANNOT_USE_GRE__
      auto udp = (struct udp_hdr*)((char*)(eth + 1) + ((ip4->version_ihl & 0x0f)<<2));
        if (unlikely((rte_cpu_to_be_16(udp->dst_port) == 2152))){ // gtp
          return(PACKET_TYPE::DECAP);
        }else {
          rte_log(RTE_LOG_INFO, RTE_LOGTYPE_USER3, "not decap... udp.dstport != 2152 (%u)\n", rte_cpu_to_be_16(udp->dst_port));
          return(PACKET_TYPE::PASS);
        }
#else
      }else if (ip4->next_proto_id == IPPROTO_GRE){
        return(PACKET_TYPE::DECAP);
#endif
      }else {
        rte_log(RTE_LOG_INFO, RTE_LOGTYPE_USER3, "not decap... ipv4 next protocol (%u)\n", ip4->next_proto_id);
        return(PACKET_TYPE::PASS);
      }
    }else if (eth_type == rte_cpu_to_be_16(ETHER_TYPE_IPv6)){
      rte_log(RTE_LOG_INFO, RTE_LOGTYPE_USER3, "not decap... packet type (IPv6:%u)\n", eth_type);
      return(PACKET_TYPE::PASS);
    }else if (eth_type == rte_cpu_to_be_16(ETHER_TYPE_ARP)){
      rte_log(RTE_LOG_INFO, RTE_LOGTYPE_USER3, "not decap... packet type (ARP:%u)\n", eth_type);
      return(PACKET_TYPE::PASS);
    }else if (eth_type == rte_cpu_to_be_16(ETHER_TYPE_VLAN)){
      rte_log(RTE_LOG_INFO, RTE_LOGTYPE_USER3, "not decap... packet type (VLAN:%u)\n", eth_type);
      return(PACKET_TYPE::VLAN);
    }else {
      rte_log(RTE_LOG_INFO, RTE_LOGTYPE_USER3, "not decap... packet type (ANOTHER_ETHER_TYPE:%u)\n", eth_type);
      return(PACKET_TYPE::PASS);
    }
  }
/**
   split string\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]       src         source string
   @param[in]       sep         separate chars
   @param[in/out]   splitted    splited array strings
   @return int 0!= error , 0==success 
 */
  static inline int str_split(const char* src, const char* sep,std::vector<std::string>& splitted){
      char*		finded = NULL;
      char*		tmpsep = NULL;
      char*		tmpsrc = NULL;
      char*		current = NULL;
      uint32_t	seplen;
      uint32_t	srclen;
      uint32_t	busyloop_counter = 0;
      std::string tmpstr("");
      //
      if (!src || !sep)		{ return(-1); }
      if (!strlen(src))		{ return(-1); }
      if (!strlen(sep))		{ return(-1); }
      //
      seplen	= strlen(sep);
      srclen	= strlen(src);
      //
      tmpsep	= (char*)malloc(seplen + 1);
      memset(tmpsep,0,(seplen + 1));
      memcpy(tmpsep,sep,seplen);
      //
      tmpsrc	= (char*)malloc(srclen + 1);
      memset(tmpsrc,0,(srclen + 1));
      memcpy(tmpsrc,src,srclen);
      //
      current	= tmpsrc;
      //
      while(true){
          // find separator
          if ((finded = strstr(current,tmpsep)) == NULL){
              splitted.push_back(current);
              break;
          }
          // first char is separator.
          if (finded == current){
              finded += seplen;
          }else{
              tmpstr.assign(current,(finded - current));
              splitted.push_back(tmpstr);
          }
          current += (finded - current);
          // end of string
          if (current >= (tmpsrc + srclen)){
              break;
          }
          busyloop_counter ++;
          if (busyloop_counter > 1000){
              return(-1);
          }
      }
      free(tmpsrc);
      free(tmpsep);
      //
      return(0);
  }
/**
   convert string(ip address formatted) to 32bit numeric\n
   *******************************************************************************
   lettle endian.\n
   *******************************************************************************
   @param[in]       src     source string
   @param[in/out]   dst     numeric
   @param[in/out]   len     IO buffer size
   @return int 0!= error , 0==success 
 */
  static inline int get_ipv4(const char* src, unsigned char* dst, unsigned* len){
      std::vector<std::string>              splitted;
      std::vector<std::string>::iterator    itr;
      int n;
      //
      if ((*len) < sizeof(uint32_t)){
          return(-1);
      }
      // ex.) 192.168.111.123
      if (str_split(src, ".", splitted) != 0){
          return(-1);
      }
      if (splitted.size() != 4){ return(-1); }
      //
      for(n = 0, itr = splitted.begin();itr != splitted.end();++itr,n++){
          dst[n] = (uint8_t)strtoul((*itr).c_str(),NULL,10);
      }
      (*len) = sizeof(uint32_t);
      return(0);
  }

/**
   is ipv[4/6] packet ip@ip_dst\n
   *******************************************************************************
   inline \n
   *******************************************************************************
   @param[in]     bf     packet buffer
   @return RETCD  0!=success,destination ip , 0==error
 */
  static inline uint32_t is_ipv(struct rte_mbuf* bf){
      if (RTE_ETH_IS_IPV4_HDR(bf->packet_type)) {
          auto ipv4 = rte_pktmbuf_mtod_offset(bf, struct ipv4_hdr*, sizeof(struct ether_hdr));
          return(ipv4->dst_addr);
      }else if (RTE_ETH_IS_IPV6_HDR(bf->packet_type)) {
          auto ipv6 = rte_pktmbuf_mtod_offset(bf, struct ipv6_hdr*, sizeof(struct ether_hdr));
          uint32_t tmp = 0;
          memcmp(&tmp, &ipv6->dst_addr[8], 4);
          return(tmp);
      }
      return(0);
  }

/**
   is udp packet -> udp-dstport\n
   *******************************************************************************
   inline \n
   *******************************************************************************
   @param[in]     bf     packet buffer
   @return RETCD  0!=success ,teid , 0==error
 */
  static inline uint16_t is_udp(struct rte_mbuf* bf){
      if (RTE_ETH_IS_IPV4_HDR(bf->packet_type)) {
          auto ipv4 = rte_pktmbuf_mtod_offset(bf, struct ipv4_hdr*, sizeof(struct ether_hdr));
          if (ipv4->next_proto_id == IPPROTO_UDP){
              auto udp = rte_pktmbuf_mtod_offset(bf, struct udp_hdr*, sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr));
              return(udp->dst_port);
          }
      }
      return(0);
  }
/**
   is gtpu -> Gtpu@teid\n
   *******************************************************************************
   inline \n
   *******************************************************************************
   @param[in]     bf     packet buffer
   @return RETCD  0!=success ,teid , 0==error
 */
  static inline uint32_t is_gtpu(struct rte_mbuf* bf){
      if (RTE_ETH_IS_IPV4_HDR(bf->packet_type)) {
          auto ipv4 = rte_pktmbuf_mtod_offset(bf, struct ipv4_hdr*, sizeof(struct ether_hdr));
          if (ipv4->next_proto_id == IPPROTO_UDP){
              auto udp = rte_pktmbuf_mtod_offset(bf, struct udp_hdr*, sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr));
              if (udp->dst_port == htons(GTPU_PORT)){
                  auto gtpu = rte_pktmbuf_mtod_offset(bf, struct gtpu_hdr*, sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr));
                  if (gtpu->type == GTPU_ECHO_REQ || gtpu->type == GTPU_ECHO_RES){
                      // is gtpu , not user payload.
                      return(0);
                  }
                  return(gtpu->tid);
              }
          }
      }
      return(0);
  }

/**
   is gtpc packet -> Gtpc\n
   *******************************************************************************
   inline \n
   *******************************************************************************
   @param[in]     bf     packet buffer
   @return RETCD  0!=success ,gtpc@type , 0==error
 */
  static inline uint32_t is_gtpc(struct rte_mbuf* bf){
      if (RTE_ETH_IS_IPV4_HDR(bf->packet_type)) {
          auto ipv4 = rte_pktmbuf_mtod_offset(bf, struct ipv4_hdr*, sizeof(struct ether_hdr));
          if (ipv4->next_proto_id == IPPROTO_UDP){
              auto udp = rte_pktmbuf_mtod_offset(bf, struct udp_hdr*, sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr));
              if (udp->dst_port == htons(GTPC_PORT)){
                  auto gtpc = rte_pktmbuf_mtod_offset(bf, struct gtpc_hdr*, sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr));
                  if (ntohs(gtpc->length) < 0 || ntohs(gtpc->length) > ETHER_MAX_LEN){
                      return(0);
                  }
                  if (gtpc->u.v2_flags.version != GTPC_VERSION_2 || gtpc->u.v2_flags.piggy != GTPC_PIGGY_OFF){
                      return(0);
                  }
                  return(gtpc->type);
              }
          }
      }
      return(0);
  }
  /**
   GTPU packet -> error indicate\n
   *******************************************************************************
   inline \n
   *******************************************************************************
   @param[in]     bf     packet buffer
   */
  static inline void error_indicate(struct rte_mbuf* bf){
      auto eh = rte_pktmbuf_mtod_offset(bf, struct ether_hdr*, 0);
      auto ip = rte_pktmbuf_mtod_offset(bf, struct ipv4_hdr*, sizeof(ether_hdr));
      auto udp= rte_pktmbuf_mtod_offset(bf, udp_hdr*, sizeof(ether_hdr) + sizeof(ipv4_hdr));
      auto gtph= rte_pktmbuf_mtod_offset(bf, struct gtpu_hdr*, sizeof(ether_hdr) + sizeof(ipv4_hdr) + sizeof(udp_hdr));
      auto errind_len = (sizeof(ether_hdr) + sizeof(ipv4_hdr) + sizeof(udp_hdr) + sizeof(gtpu_hdr) + sizeof(gtpu_err_indication_t));
      auto prev_len  = rte_pktmbuf_pkt_len(bf);
      //
      if (rte_pktmbuf_trim(bf, (prev_len - errind_len)) < 0){
          rte_panic("rte_pktmbuf_trim(%d: %s)\n", errno, strerror(errno));
      }
      bf->nb_segs = 1;
      bf->next = NULL;
      bf->pkt_len = (uint16_t)errind_len;
      bf->data_len = (uint16_t)errind_len;
      // swap mac
      auto t_macaddr = eh->d_addr;
      eh->d_addr = eh->s_addr;
      eh->s_addr = t_macaddr;
      // swap ip
      auto t_ipaddr = ip->dst_addr;
      ip->dst_addr = ip->src_addr;
      ip->src_addr = t_ipaddr;
      ip->total_length = htons(errind_len - sizeof(*eh));
      ip->hdr_checksum = 0;
      // swap udp port
      auto t_uport = udp->dst_port;
      udp->dst_port = udp->dst_port;
      udp->src_port = udp->dst_port;
      udp->dgram_len = htons(errind_len - sizeof(*eh) - sizeof(*ip));
      udp->dgram_cksum = 0;
      // error indicate set parameters
      auto eind = rte_pktmbuf_mtod_offset(bf, gtpu_err_indication_ptr, errind_len - sizeof(gtpu_err_indication_t));
      eind->teid.type = GTPU_TEIDI_TYPE;
      eind->teid.val  = gtph->tid;
      eind->peer.type = GTPU_PEER_ADDRESS;
      eind->peer.length = htons(sizeof(eind->peer.val));
      eind->peer.val  = t_ipaddr;
      // error indicator
      gtph->type = GTPU_ERROR_INDICATION;
      gtph->tid  = 0;
      gtph->length = htons(sizeof(gtpu_err_indication_t));
  }

  #define PGW_LOG_CORES  (0xFF)
  extern int PGW_LOG_LEVEL[PGW_LOG_CORES];
  #define PGW_LOG(l, ...)  PGW_LOG_(l, __func__, __VA_ARGS__, "dummy")
  #define PGW_LOG_(l, func, format, ...) __PGW_LOG(l, func, format "%.0s", __VA_ARGS__)
  static inline void __PGW_LOG(const int level, const char *funcname , const char *format, ...){
      char funcnm_bf[20] = {0};
      //
      if (level < RTE_LOG_ERR){
          char msg_bf[512] = {0};
          memcpy(funcnm_bf, funcname, MIN(strlen(funcname), sizeof(funcnm_bf)-1));
          va_list ap;
          va_start(ap, format);
          vsnprintf(msg_bf, sizeof(msg_bf)-1, format, ap);
          va_end(ap);
          if (level == RTE_LOG_CRIT){
              rte_log(level, RTE_LOGTYPE_USER3, "[CRITICAL]%s", msg_bf);
          }else if (level == RTE_LOG_ALERT){
              rte_log(level, RTE_LOGTYPE_USER3, "[ALERT]%s", msg_bf);
          }else if (level == RTE_LOG_EMERG){
              rte_log(level, RTE_LOGTYPE_USER3, "[EMERG]%s", msg_bf);
          }else{
              rte_log(level, RTE_LOGTYPE_USER3, "[----]%s", msg_bf);
          }
      }else if (level < PGW_LOG_LEVEL[rte_lcore_id()==LCORE_ID_ANY?0:rte_lcore_id()]){
          memcpy(funcnm_bf, funcname, MIN(strlen(funcname), sizeof(funcnm_bf)-1));
          va_list ap;
          va_start(ap, format);
          rte_vlog(level, RTE_LOGTYPE_USER3, format, ap);
          va_end(ap);
      }
  }

#ifndef __FILENAME__
  #define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif
  static unsigned _checksum(const void *data, unsigned short len, unsigned sum){
      unsigned  _sum   = sum;
      unsigned  _count = len;
      unsigned short* _addr  = (unsigned short*)data;
      while( _count > 1 ) {
          _sum += ntohs(*_addr);
          _addr++;
          _count -= 2;
      }
      if(_count > 0 ){
          _sum += ntohs(*_addr);
      }
      while (_sum>>16){
          _sum = (_sum & 0xffff) + (_sum >> 16);
      }
      return(~_sum);
  }
  static unsigned short _wrapsum(unsigned sum){
      sum = sum & 0xFFFF;
      return (htons(sum));
  }
  
  // pcap logger.
  static FILE* __pcap_handle = NULL;
  static uint64_t __pcap_counter = 0;
  static pthread_mutex_t __pcap_mutex;
  static void pcap_open(void){
      char f[128] = {0};
      snprintf(f, sizeof(f)-1, "/tmp/%u.pcap", getpid());
      __pcap_handle = fopen(f, "a+");
      if (__pcap_handle == NULL){
          rte_panic("cannot opne pcap file(%s)", f);
      }
  }
  static void pcap_append(int flag, const char* data, unsigned len){
      pcap_fheader_t fhead;
      pcap_pkthdr_t  pkth;
      if ((len + sizeof(pkth)) > ETHER_MAX_LEN){ return; }
      if (__pcap_counter == 0){
          bzero(&fhead, sizeof(fhead));
          fhead.magic = htonl(0xd4c3b2a1);
          fhead.major = 2;
          fhead.minor = 4;
          fhead.snap  = htonl(0x00000400);
          fhead.ltype = htonl(0x01000000);
          if (fwrite(&fhead, 1, sizeof(fhead), __pcap_handle) == 0){
              rte_panic("failed. fwrite. pcap file header.");
          }
      }
      __pcap_counter ++;
      pkth.tm_sec  = (uint32_t)(__pcap_counter>>32);
      pkth.tm_usec = (uint32_t)(__pcap_counter&0xffffffff);
      pkth.pcaplen = (uint32_t)(len);
      pkth.len     = (uint32_t)(len);
      if (fwrite(&pkth, 1, sizeof(pkth), __pcap_handle) == 0){
          rte_panic("failed. fwrite. pcap packet header.");
      }
      if (fwrite(data, 1, len, __pcap_handle) == 0){
          rte_panic("failed. fwrite. pcap packet payload.");
      }
      fflush(__pcap_handle); 
  }
  static void pcap_close(void){
      if (__pcap_handle != NULL){
          fclose(__pcap_handle);
      }
      __pcap_handle = NULL;
  }
  static inline void parse_mysql(const char* dburi, char* user_o, int len_user_o, char* pwd_o, int len_pwd_o, char* host_o, int len_host_o, unsigned int *port_o){
      if (!dburi || !user_o || !len_user_o || !pwd_o || !len_pwd_o || !host_o || !len_host_o || !port_o){
          printf("invalid arguments\n");
          return;
      }
      auto p = strchr(dburi, ':');
      if (p == 0){
          printf("invalid dburi (%s)\n", dburi);
          return;
      }
      if (*(p+1) != '/' || *(p+2) != '/'){
          printf("invalid dburi :// (%s)\n", dburi);
          return;
      }
      const char *user = (p + 3);
      const char *user_end= strpbrk(user, ":@");
      const char *const pass = user_end + 1;
      const char *pass_end = pass;
      if (*user_end == ':'){
          pass_end = strchr(pass, '@');
          if (pass_end == 0){
              printf("missing password (%s)\n", dburi);
              return;
          }
      }
      if (pass_end == pass){
          printf("missing password ..(%s)\n", dburi);
          return;
      }
      const char *host = *pass_end == '@' ? pass_end + 1 : pass_end;
      const char *host_end = strchr(host, ':');
      if (host == host_end){
          printf("missing host name :// (%s)\n", dburi);
          return;
      }
      if (host_end == 0){
          host_end = p+1 + strlen(p+1);
      }
      (*port_o) = 3306;
      if (*host_end == ':'){
          (*port_o) = strtoul(host_end + 1, NULL, 10);
      }
      //
      memcpy(user_o, user, MIN((user_end - user), len_user_o));
      memcpy(pwd_o,  pass, MIN((pass_end - pass), len_pwd_o));
      memcpy(host_o, host, MIN((host_end - host), len_host_o));
  }
};
/* @} */

#endif //__MIXIPGW_DEF_HPP
