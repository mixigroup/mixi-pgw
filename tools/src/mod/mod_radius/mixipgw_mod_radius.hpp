//
// Created by mixi on 2017/06/12.
//

#ifndef MIXIPGW_TOOLS_MOD_RADIUS_HPP
#define MIXIPGW_TOOLS_MOD_RADIUS_HPP

#include "mixipgw_tools_def.h"
#include "lib/logger.hpp"
#include "lib/misc.hpp"
#include "lib/buffer.hpp"
#include "lib/process.hpp"
#include "lib/module.hpp"
#include "lib/interface/arch_interface.hpp"
#include "lib/const/link.h"
#include "lib/const/radius.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <stdexcept>
#include <memory>
#include <functional>
#include <vector>
#include <string>
#include <map>

#ifdef __APPLE__
#include <ncurses.h>
#endif
#include <event2/event.h>
#include <event2/thread.h>
#include <boost/circular_buffer.hpp>

#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/engine.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/md5.h>


#define RADIUS_UDP_PORT     (1812)
#define RADIUS_CLIENT_MAX   (1024)
#define RADIUS_TIMECOUNTER  (1000)


typedef std::map<uint32_t, std::string> IE;
typedef std::map<uint32_t, std::string>::iterator IEITR;

typedef std::map<ULONGLONG, std::map<ULONGLONG, MIXIPGW_TOOLS::radius_link_t> > LINK;
typedef std::map<ULONGLONG, std::map<ULONGLONG, MIXIPGW_TOOLS::radius_link_t> >::iterator LINKITR;

class RadiusClient;

typedef struct radius_traffic{
    uint64_t        time;
    RadiusClient*  cli;
    uint8_t         seq;
}radius_traffic_t,*radius_traffic_ptr;

// radiusサーバ
class RadiusServer{
public:
    RadiusServer(const char*);
    virtual ~RadiusServer();
public:
    void Start(void);
    void Stop(void);
    void InitTable(void);
private:
    static void*RunLoop(void *);
    static void*BinLogLoop(void *);
    static void OnRecv(evutil_socket_t, short, void *);
    static void OnTimeOut(evutil_socket_t, short, void *);
public:
    void ReqAccess(evutil_socket_t, IE&, RadiusClient*);
    void ReqAccount(evutil_socket_t, IE&, RadiusClient*);

    void OperateTable(MIXIPGW_TOOLS::radius_link_ptr);
    bool FindTable(MIXIPGW_TOOLS::radius_link_ptr);
private:
    struct event_base   *event_base_;
    struct event        *recv_event_;
    struct event        *timeout_event_;
    pthread_t           threadid_;
    pthread_t           threadid_db_;
    pthread_mutex_t     radius_link_mtx_;
    std::string         dbhost_;
    std::string         dbport_;
    std::string         dbuser_;
    std::string         dbpswd_;
    uint64_t            timeout_counter_;
    LINK                radius_link_;
    int                 cpuid_;
    std::string         radius_binlog_;
    int                 radius_binlog_position_;
public:
    struct sockaddr_in  addr_;
    int                 addrlen_;
    int                 sock_;
    std::string         radius_secret_;
    std::map<ULONGLONG,radius_traffic_t>   radius_identifier_;
};

// client connection
class RadiusClient{
public:
    RadiusClient(struct sockaddr_in*,unsigned int,struct sockaddr*, unsigned int, uint8_t,int,const std::string&);
    virtual ~RadiusClient();
public:
    void ResponseWithCode(uint8_t);
    void ResponseAcceptFrameIp(MIXIPGW_TOOLS::radius_link_t&,uint32_t);
    void Response(void*,unsigned);
    void SetSeq(uint8_t seq){ seq_ = seq;}
    uint8_t GetSeq(void) { return(seq_); }
    void SetAuth(void* auth){ memcpy(auth_, auth, sizeof(auth_)); }
private:
    struct sockaddr     addr_;
    unsigned int        addrlen_;
    struct sockaddr_in  addr_in_;
    int                 addr_in_len_;
    int                 client_sock_;
    uint8_t             seq_;
    uint8_t             auth_[16];
    std::string         radius_secret_;
    MD5_CTX             *ctx_;
private:
    RadiusClient(){}
};

#endif //MIXIPGW_TOOLS_MOD_RADIUS_HPP
