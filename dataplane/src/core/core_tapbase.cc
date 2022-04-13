/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       core_tapbase.cc
    @brief      Tap - Core Implement common
*******************************************************************************
	forward or duplicate packets received from tap interface\n
	\n
*******************************************************************************
    @date       created(16/nov/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 16/nov/2017 
      -# Initial Version
******************************************************************************/
#include "../../inc/core.hpp"
#include "mixi_pgw_data_plane_def.hpp"

#ifndef __APPLE__
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <net/if.h>
#include <linux/if_ether.h>
#include <linux/sockios.h>
#include <linux/if_tun.h>
#include <fcntl.h>
#else
#include <libgen.h>
#include <ifaddrs.h>
#include <net/if_dl.h>
#include <sys/sys_domain.h>
#include <sys/ioctl.h>
#include <net/if_utun.h>
#include <sys/kern_control.h>
#endif

#ifndef TUNNEL_TUNTAP_DEVICE
#define TUNNEL_TUNTAP_DEVICE               "/dev/net/tun"
#endif

using namespace MIXIPGW;

MIXIPGW::CoreTapBase::CoreTapBase(){
    throw("can not use copy constructor.");
}

/**
   open tap\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]     tapname      name 
   @return RETCD  tap descriptor
 */
RETCD MIXIPGW::CoreTapBase::UtapOpen(TXT tapname){
    int fd = -1;
    char *device = NULL;
    struct ifreq req;
    //
    if ((device = strdup(TUNNEL_TUNTAP_DEVICE)) == NULL){
        rte_exit(EXIT_FAILURE, "failed. memory allocate(%d/%s)\n", errno, strerror(errno));
    }
    //
    if ((fd = open(device, O_RDWR|O_NONBLOCK)) < 0){
        rte_exit(EXIT_FAILURE, "failed. open device(%d/%s)\n", errno, strerror(errno));
    }
    free(device);
    bzero(&req, sizeof(req));
    strcpy(req.ifr_name, tapname);
    // set iff(without packet information)
    req.ifr_flags = (IFF_TAP | IFF_NO_PI);
    if (ioctl(fd, TUNSETIFF, (void*)&req) < 0){
        rte_exit(EXIT_FAILURE, "failed ioctl:TUNSETIFF(%d/%s)\n", errno, strerror(errno));
    }
    // down.
    if (SockCtl(SIOCGIFFLAGS, &req) < 0){
        rte_exit(EXIT_FAILURE, "failed ioctl:SIOCGIFFLAGS(%d/%s)\n", errno, strerror(errno));
    }
    req.ifr_flags &= ~IFF_UP;
    req.ifr_flags &= ~IFF_RUNNING;
    //
    if (SockCtl(SIOCSIFFLAGS, &req) < 0){
        rte_exit(EXIT_FAILURE, "failed ioctl(IFF_UP|IFF_RUNNING)\n");
    }
    req.ifr_flags |= IFF_UP;
    req.ifr_flags |= IFF_RUNNING;
    req.ifr_flags &= ~IFF_NOARP;

    if (SockCtl(SIOCSIFFLAGS, &req) < 0){
        rte_exit(EXIT_FAILURE, "failed ioctl(IFF_UP|IFF_RUNNING)\n");
    }
    return(fd);
}
/**
   setup socket\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]     cmd command
   @param[in]     req request 
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::CoreTapBase::SockCtl(int cmd, void* req){
    int ret = 0;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        rte_exit(EXIT_FAILURE, "Failed to open socket for ioctl.(%d)\n", cmd);
    }
    if ((ret = ioctl(sock, cmd, req)) < 0) {
        printf("failed socket_ioctl(%d/%d/%s)\n", cmd, errno, strerror(errno));
    }
    close(sock);
    return(0);
}

