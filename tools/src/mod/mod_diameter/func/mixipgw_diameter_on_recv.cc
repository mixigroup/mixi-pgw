//
// Created by mixi on 2017/06/13.
//
#include "../mixipgw_mod_diameter.hpp"
#include "../mixipgw_mod_diameter.inl"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sys/socket.h>


using namespace MIXIPGW_TOOLS;

// received event. tcp packet
void Diameter::OnRecv(evutil_socket_t sock, short what, void *arg){
    auto cli = (DiameterClient*)arg;
    char rbuf[4096];
    auto dh = (diameter_header_ptr)rbuf;
    auto inst = cli->inst_;
    uint32_t    cc_type;
    ssize_t     recvlen;
    //
    if (what & EV_READ && cli) {
        // read header
        if ((recvlen = recv(sock, dh, sizeof(*dh), 0)) <= 0) {
            Logger::LOGERR("disconnect..(%p:%d[%s])", cli, recvlen, strerror(errno));
            goto err_return;
        }
        // validate header
        uint32_t len = BE24(dh->len);
#if 0
        Logger::LOGINF("header[%04x/%04x/%04x/%04x/%04x/%04x/%04x][%06x:%u:%u]",
                       dh->version,
                       dh->len,
                       dh->f.flags,
                       dh->code,
                       dh->appid,
                       dh->popid,
                       dh->eeid,
                       len,len,
                       (len-sizeof(*dh))
        );
#endif
        if (recvlen != sizeof(*dh) || len == 0 || ((len-sizeof(*dh)) > sizeof(rbuf))){
            Logger::LOGERR("recv(%04x,%04x,%u) len check", recvlen, len, len);

            Logger::LOGINF("header[%04x/%04x/%04x/%04x/%04x/%04x/%04x][%06x:%u:%u]",
                       dh->version,
                       dh->len,
                       dh->f.flags,
                       dh->code,
                       dh->appid,
                       dh->popid,
                       dh->eeid,
                       len,len,
                       (len-sizeof(*dh)));

            goto err_return;
        }
        // read AVPpayload
        if ((recvlen = recv(sock, &rbuf[sizeof(*dh)], (len-sizeof(*dh)), 0)) <= 0) {
            Logger::LOGERR("recv(%d) payload", recvlen);
            goto err_return;
        }
        // interface of custom counter
        if (dh->version == DIAMETER_VERSION_COUNTER_INGRESS || dh->version == DIAMETER_VERSION_COUNTER_EGRESS){
            // Note:
            // internal use only -> no secure
            // in future, CRCs should check.
            cc_type = COUNTER_REQUEST;
        }else{
            if (!ValidateDiameter(len, dh, &cc_type)){
                Logger::LOGERR("failed. basic validate.", len);
                goto err_return;
            }
        }
        // Diameter protocol processing.
        // delegate processing to queued context.
        // Queue was allocated per threa, and
        //  attached to each thread in round-robin when socket is accepted.
        //  -> server resources(connection resources) are distributed appropriately.
        //
        // distribute destination queue of incomingj packets by per thread coutner values.
        //
        // Gy[Update] Requests reguire an insert into database,
        //  so dedicated Gy[Update] queue is used to collect very slow requests.
        //
        // Prioritized distributed queues allow isolation of bottleneck processing.
        //
        //  -> network communication for database access can easily exceed >10 seconds
        //      in worst case.
        //
        auto queue_id = 0;
        if (dh->code == DIAMETER_CMDCODE_CCR_CCA && dh->appid == DIAMETER_APPID_CREDIT_CONTROL && cc_type == UPDATE_REQUEST){
            // data access packet.  : Gy[Update]
            queue_id = DIAMETER_QUEUE_FOR_DB;
        }else if (cc_type == COUNTER_REQUEST){
            queue_id = DIAMETER_QUEUE_FOR_DB;
        }else{
            // typical packet  : Gx,Gy[init/terminate],Device Watch Dog
            auto recvcount = (cli->counter_++);
            queue_id = (recvcount%DIAMETER_QUEUE_ROUNDROBIN);
        }
        auto qmtx = &inst->diameter_queue_mtx_[queue_id];
        auto queu = &inst->diameter_queue_[queue_id];
        //
        pthread_mutex_lock(qmtx);
        queu->push_front(new PacketContainer((const void*)rbuf, len, sock, (*cli)));
        pthread_mutex_unlock(qmtx);

        return;
    }
err_return:
    shutdown(sock, SHUT_RDWR);
    close(sock);
    event_del(cli->client_ev_);
    // prepare mysql client instance by tcp client.
    if (cli->mysql_){
        delete cli->mysql_;
    }
    delete cli;
}
