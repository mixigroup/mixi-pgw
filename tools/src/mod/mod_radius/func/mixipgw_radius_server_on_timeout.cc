//
// Created by mixi on 2017/06/13.
//

#include "../mixipgw_mod_radius.hpp"
using namespace MIXIPGW_TOOLS;

// timeout event.
void RadiusServer::OnTimeOut(evutil_socket_t sock, short what, void *arg){
    RadiusServer* inst = (RadiusServer*)arg;
    inst->timeout_counter_++;
    auto to = inst->timeout_counter_;
    // only logging.
    if ((to % 100) == 0){
        Logger::LOGINF("RadiusServer::OnTimeOut.(" FMT_LLU ")",to);
    }
    // FIXME: For cleanup in context of libevent,
    //  this timeout event is available.
}