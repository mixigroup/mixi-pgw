/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       pgw_ext.h
    @brief      mixi_pgw_ctrl_plane c function define, common header
*******************************************************************************
*******************************************************************************
    @date       created(07/nov/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license
*******************************************************************************
    @par        History
    - 07/nov/2017 
      -# Initial Version
******************************************************************************/
#ifndef MIXI_PGW_PGW_EXT_H
#define MIXI_PGW_PGW_EXT_H

#include "pgw_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 completion handler \n
 *******************************************************************************
 generic callback\n
 *******************************************************************************
 @param[in]   event           event type
 @param[in]   pinst           instance handle
 @param[in]   sock            socket descriptor
 @param[in]   what            event kind
 @param[in]   data            event data first address
 @param[in]   datalen         event data length
 @param[in]   saddr           server address
 @param[in]   saddrlen        server address length
 @param[in]   caddr           client address
 @param[in]   caddrlen        client address length
 @param[in/out] ext           extend data of event extend
 @return RETCD  0==success,0!=error
*/
typedef RETCD (*pgw_callback)(EVENT event, handle_ptr pinst, evutil_socket_t sock , short what, const U8* data, const INT datalen, struct sockaddr_in* saddr, ssize_t saddrlen, struct sockaddr_in* caddr, ssize_t caddrlen, void* ext);

/**
 instanciate \n
 *******************************************************************************
 initialize client library\n
 *******************************************************************************
 @param[in]     type    not supported : reserved
 @param[out]    ppinst  instance address
 @return RETCD  0==success,0!=error
*/
RETCD pgw_create_instance(INT type, handle_ptr *ppinst);

/**
 deallocate instance\n
 *******************************************************************************
 cleanup client library\n
 *******************************************************************************
 @param[in/out]   ppinst  instance address
 @return RETCD    0==success,0!=error
*/
RETCD pgw_release_instance(handle_ptr *ppinst);


/**
 start instance.\n
 *******************************************************************************
 start client library\n
 *******************************************************************************
 @param[in]     pinst     instance address
 @param[in]     callback  callback
 @param[in]     userdata  callback user
 @return RETCD  0==success,0!=error
*/
RETCD pgw_start(handle_ptr pinst, pgw_callback callback, PTR userdata);

/**
 stop instance.\n
 *******************************************************************************
 stop client library\n
 *******************************************************************************
 @param[in]     pinst     instance address
 @param[in]     callback  callback
 @param[in]     userdata  callback user
 @return RETCD  0==success,0!=error
*/
RETCD pgw_stop(handle_ptr pinst, pgw_callback callback, PTR userdata);


/**
 set property.\n
 *******************************************************************************
 set dynamic property\n
 *******************************************************************************
 @param[in]     pinst instance
 @param[in]     id    property type
 @param[in]     value  data first address
 @param[in]     length data length
 @return RETCD  0==success,0!=error
*/
RETCD pgw_set_property(handle_ptr pinst, INT id, PTR value, INT length);

/**
 get property.\n
 *******************************************************************************
 get dynamic property\n
 *******************************************************************************
 @param[in]     pinst instance
 @param[in]     id    property type
 @param[out]    value  data first address
 @param[out]    length data length
 @return RETCD  0==success,0!=error
*/
RETCD pgw_get_property(handle_ptr pinst, INT id, PTR* value, INT* length);


/**
 inject packet.\n
 *******************************************************************************
 send packet\n
 *******************************************************************************
 @param[in]     pinst     instance
 @parma[in]     saddr     source address
 @parma[in]     caddr     destination address
 @param[in]     caddrlen  destination address length
 @param[in]     packet    packet 
 @param[in]     length    packet length
 @return RETCD  0==success , 0!=error
*/
RETCD pgw_send(handle_ptr pinst, const struct sockaddr_in* saddr, const struct sockaddr_in* caddr, const ssize_t caddrlen, const PTR packet, U32 length);

/**
 inject packet.\n
 *******************************************************************************
 send packet in specific socket\n
 *******************************************************************************
 @param[in]     sock      socket descriptor
 @parma[in]     caddr     destination address
 @param[in]     caddrlen  destination address長
 @param[in]     packet    packet 
 @param[in]     length    packet length
 @return RETCD  0==success ,   0!=error
*/
RETCD pgw_send_sock(int sock, const struct sockaddr_in* caddr, const ssize_t caddrlen, const PTR packet, U32 length);




/**
 dequeue burst.\n
 *******************************************************************************
 dequeue from ring\n
 *******************************************************************************
 @param[in]     pnode   node object
 @param[in]     order   NODE_IPC_INGRESS or NODE_IPC_EGRESS
 @param[in/out] packets packet 
 @param[in]     length  count of burst
 @return RETCD  0>exists, count of data, 0<=empty or error
*/
RETCD pgw_deq_burst(node_ptr pnode, INT order, packet_ptr packets[], U32 length);

/**
 enqueue burst.\n
 *******************************************************************************
 enqueue to ring\n
 *******************************************************************************
 @param[in]     pnode   node object
 @param[in]     order   NODE_IPC_INGRESS or NODE_IPC_EGRESS
 @param[in]     packet  packet 
 @return RETCD  0==success,0!=error
*/
RETCD pgw_enq(node_ptr pnode, INT order, packet_ptr packet);

/**
  swap packet address.\n
 *******************************************************************************
  swap packet address\n
 *******************************************************************************
 @param[in]     packet  packet 
 @return RETCD  0==success,0!=error
 */
RETCD pgw_swap_address(packet_ptr packet);


/**
 allocate packet.\n
 *******************************************************************************
 allocate 1packet memory\n
 *******************************************************************************
 @param[in]     packet  packet 
 @param[in]     length  data length
 @return RETCD  0==success,0!=error
*/
RETCD pgw_alloc_packet(packet_ptr* packet, U32 length);


/**
 duplicate packet.\n
 *******************************************************************************
 duplicate 1packet memory\n
 *******************************************************************************
 @param[in/out] dst     output packet 
 @param[in]     src     input packet 
 @return RETCD  0==success,0!=error
*/
RETCD pgw_duplicate_packet(packet_ptr* dst, const packet_ptr src);


/**
 free packet.\n
 *******************************************************************************
 1 packet release\n
 *******************************************************************************
 @param[in]     packet  packet 
 @return RETCD  0==success,0!=error
*/
RETCD pgw_free_packet(packet_ptr packet);




/**
 SGW Peers callback \n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   userdat         user data
 @param[in]   peer            peer object
 @return RETCD  0==success,0!=error
*/
typedef RETCD (*pgw_sgw_peer_callback)(void* userdat, sgw_peer_ptr peer);

/**
 allocate sgw peer \n
 *******************************************************************************
 initialize sgw peers\n
 *******************************************************************************
 @param[in]     type    not supported : reserved
 @param[out]    pppeer  sgw peersinstance address
 @return RETCD  0==success,0!=error
*/
RETCD pgw_create_sgw_peers(INT type, sgw_peers_ptr *pppeer);

/**
 deallocate sgw peers \n
 *******************************************************************************
 cleanup sgw peers\n
 *******************************************************************************
 @param[in/out]   pppeer  sgw peersinstance address
 @return RETCD    0==success,0!=error
*/
RETCD pgw_free_sgw_peers(sgw_peers_ptr *pppeer);

/**
 sgw peers : peer SET \n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]     peers   sgw peersinstance
 @param[in]     ip      ip address
 @param[in]     counter counter, default= 0
 @param[in]     expire  expire epoch default = time(NULL) + x
 @return RETCD  0==success,0!=error
*/
RETCD pgw_set_sgw_peer(sgw_peers_ptr peers, U32 ip, U32 counter, U32 expire);

/**
 sgw peers : peer GET \n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]     peers    sgw peersinstance
 @param[in]     ip       target ip address
 @param[in]     callback find callback
 @param[in]     udata    callback , user data
 @return RETCD  0==success,0!=error
*/
RETCD pgw_get_sgw_peer(sgw_peers_ptr peers, U32 ip, pgw_sgw_peer_callback callback, void* udata);


/**
 sgw peers : delete peer\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]     peers    sgw peersinstance
 @param[in]     ip       delete target ip address
 @return RETCD  0==success,0!=error
*/
RETCD pgw_delete_sgw_peer(sgw_peers_ptr peers, U32 ip);



#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define pgw_panic(...)  pgw_panic_(__FILENAME__, __LINE__, __VA_ARGS__, "dummy")
#define pgw_panic_(func, line, format, ...) __pgw_panic(func, line, format "%.0s", __VA_ARGS__)
void __pgw_panic(const char *funcname , const int line, const char *format, ...)
#ifdef __GNUC__
#if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 2))
	__attribute__((cold))
#endif
#endif
__attribute__((noreturn))
__attribute__((format(printf, 3, 4)));



extern int PGW_LOG_LEVEL;

#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"

#define PGW_LOG(l, ...)  PGW_LOG_(l, __func__, __VA_ARGS__, "dummy")
#define PGW_LOG_(l, func, format, ...) __PGW_LOG(l, func, format "%.0s", __VA_ARGS__)
static inline void __PGW_LOG(const int level, const char *funcname , const char *format, ...){
    char funcnm_bf[20] = {0};
    char msg_bf[512] = {0};
    int  syslog_level = LOG_DEBUG;
#ifndef __DEBUG_LOG__
    if (level >= PGW_LOG_LEVEL_DBG){
        return;
    }
#endif

    if (level < PGW_LOG_LEVEL){
        memcpy(funcnm_bf, funcname, MIN(strlen(funcname), sizeof(funcnm_bf)-1));
        va_list ap;
        va_start(ap, format);
        vsnprintf(msg_bf, sizeof(msg_bf)-1,format, ap);
        switch(level){
            case PGW_LOG_LEVEL_ALL: // passthrough
            case PGW_LOG_LEVEL_INF: syslog_level = LOG_INFO; break;
            case PGW_LOG_LEVEL_DBG: syslog_level = LOG_DEBUG; break;
            case PGW_LOG_LEVEL_WRN: syslog_level = LOG_WARNING; break;
            case PGW_LOG_LEVEL_ERR: syslog_level = LOG_ERR; break;
        }
        syslog(syslog_level,"[  %20s]%s", funcnm_bf, msg_bf);
        va_end(ap);
    }
}



/**
 socket, setup nonblocking\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]    sock  descriptor
 @return int  0==success,0!=error
*/
static inline int set_nonblock(int sock){
    int flags,on = 1;

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))< 0){
        pgw_panic("setsockopt.. reuse addr");
    }
    on = 0xB8;  // EF(Expedited Forwarding)
    if (setsockopt(sock, IPPROTO_IP, IP_TOS, &on, sizeof(on))< 0){
        pgw_panic("setsockopt.. ip tos");
    }
#ifdef __APPLE__   // MacOS/X requires an additional call also
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on))< 0){
        pgw_panic("setsockopt.. reuse port");
    }
#endif
    flags = fcntl(sock, F_GETFL);
    if (flags < 0){
        return flags;
    }
    return fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}
extern int PGW_RECOVERY_COUNT;


#ifdef __cplusplus
}
#endif


#endif //MIXI_PGW_PGW_EXT_H
