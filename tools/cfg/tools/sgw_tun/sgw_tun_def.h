#ifndef SGW_EMU_SGW_EMU_DEF_H
#define SGW_EMU_SGW_EMU_DEF_H
#define _GNU_SOURCE 1
#include <sched.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <poll.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <net/ethernet.h>
#include <netinet/tcp.h>
#include <assert.h>
#include <errno.h>

#ifndef __FAVOR_BSD
#define __FAVOR_BSD
#endif
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/ip_icmp.h>
#include <netinet/if_ether.h>
#include <net/if_arp.h>
#include <pthread.h>
#include <ifaddrs.h>

#ifndef __APPLE__
#include <linux/if_ether.h>
#include <linux/sockios.h>
#include <linux/if_tun.h>
#else
#include <libgen.h>
#include <ifaddrs.h>
#include <net/if_dl.h>
#include <sys/sys_domain.h>
#include <sys/ioctl.h>
#include <net/if_utun.h>
#include <sys/kern_control.h>


#endif

//#define LOCALIP             ("127.0.0.3")
#define LOCALMASK           ("255.255.255.0")
#define GTPU_PORT           2152
#define GTPC_PORT           2123

#ifndef TUNNEL_TUNTAP_DEVICE
#define TUNNEL_TUNTAP_DEVICE               "/dev/net/tun"
#endif

#ifndef MIN
# define MIN(a,b) (a<b?a:b)
#endif
#ifndef MAX
# define MAX(a,b) (a>b?a:b)
#endif

#define GTPC_STATE_DC                 (0)
#define GTPC_STATE_CREATE_SESS_REQ    (1)
#define GTPC_STATE_CREATE_SESS_RES    (2)
#define GTPC_STATE_MODIFY_BEARER_REQ  (3)
#define GTPC_STATE_MODIFY_BEARER_RES  (4)


#define CREATE_SESSION_APN            ("affirmed1.ctc")
#define CREATE_SESSION_EBI            (5)
#define CREATE_SESSION_AMBR_UP        (10000)
#define CREATE_SESSION_AMBR_DOWN      (10000)

typedef struct sgw_tun_container{
    int tun_fd;
    int udp_fd;
    int udpc_fd;
    int gtpc_state;
    pthread_t tun_thread;
    pthread_t udp_thread;
    struct sockaddr_in pgwpeer;
    char device_name[64]; // ex. /dev/net/tun0

    char local_addr[64];
    unsigned short local_port;

    char peer_addr_dst[64];
    unsigned short teid;

    unsigned long long size_tun_to_udp;
    unsigned long long size_udp_to_tun;
    // 
    unsigned long long imsi,msisdn;
    unsigned int fteid;
    unsigned int sgwfteid;
    char sgwgtpuipv[32];
    char ueipv[32];

}sgw_tun_container_t,*sgw_tun_container_ptr;

#endif //SGW_EMU_SGW_EMU_DEF_H



