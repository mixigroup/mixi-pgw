#include "sgw_tun_def.h"
#include "sgw_tun_ext.h"
namespace utun{
     #include <linux/route.h>
};
#include "lib/const/gtpu.h"

static int socket_ioctl(int cmd, void* req);
static int tun_set_queue(int fd, int enable);

void* utun_run(void* arg){
    sgw_tun_container_ptr p = (sgw_tun_container_ptr)arg;
    struct timeval tv{1,0};
    assert(p);
    //
    p->tun_fd = utun_open(p);
    assert(p->tun_fd >= 0);

    printf(">>tun loop(%d/%p/%s/%d/%d)\n", getpid(), (void*)pthread_self(), p->peer_addr_dst, p->teid, p->tun_fd);

    struct sockaddr_in peer;
    bzero(&peer, sizeof(peer));
    peer.sin_family = AF_INET;
    assert(inet_pton(AF_INET, p->peer_addr_dst, &peer.sin_addr.s_addr));
    peer.sin_port = htons(GTPU_PORT);
    peer.sin_family = AF_INET;

    memcpy(&p->pgwpeer, &peer, sizeof(peer));

    //
    while(!QUIT()){
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(p->tun_fd, &rfds);
        auto r = select(p->tun_fd+1, &rfds, NULL, NULL, &tv);
        if (r<0){
            if (errno == EINTR){ continue; }
            printf("err(%s/%d)\n", strerror(errno), errno);
            break;
        }else if (r == 0){
            continue; // timeout
        }
        //
        if (FD_ISSET(p->tun_fd, &rfds)) {
            char bf[ETHER_MAX_LEN];
            auto gh = (MIXIPGW_TOOLS::gtpu_header_ptr)bf;
            bzero(gh, sizeof(*gh));
            auto l = read(p->tun_fd,(void*)&bf[sizeof(*gh)],ETHER_MAX_LEN - sizeof(*gh));
            if (l > 0){
                gh->tid = p->sgwfteid;
                gh->type= 0xff;
                gh->u.v1_flags.version = 1;
                gh->u.v1_flags.proto = 1;
                gh->length = htons(l);
                // 
                auto s = sendto(p->udp_fd,bf,l+sizeof(*gh),0, (struct sockaddr*)&p->pgwpeer, sizeof(p->pgwpeer));
                if (s > 0){
                    p->size_tun_to_udp += s;
                }
            }
        }
    }
    return(NULL);
}
int utun_open(void* arg) {
    int fd = -1;
    char *device = NULL;
    sgw_tun_container_ptr p = (sgw_tun_container_ptr)arg;
    //
    device = strdup(TUNNEL_TUNTAP_DEVICE);
    assert(device);
    //
    fd = open(device, O_RDWR|O_NONBLOCK);
    assert(fd>=0);
    free(device);
    struct ifreq req;
    bzero(&req, sizeof(req));
    strcpy(req.ifr_name, p->device_name);
    // set iff
    req.ifr_flags = (TUN_TUN_DEV|IFF_NO_PI);
    if (ioctl(fd, TUNSETIFF, (void*)&req) < 0){
        printf("failed ioctl:TUNSETIFF(%d/%s)\n", errno, strerror(errno));
        assert(!"failed ioctl:TUNSETIFF");
    }

    struct sockaddr_in *sin = NULL;

    // address
    sin = (struct sockaddr_in *) &req.ifr_addr;
    sin->sin_family = AF_INET;
    if(inet_aton(p->ueipv, &sin->sin_addr) == 0) { assert(!"Bad IP address(peer)"); }
    if (socket_ioctl(SIOCSIFADDR , (void*)&req) < 0){
        printf("failed ioctl:SIOCSIFADDR(%d/%s)\n", errno, strerror(errno));
        assert(!"failed ioctl:SIOCSIFADDR");
    }
    // netmask
    sin = (struct sockaddr_in *) &req.ifr_netmask;
    sin->sin_family = AF_INET;
    if(inet_aton(LOCALMASK, &sin->sin_addr) == 0) { assert(!"Bad IP address(mask)"); }
    if (socket_ioctl(SIOCSIFNETMASK, (void*)&req) < 0){
        printf("failed ioctl:SIOCSIFNETMASK(%d/%s)\n", errno, strerror(errno));
        assert(!"failed ioctl:SIOCSIFNETMASK");
    }
    // link dest
    sin = (struct sockaddr_in *) &req.ifr_netmask;
    sin->sin_family = AF_INET;
    if(inet_aton(p->ueipv, &sin->sin_addr) == 0) { assert(!"Bad IP address(dst)"); }
    if (socket_ioctl(SIOCSIFDSTADDR, (void*)&req) < 0){
        printf("failed ioctl:SIOCSIFDSTADDR(%d/%s)\n", errno, strerror(errno));
        assert(!"failed ioctl:SIOCSIFDSTADDR");
    }
    //
    bzero(&req, sizeof(req));
    strcpy(req.ifr_name, p->device_name);
    if (socket_ioctl(SIOCGIFFLAGS, &req) < 0){
        printf("failed ioctl:SIOCGIFFLAGS(%d/%s)\n", errno, strerror(errno));
        assert(!"Failed to ioctl(SIOCGIFFLAGS)");
    }
    // down.
    if (socket_ioctl(SIOCGIFFLAGS, &req) < 0){
        printf("failed ioctl:SIOCGIFFLAGS(%d/%s)\n", errno, strerror(errno));
        assert(!"Failed to ioctl(SIOCGIFFLAGS)");
    }
    req.ifr_flags &= ~IFF_UP;
    req.ifr_flags &= ~IFF_RUNNING;
    //
    if (socket_ioctl(SIOCSIFFLAGS, &req) < 0){
        assert(!"failed ioctl(IFF_UP|IFF_RUNNING)");
    }
    // interface index
    if (socket_ioctl(SIOCGIFINDEX, (void*)&req) < 0){
        printf("failed ioctl:SIOCGIFINDEX(%d/%s)\n", errno, strerror(errno));
        assert(!"failed ioctl:SIOCGIFINDEX");
    }
    // not point to point.
    if (socket_ioctl(SIOCGIFFLAGS, &req) < 0){
        printf("failed ioctl:SIOCGIFFLAGS(%d/%s)\n", errno, strerror(errno));
        assert(!"Failed to ioctl(SIOCGIFFLAGS)");
    }
    req.ifr_flags &= ~IFF_POINTOPOINT;
    req.ifr_flags |= IFF_BROADCAST;
    if (socket_ioctl(SIOCSIFFLAGS, &req) < 0){
        assert(!"failed ioctl(IFF_UP|IFF_RUNNING)");
    }
    if (socket_ioctl(SIOCGIFFLAGS, &req) < 0){
        printf("failed ioctl:SIOCGIFFLAGS(%d/%s)\n", errno, strerror(errno));
        assert(!"Failed to ioctl(SIOCGIFFLAGS)");
    }
    req.ifr_flags |= IFF_UP;
    req.ifr_flags |= IFF_RUNNING;
    req.ifr_flags &= ~IFF_NOARP;

    if (socket_ioctl(SIOCSIFFLAGS, &req) < 0){
        assert(!"failed ioctl(IFF_UP|IFF_RUNNING)");
    }
    if (socket_ioctl(SIOCGIFFLAGS, &req) < 0){
        printf("failed ioctl:SIOCGIFFLAGS(%d/%s)\n", errno, strerror(errno));
        assert(!"Failed to ioctl(SIOCGIFFLAGS)");
    }
#if 0
    // route
    struct rtentry rt;
    struct sockaddr_in *sockinfo = (struct sockaddr_in *)&rt.rt_gateway;
    sockinfo->sin_family = AF_INET;
    inet_pton(AF_INET, p->peer_addr_dst, &sockinfo->sin_addr);
    sockinfo = (struct sockaddr_in *)&rt.rt_dst;
    sockinfo->sin_family = AF_INET;
    sockinfo->sin_addr.s_addr = INADDR_ANY;
    //
    sockinfo = (struct sockaddr_in *)&rt.rt_genmask;
    sockinfo->sin_family = AF_INET;
    sockinfo->sin_addr.s_addr = INADDR_ANY;
    rt.rt_flags = RTF_UP | RTF_GATEWAY;
    rt.rt_dev = NULL;
    //
    assert(socket_ioctl(SIOCADDRT, &rt)!=-1);
#endif
    return(fd);
}

int socket_ioctl(int cmd, void* req) {
    int ret = 0;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        assert(!"Failed to open socket for ioctl");
    }
    if ((ret = ioctl(sock, cmd, req)) < 0) {
        printf("failed socket_ioctl(%d/%d/%s)\n", cmd, errno, strerror(errno));
    }
    close(sock);
    return 0;
}



