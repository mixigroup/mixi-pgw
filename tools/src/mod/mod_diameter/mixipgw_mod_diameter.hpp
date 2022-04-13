//
// Created by mixi on 2017/06/22.
//

#ifndef MIXIPGW_TOOLS_MOD_DIAMETER_HPP
#define MIXIPGW_TOOLS_MOD_DIAMETER_HPP


#include "mixipgw_tools_def.h"
#include "lib/logger.hpp"
#include "lib/misc.hpp"
#include "lib/buffer.hpp"
#include "lib/process.hpp"
#include "lib/module.hpp"
#include "lib/const/diameter.h"
#include "lib_db/mysql.hpp"

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


#define DIAMETER_LISTEN_Q                           (1024)
#define DIAMETER_CLIENT_THREAD                      (8)
#define DIAMETER_QUEUE_COUNT                        (DIAMETER_CLIENT_THREAD + 1)
#define DIAMETER_QUEUE_ROUNDROBIN                   (DIAMETER_CLIENT_THREAD)
#define DIAMETER_QUEUE_FOR_DB                       (DIAMETER_QUEUE_COUNT-1)
#define DIAMETER_GRANTED_SERVICE_UNIT_SIZE          (1048576)


#define PADD4(a)    ((int)a+(a%4?(4-(a%4)):0))

#ifndef SOL_TCP
#define SOL_TCP     (IPPROTO_TCP)
#endif

//
typedef std::map<uint32_t, std::string> IE;
typedef std::map<uint32_t, std::string>::iterator IEITR;
//
typedef std::map<std::string, std::string>  SESS;
typedef std::map<std::string, std::string>::iterator SESSITR;

//
typedef std::map<ULONGLONG, std::map<ULONGLONG, MIXIPGW_TOOLS::diameter_link_t> > LINK;
typedef std::map<ULONGLONG, std::map<ULONGLONG, MIXIPGW_TOOLS::diameter_link_t> >::iterator LINKITR;

// container
class ThreadContainer{
public:
    ThreadContainer():id_(0), ptr_(NULL){}
    ~ThreadContainer(){}
public:
    uint32_t    id_;
    void*       ptr_;
};

// diameter server
class PacketContainer;
class Diameter{
public:
    Diameter(const char*);
    virtual ~Diameter();
public:
    void Start(void);
    void Stop(void);
    void InitTable(void);
private:
    static void*RunLoop(void *);
    static void*ClientLoop(void *);
    static void*BinLogLoop(void *);
    static void*QueueLoop(void *);
    static void OnAccept(evutil_socket_t, short, void *);
    static void OnRecv(evutil_socket_t, short, void *);
    static void OnTimeOut(evutil_socket_t, short, void *);
public:
    int  ReqGx(MIXIPGW_TOOLS::diameter_header_ptr,evutil_socket_t, IE&, void*);
    int  ReqGy(MIXIPGW_TOOLS::diameter_header_ptr,evutil_socket_t, IE&, void*);
    int  ReqDw(MIXIPGW_TOOLS::diameter_header_ptr,evutil_socket_t, IE&, void*);
    int  ReqCe(MIXIPGW_TOOLS::diameter_header_ptr,evutil_socket_t, IE&, void*);
    int  ReqAny(void*,evutil_socket_t,void*);
    //
    void OperateTable(MIXIPGW_TOOLS::diameter_link_ptr);
    bool FindTable(MIXIPGW_TOOLS::diameter_link_ptr);
private:
    // Gx
    int  ReqGxInitial(MIXIPGW_TOOLS::diameter_header_ptr,evutil_socket_t, IE&, void*);
    int  ReqGxUpdate(MIXIPGW_TOOLS::diameter_header_ptr,evutil_socket_t, IE&, void*);
    int  ReqGxTerminate(MIXIPGW_TOOLS::diameter_header_ptr,evutil_socket_t, IE&, void*);
    int  ReqGxEvent(MIXIPGW_TOOLS::diameter_header_ptr,evutil_socket_t, IE&, void*);
    // Gy
    int  ReqGyInitial(MIXIPGW_TOOLS::diameter_header_ptr,evutil_socket_t, IE&, void*);
    int  ReqGyUpdate(MIXIPGW_TOOLS::diameter_header_ptr,evutil_socket_t, IE&, void*);
    int  ReqGyTerminate(MIXIPGW_TOOLS::diameter_header_ptr,evutil_socket_t, IE&, void*);
    int  ReqGyEvent(MIXIPGW_TOOLS::diameter_header_ptr,evutil_socket_t, IE&, void*);

private:
    struct event_base   *event_base_;
    struct event_base   *event_base_cli_[DIAMETER_CLIENT_THREAD];
    struct event        *recv_event_;
    struct event        *timeout_event_;
    pthread_t           threadid_;
    pthread_t           threadid_db_;
    pthread_t           threadid_cli_[DIAMETER_CLIENT_THREAD];
    pthread_t           threadid_queue_[DIAMETER_QUEUE_COUNT];
    pthread_mutex_t     diameter_link_mtx_;
    pthread_mutex_t     diameter_queue_mtx_[DIAMETER_QUEUE_COUNT];
    std::deque<PacketContainer*>    diameter_queue_[DIAMETER_QUEUE_COUNT];
    ThreadContainer     queue_loop_container_[DIAMETER_QUEUE_COUNT];
    std::string         dbhost_;
    std::string         dbport_;
    std::string         dbuser_;
    std::string         dbpswd_;
    std::string         dbinst_;
    int                 dbserverid_;
    uint64_t            client_counter_;
    uint64_t            timeout_counter_;
    LINK                diameter_link_;
    int                 cpuid_;
    std::string         diameter_binlog_;
    int                 diameter_binlog_position_;
    int                 port_;
    std::string         listen_interface_;
    std::string         origin_host_;
    std::string         origin_realm_;
    std::string         host_ip_address_;
    std::string         monitoring_key_;
    std::string         product_name_;
    int                 vendor_id_;

public:
    struct sockaddr_in  addr_;
    int                 addrlen_;
    int                 accept_sock_;
    SESS                session_;
    MIXIPGW_TOOLS::MysqlCfg   *mysqlconfig_;
};
//
class DiameterClient{
public:
    DiameterClient();
    DiameterClient(const DiameterClient&);
    DiameterClient(int,struct sockaddr_in*,unsigned int);
    virtual ~DiameterClient();
public:
    int Send(void*,int);
    void Response(MIXIPGW_TOOLS::diameter_header_ptr, const std::string& ,const unsigned);
public:
    struct sockaddr_in  addr_in_;
    int                 addr_in_len_;
    int                 client_sock_;
    int                 client_id_;
    uint64_t            counter_;
    struct event       *client_ev_;
    class Diameter    *inst_;
    std::string         origin_host_;
    std::string         origin_realm_;
    MIXIPGW_TOOLS::Mysql      *mysql_;
};
//
class PacketContainer{
public:
    PacketContainer(const void* pkt,unsigned len, int sock, const DiameterClient& cpysrccli){
        // Packet Queuing Container
        //  In constructor processing, it is important that
        //  DiameterClient variable is instance copy.
        //  -> Those are weak copy, must not be released/freed.
        if (!(buffer_ = malloc(len))){
            throw std::runtime_error("malloc");
        }
        memcpy(buffer_, pkt, len);
        sock_ = sock;
        len_ = len;
        cli_ = cpysrccli;
    }
    virtual ~PacketContainer(){
        if (buffer_){
            free(buffer_);
        }
        buffer_ = NULL;
    }
public:
    void* Ref(void){ return(buffer_); }
    unsigned short Len(void){ return(len_); }
    int   Sock(void){ return(sock_); }
    DiameterClient& Client(void){ return(cli_); }
private:
    PacketContainer(){}
    unsigned    len_;
    void *      buffer_;
    int         sock_;
    DiameterClient cli_;
}; // class PacketContainer

#endif //MIXIPGW_TOOLS_MOD_DIAMETER_HPP
