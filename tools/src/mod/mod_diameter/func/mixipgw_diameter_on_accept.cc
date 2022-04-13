//
// Created by mixi on 2017/06/22.
//

#include "../mixipgw_mod_diameter.hpp"
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sys/socket.h>


using namespace MIXIPGW_TOOLS;

// received event. udp packet
void Diameter::OnAccept(evutil_socket_t sock, short what, void *arg){
    auto inst = (Diameter*)arg;
    struct sockaddr_in addr;
    int one = 1;
    socklen_t addrlen = sizeof(addr);
    //
    auto client = accept(sock, (struct sockaddr*)&addr, &addrlen);

    if (client>0){
        auto flags = fcntl(client,F_GETFL,0);
        if (flags < 0){
            throw std::runtime_error("fcntl(F_GETFL)..");
        }
        fcntl(client,F_SETFL,flags^O_NONBLOCK);
        if (setsockopt(client, SOL_TCP, TCP_NODELAY, &one, sizeof(one)) < 0){
            throw std::runtime_error("nagle off..");
        }
        one = (1024*1024*32);
        if (setsockopt(client, SOL_SOCKET, SO_RCVBUF, &one, sizeof(one)) < 0){
            throw std::runtime_error("recieve buffer size..");
        }
        one = (1024*1024*32);
        if (setsockopt(client, SOL_SOCKET, SO_SNDBUF, &one, sizeof(one)) < 0){
            throw std::runtime_error("send buffer size..");
        }
        // round-robin selection of event loops.
        inst->client_counter_++;

        auto clienttid = (inst->client_counter_%DIAMETER_CLIENT_THREAD);
        auto pcli = new DiameterClient(client, &addr, (unsigned int)addrlen);
        auto clievent = event_new(inst->event_base_cli_[clienttid], client, EV_READ|EV_PERSIST, Diameter::OnRecv, pcli);
        pcli->client_ev_    = clievent;
        pcli->inst_         = inst;
        pcli->origin_host_  = inst->origin_host_;
        pcli->origin_realm_ = inst->origin_realm_;
        pcli->client_id_    = clienttid;
        // prepare database connection every client
        pcli->mysql_        = new Mysql(inst->mysqlconfig_);
        //
        if (clievent){
            if (!event_add(clievent, NULL)){
                char rmt[32]={0};
                inet_ntop(AF_INET, &addr.sin_addr.s_addr, rmt, sizeof(rmt));
                // connected from tcp client peer
                Logger::LOGINF("tcp connected from. (%s:%u) cli(%p) ev(%p)", rmt, addr.sin_port, pcli, inst->event_base_cli_[clienttid]);
            }
        }
    }
}
