//
// Created by mixi on 2017/06/22.
//
#include "mixi-binlog.h"
#include "../mixipgw_mod_diameter.hpp"


using namespace MIXIPGW_TOOLS;
namespace mx = MIXIBINLOG;

#define TBL_DIAMETER  ("diameter")

//
namespace MIXIBINLOG_CLIENT {
    typedef enum DIAMETER_COL_{
        INTIDX = 0,
        IMSI,   UEIPV4,     UEIPV6,     NASIPV,
        POLICY, INGRESS,    EGRESS,     THRESHOLD,
        ACTIVE, UPDATED_AT,
        MAX
    }DIAMETER_COL;

    // notify callback defined.
    class NotifyImpl:
        public mx::Notify
    {
    public:
        virtual int OnNotify(
            int event,
            int clmnid,
            int type,
            void* value,
            int len,
            void* data
        ) override;
    };
    typedef struct record {
        int evt;
        diameter_link_t lnk;
    } record_t, *record_ptr;
}; // namespace MIXIBINLOG_CLIENT

// notify callback implementation.
int
MIXIBINLOG_CLIENT::NotifyImpl::OnNotify(
    int event,      /* event */
    int clmnid,     /* index of columns  */
    int type,       /* type of value(TYPE_NUMERIC, TYPE_TEXT, etc) */
    void* value,    /* column value */
    int len,        /* length of value */
    void* data      /* user-data */
)
{
    auto rec = (record_ptr)data;
    if (type == mx::Notify::TYPE_NUMERIC) {
        switch(clmnid) {
        case INTIDX:    rec->lnk.mkey =  *((uint64_t*)value); break;
        case IMSI:      rec->lnk.key = *((uint64_t*)value); break;
        case INGRESS:   rec->lnk.ingress = *((uint64_t*)value); break;
        case EGRESS:    rec->lnk.egress = *((uint64_t*)value); break;
        case THRESHOLD: rec->lnk.threshold = *((uint64_t*)value); break;
        case ACTIVE:    rec->lnk.stat.type = *((uint64_t*)value); break;
        }
    } else if (type == mx::Notify::TYPE_TEXT) {
        uint8_t ipv[16] = {0}; unsigned int ipv_len(16);

        switch(clmnid) {
        case UEIPV4:
            if (Misc::GetIpv4((const char*)value, ipv, &ipv_len) == RETIPV4) { memcpy(&rec->lnk.ipv4, ipv, 4); }
            break;
        case UEIPV6:
            if (Misc::GetIpv6((const char*)value, ipv, &ipv_len) == RETIPV6) { memcpy(&rec->lnk.ipv6, ipv, 16); }
            break;
        case NASIPV:
            if (Misc::GetIpv4((const char*)value, ipv, &ipv_len) == RETIPV4) { memcpy(&rec->lnk.nasipv, ipv, 4); }
            break;
        case POLICY:
            memcpy(&rec->lnk.policy, (const char*)value, MIN(sizeof(rec->lnk.policy)-1, strlen((const char*)value))); break;
            break;
        }
    }
    return(0);
}
//
void* Diameter::BinLogLoop(void* arg){
    Diameter* inst = (Diameter*)arg;
    int err, nval, ret;
    short sval;
    mx::Event* event = NULL;
    MIXIBINLOG_CLIENT::NotifyImpl notify;
    MIXIBINLOG_CLIENT::record_t record;
    
    // instantiate.
    ret = mx::Event::CreateEvent(
        TBL_DIAMETER,
        inst->dbhost_.c_str(),
        inst->dbuser_.c_str(),
        inst->dbpswd_.c_str(),
        inst->diameter_binlog_.c_str(),
        inst->diameter_binlog_position_,
        &event
    );
    if (ret) {
        throw std::runtime_error("failed. CreateEvent");
    }

    // mysql binlog api
    while(!Module::ABORT()){
        bzero(&record, sizeof(record));
        if (event->Read(&notify, &record) != 0) {
            break;
        }
        // processing by event type
        if (record.evt == 30) {
            // [INSERT]
            record.lnk.stat.type = LINKMAPTYPE_UP;
            inst->OperateTable(&record.lnk);
        } else if (record.evt == 31) {
            // [UPDATE]
            if (record.lnk.stat.type == 0){
                record.lnk.stat.type = LINKMAPTYPE_RM;
                inst->OperateTable(&record.lnk);
            }else{
                record.lnk.stat.type = LINKMAPTYPE_UP;
                inst->OperateTable(&record.lnk);
            }
        } else if (record.evt == 32) {
            // [DELETE]
            record.lnk.stat.type = LINKMAPTYPE_RM;
            inst->OperateTable(&record.lnk);
        }
        usleep(10);
    }
    // cleanup
    ret = mx::Event::ReleaseEvent(
        &event
    );
    return((void*)NULL);
}

