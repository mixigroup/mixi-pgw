#ifndef DATAPLANE_STRS_H
#define DATAPLANE_STRS_H
#ifndef _GNU_SOURCE
#define _GNU_SOURCE     /* for CPU_SET() */
#endif
#include <stdio.h>
#define NETMAP_WITH_LIBS
#include <net/netmap_user.h>


#include <ctype.h>      // isprint()
#include <unistd.h>     // sysconf()
#include <sys/poll.h>
#include <arpa/inet.h>  /* ntohs */
#include <sys/sysctl.h> /* sysctl */
#include <ifaddrs.h>    /* getifaddrs */
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/ip6.h>

#ifndef __APPLE__
#define IPV6_VERSION    0x60
#define IPV6_DEFHLIM    64
#endif
#include <assert.h>
#include <math.h>

#include <pthread.h>

// defined.
#define MAX_BODYSIZE	(16384)
#define UNUSED(x)       (void)x
#ifndef MIN
#define MIN(a,b) (a<b?a:b)
#endif
#define TEST_ECN_STRS   (0x02)
//#define __IPV6__
typedef struct pkt {
    struct ether_header eh;
    uint16_t vlanid;
    uint16_t etype;
#ifndef __IPV6__
    struct ip ip;
#else
    struct ip6_hdr ip;
#endif
    struct udphdr udp;
    uint8_t body[MAX_BODYSIZE];
    uint32_t size;
} __attribute__((__packed__)) pkt_t, *pkt_ptr;




typedef struct observe {
    uint64_t   pkts,bytes,events,drop,min_space;
    struct timeval tm;
}observe_t,*observe_ptr;


typedef void* (*thread_func)(void*);

typedef struct thread_arg{
    int     id;
    int     fd;
    int     cancel;
    int     used;
    struct nm_desc *nmd;
    struct timespec tic,toc,tx_period;
    pthread_t   thread;
    int     affinity;
    int     tx_rate;
    int     burst;
    int     vlanid;
    pkt_t   pkt;
    uint64_t    total_send;
    thread_func operate;
    // result
    uint64_t    pkts;
    uint64_t    bytes;
    uint64_t    counter;
    uint64_t    recverr;
    //
    struct ether_addr dst_mac;
    struct ether_addr src_mac;

    uint16_t    randidx[65536];
    int         seq_rand;
    int         ip_range;
    int         port_range;
}thread_arg_t,*thread_arg_ptr;

#define FUNC_DC         (0)
#define FUNC_SEND       (1)
#define FUNC_RECV       (2)
#define FUNC_RECV_SEND  (3)
typedef struct system_arg{
    int     func;
    int     affinity;
    int     vlanid;
    int     seq_rand;
    int     ip_range;
    int     port_range;
    char    ifnm[64];
    struct nm_desc *nmd;
    pthread_t observer_thread;
}system_arg_t, *system_arg_ptr;


// gtp-u header.
typedef struct gtpu_header {	/* According to 3GPP TS 29.060. */
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
} __attribute__ ((packed)) gtpu_header_t,*gtpu_header_ptr;


#define PHIRESET_WAITSEC    (4)

#ifndef __APPLE__

#define cpuset_t        cpu_set_t

#define ifr_flagshigh  ifr_flags        /* only the low 16 bits here */
#define IFF_PPROMISC   IFF_PROMISC      /* IFF_PPROMISC does not exist */
#include <linux/ethtool.h>
#include <linux/sockios.h>

#define CLOCK_REALTIME_PRECISE CLOCK_REALTIME
#include <netinet/ether.h>      /* ether_aton */
#include <linux/if_packet.h>    /* sockaddr_ll */
#endif  /* !=__APPLE__ */

#ifdef __APPLE__
#define cpuset_t        uint64_t        // XXX
static inline void CPU_ZERO(cpuset_t *p){
    *p = 0;
}

static inline void CPU_SET(uint32_t i, cpuset_t *p){
    *p |= 1<< (i & 0x3f);
}
#define pthread_setaffinity_np(a, b, c) ((void)a, 0)
#define sched_setscheduler(a, b, c)	(1) /* error */
#define clock_gettime(a,b)      \
        do {struct timespec t0 = {0,0}; *(b) = t0; } while (0)

#define	_P64	unsigned long
#endif


#define LOG(_fmt, ...)						\
	do {							\
		struct timeval _t0;				\
		gettimeofday(&_t0, NULL);			\
		fprintf(stdout, "%03d.%06d[%5d]%s " _fmt "\n",	\
		    (int)(_t0.tv_sec % 1000), (int)_t0.tv_usec,	\
		    __LINE__, __FUNCTION__, ##__VA_ARGS__);	\
        } while (0)
#define ERR(_fmt, ...)						\
	do {							\
		struct timeval _t0;				\
		gettimeofday(&_t0, NULL);			\
		fprintf(stderr, "%03d.%06d[%5d]%s " _fmt "\n",	\
		    (int)(_t0.tv_sec % 1000), (int)_t0.tv_usec,	\
		    __LINE__, __FUNCTION__, ##__VA_ARGS__);	\
        } while (0)

static inline const char *norm2(char *buf, double val, const char *fmt){
    const char *units[] = { "", "K", "M", "G", "T" };
    u_int i;
    for (i = 0; val >=1000 && i < sizeof(units)/sizeof(char *) - 1; i++)
        val /= 1000;
    sprintf(buf, fmt, val, units[i]);
    return(buf);
}
static inline const char *norm(char *buf, double val){
    return norm2(buf, val, "%.3f %s");
}
static inline struct timespec timespec_add(struct timespec a, struct timespec b){
    struct timespec ret = { a.tv_sec + b.tv_sec, a.tv_nsec + b.tv_nsec };
    if (ret.tv_nsec >= 1000000000) {
        ret.tv_sec++;
        ret.tv_nsec -= 1000000000;
    }
    return ret;
}
static inline struct timespec timespec_sub(struct timespec a, struct timespec b){
    struct timespec ret = { a.tv_sec - b.tv_sec, a.tv_nsec - b.tv_nsec };
    if (ret.tv_nsec < 0) {
        ret.tv_sec--;
        ret.tv_nsec += 1000000000;
    }
    return ret;
}
static inline uint32_t checksum(const void *data, uint16_t len, uint32_t sum){
    unsigned  _sum   = sum,_checksum = 0;
    unsigned  _count = len;
    unsigned short* _addr  = (unsigned short*)data;
    //
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

static inline uint16_t wrapsum(uint32_t sum){
    sum = ~sum & 0xFFFF;
    return (htons(sum));
}

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


  typedef struct ipv6_pseudo_header{
      struct in6_addr     srcaddr;
      struct in6_addr     dstaddr;
      uint32_t            payloadlen;
      uint32_t            next;
  }__attribute__ ((packed)) ipv6_pseudo_header_t,*ipv6_pseudo_header_ptr;
  
  static_assert(sizeof(ipv6_pseudo_header_t)==40, "need 40 octet");


#include <string>
#include <fstream>
  static uint64_t pcapcounter_;
  static inline bool file_is_appendable(const std::string& path) {
      std::ofstream check_appendable_file;
      check_appendable_file.open(path.c_str(), std::ios::app);
      if (check_appendable_file.is_open()) {
          // all fine, just close file
          check_appendable_file.close();

          return true;
      } else {
          return false;
      }
  }
static inline FILE* start_pcap(const char* p){
    std::string path = "/tmp/hoge.pcap";
    if (p){
        path = p;
    }
    unlink(path.c_str());
    return(fopen(path.c_str(),"a+"));
}
static inline void stop_pcap(FILE* f){
    fflush(f);
    fclose(f);
}
extern FILE* pcapf_;
static inline void append_pcap(FILE* f, const char* data, unsigned len){
    pcap_fheader_t  fhead;
    pcap_pkthdr_t   pkth;
    //
    if (!pcapf_|| (len + sizeof(pkth)) > ETHER_MAX_LEN){ return; }
    static uint64_t pcap_counter = 0;
    // file header if need.
    if (!pcap_counter++){
        memset(&fhead,0,sizeof(fhead));
        fhead.magic = htonl(0xd4c3b2a1);
        fhead.major = 2;
        fhead.minor = 4;
        fhead.snap  = htonl(0x00000400);
        fhead.ltype = htonl(0x01000000);
        //
        if (fwrite(&fhead, 1, sizeof(fhead), f) == 0){
            throw std::runtime_error("write file.");
        }
    }
    //
    pkth.tm_sec  = (uint32_t)(pcap_counter>>32);
    pkth.tm_usec = (uint32_t)(pcap_counter&0xffffffff);
    pkth.pcaplen = (uint32_t)(len);
    pkth.len     = (uint32_t)(len);
    if (fwrite(&pkth, 1, sizeof(pkth), f) == 0){
        throw std::runtime_error("fwirtfwirtee");
    }
    // user payload
    if (fwrite(data, 1, len, f) == 0){
        throw std::runtime_error("fwrite");
    }
    // flush
    fflush(f);
}

  static inline std::string pcapf(const std::string& dir){
      char pid[32] = {0};
      sprintf(pid, "%d", (unsigned)getpid());
      return(dir + pid + ".pcap");
  }

#ifdef __cplusplus
extern "C" {
#endif

// common.
void usage(void);
int setaffinity(pthread_t , int);
void sigint_h(int );
void init_globals(int argc, char **argv, system_arg_ptr);
void open_netmap(system_arg_ptr);
void logging_netmap(system_arg_ptr);
void start_thread(system_arg_ptr);
void wait_thread(system_arg_ptr);
uint64_t wait_for_next_report(struct timeval *, struct timeval *, int );
thread_arg_ptr ref_thread_arg(int);
unsigned THREAD_NUM(void);
void SET_THREAD_NUM(unsigned);


// operators
void *strs_send(void *data);
void *strs_recv(void *data);
void *strs_recv_send(void *data);
void *strs_observer(void *data);
int  send_packets(thread_arg_ptr, struct netmap_ring*, struct pkt*, int, u_int);
int  recv_packets(thread_arg_ptr, struct netmap_ring*, uint64_t* ,uint64_t* );
void update_packet(struct pkt*, thread_arg_ptr);
void check_packet(struct pkt*, thread_arg_ptr,uint64_t);
struct timespec wait_time(struct timespec ts);
void init_randidx(void);

#ifdef __cplusplus
}
#endif


#endif //DATAPLANE_STRS_H
