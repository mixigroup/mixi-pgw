//
// Created by mixi on 2017/06/27.
//
#include "../mixipgw_mod_diameter.hpp"
#include "../mixipgw_mod_diameter.inl"

#define BATCH_SIZE  (16)

using namespace MIXIPGW_TOOLS;

void*Diameter::QueueLoop(void * arg){
    auto qc     = (ThreadContainer*)arg;
    auto inst   = (Diameter*)qc->ptr_;
    auto id     = qc->id_;
    auto& qmtx  = inst->diameter_queue_mtx_[id];
    auto& queu  = inst->diameter_queue_[id];

    Logger::LOGINF("Diameter::QueueLoop(%p:%u)", qc, id);

    // queue loop.
    // Dequeue loops are assigned separately for each ClientLoop.
    while(!Module::ABORT()){
        PacketContainer* pkt[BATCH_SIZE + 1] = {NULL,};
        // Packets arrive in queue for each thread,
        // batch size continuously received with single lock.
        pthread_mutex_lock(&qmtx);
        for(auto n = 0;n < BATCH_SIZE;n++){
            if (!queu.empty()){
                pkt[n] = queu.back();
                queu.pop_back();
            }else{
                break;
            }
        }
        pthread_mutex_unlock(&qmtx);
        // queue is empty -> halt 
        if (!pkt[0]){ usleep(10); continue; }
        //
        for(auto n = 0;n < BATCH_SIZE;n++){
            if (pkt[n] == NULL){ break; }
            //
            auto pbf = pkt[n]->Ref();
            auto len = pkt[n]->Len();
            auto sck = pkt[n]->Sock();
            auto cli = pkt[n]->Client();
            auto dh  = (diameter_header_ptr)pbf;

            auto code   = BE24(dh->code);
            auto appid  = ntohl(dh->appid);
            if (dh->version == DIAMETER_VERSION_COUNTER_INGRESS || dh->version == DIAMETER_VERSION_COUNTER_EGRESS){
                inst->ReqAny(dh, sck, &cli);
            }else{
                IE mpkt;
                if (!ParsePacketToVector(dh, mpkt)){
                    Logger::LOGERR("failed.queue ParsePacketToVector.(%u:%u)", n, len);
                    shutdown(sck, SHUT_RDWR);
                    delete pkt[n];
                    continue;
                }
                if (code == DIAMETER_CMDCODE_CCR_CCA){
                    if (appid == DIAMETER_APPID_3GPP_GX_29212){
                        inst->ReqGx(dh, sck, mpkt, &cli);
                    }else{
                        Logger::LOGERR("not implemented applicationid(%u).", appid);
                        shutdown(sck, SHUT_RDWR);
                    }
                }else if (code == DIAMETER_CMDCODE_DWR_DWA){
                    inst->ReqDw(dh, sck, mpkt, &cli);
                }else if (code == DIAMETER_CMDCODE_CER_CEA){
                    inst->ReqCe(dh, sck, mpkt, &cli);
                }else{
                    Logger::LOGERR("not implemented code(%u / %u).", code, appid);
                    shutdown(sck, SHUT_RDWR);
                }
            }
            delete pkt[n];
        }
    }
    return(NULL);
}
