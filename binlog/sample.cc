#include <stdlib.h>
#include <iostream>
#include "binlog.h"

namespace mx = MIXIBINLOG;

namespace MIXIBINLOGSAMPLE {
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
    // record
    typedef struct record {
        int event;
        uint64_t id;
        std::string name;
    } record_t, *record_ptr;
}; // namespace MIXIBINLOGSAMPLE

// notify callback implementation.
int
MIXIBINLOGSAMPLE::NotifyImpl::OnNotify(
    int event,      /* event */
    int clmnid,     /* index of columns  */
    int type,       /* type of value(TYPE_NUMERIC, TYPE_TEXT, etc) */
    void* value,    /* column value */
    int len,        /* length of value */
    void* data      /* user-data */
)
{
    printf(
        "OnNotify(%d, %d, %d, %p, %d, %p)\n",
        event,
        clmnid,
        type,
        value,
        len,
        data
    );
    auto rec = (record_ptr)data;
    if (type == mx::Notify::TYPE_NUMERIC) {
        switch(clmnid) {
        case 0:
            rec->event = event;
            break;
        case 1:
            rec->id = *((uint64_t*)value);
            break;
        }
    } else if (type == mx::Notify::TYPE_TEXT) {
        switch(clmnid) {
        case 2:
            rec->name.assign((const char*)value, len);
            break;
        }
    }
    return(0);
}
//
int main(
    int argc,
    char* argv[])
{
    int ret = 0;
    mx::Event* event = NULL;
    MIXIBINLOGSAMPLE::NotifyImpl notify;
    MIXIBINLOGSAMPLE::record_t record;

    // instantiate.
    ret = mx::Event::CreateEvent(
        "target",
        "127.0.0.1",
        "root",
        "develop",
        "binlog.000001",
        157,
        &event
    );
    if (ret) {
        throw std::runtime_error("failed. CreateEvent");
    }
    // entry.
    record.event = 0;
    while(event->Read(&notify, &record) == 0) {
        if (record.event != 30 &&
            record.event != 31 &&
            record.event != 32)
        {
            continue;
        }
        printf("%d/%d : %s\n",
            (int)record.event,
            (int)record.id,
            record.name.c_str()
        );
        record.event = 0;
    }
    // cleanup
    ret = mx::Event::ReleaseEvent(
        &event
    );
    
    return(0);
}


