//
// Created by mixi on 2017/01/10.
//

#ifndef MIXIPGW_TOOLS_DEF_H
#define MIXIPGW_TOOLS_DEF_H

#define CHECK_PERF
#undef  CHECK_PERF
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <sched.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <net/if.h>

#include <net/ethernet.h>
#include <netinet/tcp.h>
#ifndef __FAVOR_BSD
#define __FAVOR_BSD
#endif
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp6.h>
#include <netinet/if_ether.h>
#include <net/if_arp.h>
#include <pthread.h>
#include <ifaddrs.h>

#ifndef __APPLE__
#include <linux/if_ether.h>
#include <linux/sockios.h>
#else
#include <libgen.h>
#include <ifaddrs.h>
#include <net/if_dl.h>
#endif
#ifndef IFT_ETHER
#define IFT_ETHER 0x6/* Ethernet CSMACD */
#endif

#ifndef __u8
typedef unsigned char __u8;
#endif
#ifndef __u32
typedef uint32_t __u32;
#endif

#ifndef __be16
typedef uint16_t __be16;
#endif
#ifndef __be32
typedef uint32_t __be32;
#endif

#ifdef __APPLE__
  #ifndef FMT_LLU
    #define FMT_LLU     "%llu"
  #endif
  #ifndef LLU
    #define LLU         "llu"
  #endif
#else
  #ifndef FMT_LLU
    #define FMT_LLU     "%lu"
  #endif
  #ifndef LLU
    #define LLU         "lu"
  #endif
#endif

#ifndef IPTOS_ECN
#define IPTOS_ECN(x)    ((x) & IPTOS_ECN_MASK)
#endif
#ifndef IPTOS_DSCP
#define IPTOS_DSCP(x)   ((x) & 0xfc)
#endif


#ifndef IPTOS_ECN_NOTECT
#define IPTOS_ECN_NOTECT 0x00	/* not-ECT */
#endif
#ifndef IPTOS_ECN_ECT1
#define	IPTOS_ECN_ECT1		0x01	/* ECN-capable transport (1) */
#endif
#ifndef IPTOS_ECN_ECT0
#define	IPTOS_ECN_ECT0		0x02	/* ECN-capable transport (0) */
#endif
#ifndef IPTOS_ECN_CE
#define	IPTOS_ECN_CE		0x03	/* congestion experienced */
#endif
#ifndef IPTOS_ECN_MASK 
#define	IPTOS_ECN_MASK		0x03	/* ECN field mask */
#endif






#define GTPU_PORT           2152
//#define _DBG_GTPU_         50000
#define _DBG_GTPU_             0
#define GTPC_PORT           2123
#define BFDC_PORT           3784
#define GTPLBMX_CTRL_PORT       10219

#define IS_RETOK(x) (x==RETOK?1:0)
#define IS_RETRELAY(x) (x==RETRELAY?1:0)
#define RETRELAY            (1)
#define RETOK               (0)
#define RETERR              (-1)
#define RETWRN              (1)
#define RETFOUND            (RETWRN)
#define RETNEXT             (RETOK)
#define RETIPV4             (RETOK)
#define RETIPV6             (RETWRN)
#define RETPTARP            (1)
#define RETPTICMP           (2)
#define RETPTGTPU           (3)
#define RETPTOVERIP         (4)
#define RETPTCTRL           (5)
#define RETPTGTPUECHOREQ    (6)
#define RETPTBFD            (7)
#define RETPTGTPC           (8)
#define RETPTBFDC           (9)

#define OWNER_ME    (1)
#define NOT_ME      (0)

#define MF_ORDER_DECAP      (0x00000001)
#define MF_ORDER_ENCAP      (0x00000002)
#define MF_ORDER_INGRESS    (0x00000004)
#define MF_ORDER_EGRESS     (0x00000008)
#define MF_BFD_UP           (0x10000000)
#define MF_BFD_DOWN         (0x20000000)
#define MF_PREPARED_CMP     (0x01000000)
#define MF_BOUNDARY_1MB     (0x02000000)

#define MF_IS_DECAP(x)      ((x&MF_ORDER_DECAP)==MF_ORDER_DECAP?1:0)
#define MF_IS_ENCAP(x)      ((x&MF_ORDER_ENCAP)==MF_ORDER_ENCAP?1:0)
#define MF_IS_INGRESS(x)    ((x&MF_ORDER_INGRESS)==MF_ORDER_INGRESS?1:0)
#define MF_IS_EGRESS(x)     ((x&MF_ORDER_EGRESS)==MF_ORDER_EGRESS?1:0)
#define MF_BFD_SET_UP(x)    {x&= ~MF_BFD_DOWN;x|= MF_BFD_UP;}
#define MF_BFD_SET_DOWN(x)  {x&= ~MF_BFD_UP;x|= MF_BFD_DOWN;}


#ifndef INVALID_SOCKET
#define INVALID_SOCKET  (0xffffffff)
#endif
#define BURST_SIZE      (8192)
#define ENABLE_ZEROCOPY (1)
#define WAIT_LINKSEC    (4)
#ifndef MIN
# define MIN(a,b) (a<b?a:b)
#endif
#ifndef MAX
# define MAX(a,b) (a>b?a:b)
#endif
#ifndef ULONGLONG
#define ULONGLONG unsigned long long
#endif
#ifndef LONGLONG
#define LONGLONG long long
#endif
#ifndef ULONG
#define ULONG unsigned long
#endif
#ifndef USHORT
#define USHORT unsigned short
#endif
#ifndef U8
#define U8 unsigned char
#endif

#define HTON24(x)  ((htonl(x)>>8)&0xffffff)
#define NTOH24(x)  (ntohl(x)&0xffffff)

#ifndef HTONLL
#ifndef htonll
#define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) ((1==ntohl(1)) ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))
#endif
#define	HTONLL(x)  (htonll((__uint64_t)x))
#define	NTOHLL(x)  (ntohll((__uint64_t)x))
#endif

#define MAKE_ULL(hi,lo) ((((ULONGLONG)((ULONG)hi & 0xffffffff))<<32 )|((ULONGLONG)((ULONG)lo & 0xffffffff)))
#define MAKE_UL(hi,lo) ((((ULONG)((USHORT)hi & 0xffff))<<16 )|((ULONG)((USHORT)lo & 0xffff)))
#define MAKE_DIGIT(hi,lo) ((((U8)((U8)hi & 0x0f))<<4 )|((U8)((U8)lo & 0x0f)))

// processed flag
#define PROC_RX_DROP            (0x00000000)
#define PROC_NEED_SEND          (0x00000001)
#define PROC_SWAP_BANDWITDH     (0x00000002)
#define PROC_TRIGGER_1S_EVENT   (0x00000004)
#define PROC_RFC4115_RED        (0x00000010)

// eventstate flag
#define EVENTSTAT_SWAP_NEXTTM   (0x00000001)
#define EVENTSTTA_BFD_RCV       (0x00000002)

// process ctrl type
#define PROCCTRL_SWAP_TBL       (0x00000001)
#define PROCCTRL_INSERT_ITM     (0x00000002)
#define PROCCTRL_DELETE_ITM     (0x00000004)
#define PROCCTRL_BASETEID       (0x00000008)
#define PROCCTRL_BASERATE       (0x00000010)
#define PROCCTRL_TRGTRATE       (0x00000020)

#define PROCCTRLR_ERR           (0x10000000)
#define PROCCTRLR_OK            (0x20000000)
#define LEN_VLAN(e)             (e->ether_type==htons(ETHERTYPE_VLAN)?((*(((u_short*)(e+1))+1))==htons(ETHERTYPE_VLAN)?8:4):0)
#define USEPIPEMODE

namespace MIXIPGW_TOOLS{

  #ifndef IPV6_LEN
  #define IPV6_LEN (16)
  #endif
//#define __WITHOUT_ECM__

#ifdef __WITHOUT_ECM__
  #define MAKE_NEXT_HOP(c,f)      (((c&0xFF0000FF)|((f&0xFF)<<16)))|(0x14<<8)
#else
  #define MAKE_NEXT_HOP(c,f)      (((c&0xFF00FFFF)|((f&0xFF)<<16)))
#endif
  #define FID_DC                  (0xff)
  #define FID_COUNTER_I           (0x00)
  #define FID_TRANS_I             (0x01)
  #define FID_TRANS_E             (0x02)
  #define FID_COUNTER_E           (0x03)
  #define FID_REFLECT             (0x04)
#ifdef __WITHOUT_ECM__ 
  #define FID_ECM                 (0x03)
#else
  #define FID_ECM                 (0x10)
#endif

  #define FID_NAT_I               (0x20)
  #define FID_SGW_E               (0x21)
  #define FID_ARBITOR_COUNTER_I   (0x40)
  #define FID_ARBITOR_TRANS_I     (0x41)

  #define MAKE_V6_FROM_V4(v6,v4)      (memcpy(&v6[IPV6_LEN-sizeof(__be32)],&v4,sizeof(__be32)))

//#define MAX_STRAGE_PER_GROUP    (16384)
  #define MAX_STRAGE_PER_GROUP    (65536)
  #define LINKMAPSTAT_OFF         (0)
  #define LINKMAPSTAT_ON          (1)
  #define LINKMAPTYPE_DC          (0)
  #define LINKMAPTYPE_UP          (1)
  #define LINKMAPTYPE_RM          (2)

  #define USE_SINGLE_QUEUE_NIC





  // notification stored
  typedef struct notification_item {
      __be32      teid;
      __be32      ipv4;
      uint64_t    ingress;
      uint64_t    egress;
  }notification_item_t,*notification_item_ptr;

#define NOTIFICATION_VERSION1           htonl(0x3)
  //
#define NOTIFICATION_ORDER_REQ          htonl(0x0)
#define NOTIFICATION_ORDER_ANS          htonl(0x1)
  //
#define NOTIFICATION_PROTO_FROM_DELIVER htonl(1)
#define NOTIFICATION_PROTO_INIT_MODULE  htonl(2)
#define NOTIFICATION_PROTO_POLL_DB      htonl(3)
#define NOTIFICATION_PROTO_INS_TEIDIP4  htonl(4)
#define NOTIFICATION_PROTO_DEL_TEIDIP4  htonl(5)
#define NOTIFICATION_PROTO_UPD_TEIDIP4  htonl(6)
#define NOTIFICATION_PROTO_SEL_TEIDIP4  htonl(7)
#define NOTIFICATION_PROTO_HALT         htonl(8)
#define NOTIFICATION_PROTO_DELETE_BEARER htonl(9)
#define NOTIFICATION_PROTO_DELAY        htonl(10)
#define NOTIFICATION_PAGESIZE   (32)

  // notification packed.
  typedef struct notification_item_packed {
      union _u {
          struct _flags{
              uint32_t order;         // request= 0/response = 1
              uint32_t version;       // version
              uint32_t proto;         //
              uint32_t error;         //
          }flags;
          uint32_t uflags;
      }u;
      char        reportid[NOTIFICATION_PAGESIZE];
      __be32      count;
      union _teid{
          __be32    teid[NOTIFICATION_PAGESIZE];
          __be32    teid_00;
      }teid;
      __be32      ipv4[NOTIFICATION_PAGESIZE];
      union _p {
          struct _traffic{
              uint64_t    ingress[NOTIFICATION_PAGESIZE];
              uint64_t    egress[NOTIFICATION_PAGESIZE];
          }traffic;
          struct _poll{
              __be32      teid_range_s[NOTIFICATION_PAGESIZE];
              __be32      teid_range_e[NOTIFICATION_PAGESIZE];
              __be32      lastupdate[NOTIFICATION_PAGESIZE];
              __be32      newupdate[NOTIFICATION_PAGESIZE];
          }poll;
          struct _rmbr{
              __be32      teid[NOTIFICATION_PAGESIZE];
              __be32      sgwip4[NOTIFICATION_PAGESIZE];
              __be16      sgwport[NOTIFICATION_PAGESIZE];
              __u8        ebi[NOTIFICATION_PAGESIZE];
          }rmbr;
      }p;
  }__attribute__ ((packed)) notification_item_packed_t,*notification_item_packed_ptr;


#define PCAPLEVEL_DC    (0)
#define PCAPLEVEL_MIN   (1)
#define PCAPLEVEL_ARP   (2)
#define PCAPLEVEL_ICMP  (3)
#define PCAPLEVEL_DBG   (4)
#define PCAPLEVEL_PARSE (5)

  // pcap
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

#define EH(p)   ((struct ether_header*)p->Header(PktInterface::ETHER))
#define IP(p)   ((struct ip*)p->Header(PktInterface::IP))
#define IPV6(p) ((struct ip6_hdr*)p->Header(PktInterface::IP))
#define ARP(p)  ((struct ether_arp*)p->Header(PktInterface::ARP))
#define UDP(p)  ((struct udphdr*)p->Header(PktInterface::UDP))
#define GTPU(p) ((gtpu_header_ptr)p->Header(PktInterface::GTPU))
#define BFD(p)  ((bfd_ptr)p->Header(PktInterface::BFD))
#define IP_I(p) ((struct ip*)p->Header(PktInterface::IP_I))
#define ICMP(p) ((struct icmp*)p->Header(PktInterface::ICMP))
#define ICMPV6(p) ((struct icmp6_hdr*)p->Header(PktInterface::ICMP))
#define ICMP_I(p) ((struct icmp*)p->Header(PktInterface::ICMP_I))
#define UDPP(p) ((char*)p->Header(PktInterface::GTPU))
#define PAYLOAD(p)  ((char*)p->Header(PktInterface::PAYLOAD))

#define CHK_ETHERTYPE(e,t)  (e->ether_type==htons(ETHERTYPE_VLAN)?\
    (*(((u_short*)(e+1))+1))==htons(ETHERTYPE_VLAN)?\
        (*(((u_short*)(e+1))+2))==htons(t):(*(((u_short*)(e+1))+1))==htons(t):e->ether_type==htons(t))

};// namespace MIXIPGW_TOOLS


#endif //MIXIPGW_TOOLS_DEF_H
