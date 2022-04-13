//
// Created by mixi on 2017/06/22.
//

#include "../mixipgw_mod_diameter.hpp"

using namespace MIXIPGW_TOOLS;

// timeout event.
void Diameter::OnTimeOut(evutil_socket_t sock, short what, void *arg){
    Diameter* inst = (Diameter*)arg;
    inst->timeout_counter_++;
    auto to = inst->timeout_counter_;
    // logging only
    if ((to % 100) == 0){
        Logger::LOGINF("Diameter::OnTimeOut.(" FMT_LLU ")",to);
    }
    // FIXME: For cleanup in context of libevent,
    //  this timeout event is available.
}