/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       start.c
    @brief      start PGW system
*******************************************************************************
*******************************************************************************
    @date       created(07/nov/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 07/nov/2017
      -# Initial Version
******************************************************************************/

#include "pgw_ext.h"
#include "node_ext.h"
#include "gtpc_ext.h"
#include "gtpu_ext.h"
#include <mysql.h>

static RETCD __start_udp_server(server_ptr);
static void* __udp_server_thread(void* arg);
static void __on_receive(evutil_socket_t, short, void*);
static void __on_timeout(evutil_socket_t, short, void*);

/**
 start instance.\n
 *******************************************************************************
 start client library\n
 *******************************************************************************
 @param[in]     pinst     instance address
 @param[in]     callback  callback
 @param[in]     userdata  callback user data
 @return RETCD  0==success,0!=error
*/
RETCD pgw_start(handle_ptr pinst, pgw_callback callback, PTR userdata){
    pinst->callback = callback;
    pinst->callback_data = userdata;
    // start gtpc server
    if (__start_udp_server(pinst->server_gtpc) != OK){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. __start_udp_server(gtpc).\n");
        return(ERR);
    }
    // start gtpu server
    if (__start_udp_server(pinst->server_gtpu) != OK){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. __start_udp_server(gtpu).\n");
        return(ERR);
    }
    // start node(data access function)
    if (node_start_paralell(pinst) != OK){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. node_start_paralell.\n");
        return(ERR);
    }
    return(OK);
}
/**
 start server func.\n
 *******************************************************************************
 start udp server\n
 *******************************************************************************
 @param[in]     pserver  instance address
 @return RETCD  0==success,0!=error
*/
RETCD __start_udp_server(server_ptr pserver){
    if (!pserver){
#ifdef __TESTING__
        return(OK);
#else
        return(ERR);
#endif
    }
    pserver->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (pserver->sock < 0) {
        PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. create socket(%d: %s)\n", errno, strerror(errno));
        return(ERR);
    }
#ifndef __linux__
#ifdef SO_BINDTODEVICE
    PGW_LOG(PGW_LOG_LEVEL_ERR, "linux__ \n");
    setsockopt(pserver->sock, SOL_SOCKET, SO_BINDTODEVICE, pserver->nic, strlen(pserver->nic)+1);
#endif
#endif
    if (set_nonblock(pserver->sock)) {
        PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. socket unblocking(%d: %s)\n", errno, strerror(errno));
        goto error;
    }
    //
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(pserver->port);
    inet_pton(AF_INET, (char*)pserver->ip, &addr.sin_addr.s_addr);
    memcpy(&pserver->addr, &addr, sizeof(addr));
    //
    if (bind(pserver->sock, (struct sockaddr *)&addr, sizeof(addr))) {
        PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. bind (%d: %s)\n", errno, strerror(errno));
        goto error;
    }
    PGW_LOG(PGW_LOG_LEVEL_INF, "udpserver. bind (%d: %s)\n", pserver->port, pserver->ip);

    if (pthread_create(&pserver->thread, NULL, __udp_server_thread, pserver)) {
        PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. pthread_create(%d: %s)\n", errno, strerror(errno));
        goto error;
    }
    return(OK);
error:
    if (pserver->sock >= 0) {
        close(pserver->sock);
        pserver->sock = -1;
    }
    return(ERR);
}
/**
 udp server thread.\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]     arg     thread argument
 @return void* not supported
*/
void* __udp_server_thread(void* arg){
    server_ptr pserver = (server_ptr)arg;
    struct timeval tm;
    //
    if (pserver == NULL){
        return(NULL);
    }
    PGW_LOG(PGW_LOG_LEVEL_INF, "__udp_server_thread(%s)\n", __func__);
    //
    pserver->event_base = event_base_new();
    pserver->recv_event = event_new(pserver->event_base, pserver->sock, EV_READ|EV_PERSIST, __on_receive, pserver);
    pserver->timeout_event = event_new(pserver->event_base, -1, EV_TIMEOUT|EV_PERSIST, __on_timeout, pserver);
    //
    tm.tv_sec = (SERVER_TO / 1000);
    tm.tv_usec = (SERVER_TO % 1000) * 1000;
    if (event_add(pserver->recv_event, NULL) || event_add(pserver->timeout_event, &tm)) {
        pgw_panic("event_add failed(%d: %s)\n", errno, strerror(errno));
    }
    // event loop
    while(!pserver->halt) {
        event_base_loop(pserver->event_base, EVLOOP_ONCE);
    }
    return(NULL);
}

/**
 receive udp packet\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]     sock    socket descriptor
 @param[in]     what    reason
 @param[in]     arg     user data
*/
void __on_receive(evutil_socket_t sock , short what, void* arg){
    server_ptr pserver = (server_ptr)arg;
    handle_ptr phandle;
    char rbuf[PCKTLEN_UDP];
    struct sockaddr_in ssa;
    unsigned int slen = sizeof(ssa);
    struct sockaddr_in sa;
    socklen_t salen = sizeof(sa);
    ssize_t rlen;
    gtpc_header_ptr rh;
    packet_t        pckt;

    if (pserver == NULL){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid instance(server == NULL)\n");
        goto error;
    }
    phandle = pserver->handle;
    // read gtp header(don't proceed)
    rlen = recvfrom(sock, rbuf, sizeof(gtpc_header_t) -4, MSG_PEEK, (struct sockaddr*)&ssa, &slen);
    // when could not read to header length then drop.
    if (rlen != (sizeof(gtpc_header_t)-4)){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid read header(%d)\n", (INT)rlen);
        goto error;
    }
    rh = (gtpc_header_ptr)rbuf;
    if (ntohs(rh->length) < 4 || ntohs(rh->length) > ETHER_MAX_LEN){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "invalid header length(%d)", ntohs(rh->length));
        goto error;
    }

    pckt.data = (U8*)rh;
    pckt.datalen = sizeof(*rh);
    if (pserver->port == GTPU1_PORT){
        // gtpuvalidate header
        if (gtpu_validate_header(&pckt)!=OK){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. gtpu_validate_header(%d)", ntohs(rh->length));
            goto error;
        }
        pckt.datalen = sizeof(gtpu_header_t) + ntohs(rh->length);
    }else if (pserver->port == GTPC_PORT){
        // gtpcvalidate header
        if (gtpc_validate_header(&pckt)!=OK){
            PGW_LOG(PGW_LOG_LEVEL_ERR, "failed. gtpc_validate_header(%d)", ntohs(rh->length));
            goto error;
        }
        pckt.datalen = 4 + ntohs(rh->length);
    }
    // read remaining payload
    rlen = recvfrom(sock, rbuf, pckt.datalen, 0, NULL, NULL);
    // error if could not read it all together.
    if (rlen < 4 || rlen != pckt.datalen){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "not enough len(%d != %u)\n", (INT)rlen, pckt.datalen);
        goto error;
    }
    //
    if (getsockname(sock, (struct sockaddr *)&sa, &salen) < 0){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "getsockname(%d)\n", (INT)rlen);
        goto error;
    }
    if (phandle->callback){
        ((pgw_callback)phandle->callback)(EVENT_GTPRX, phandle, sock, what, (U8*)rbuf, rlen, &ssa, slen, &sa, salen, pserver);
    }
    return;
error:
    recvfrom(sock, rbuf, 4, 0, (struct sockaddr*)&ssa, &slen);
}
/**
 timer event\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]     sock    socket descriptor
 @param[in]     what    reason
 @param[in]     arg     user data
*/
void __on_timeout(evutil_socket_t sock, short what, void* arg){
    server_ptr pserver = (server_ptr)arg;
    if (pserver){
        if (pserver->main_server == UDPSERVER_MAIN){
            if (pserver->handle->callback){
                ((pgw_callback)pserver->handle->callback)(EVENT_TIMER, pserver->handle, sock, what, NULL, 0, NULL, 0, NULL, 0, pserver);
            }
        }
    }
}

