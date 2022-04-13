#pragma once

namespace MIXIBINLOG {
    //
    class Notify {
    public:
        enum {
            TYPE_MIN = 0,
            TYPE_NUMERIC,
            TYPE_TEXT,
            TYPE_MAX
        };
    public:
        virtual int OnNotify(
            int clmnid,
            int type,
            void* value,
            int len,
            void* data
        ) = 0;
    };  // class Notify
    //
    class Event {
    public:
        static int CreateEvent(
            const char* table,
            const char* host,
            const char* user,
            const char* pwd,
            const char* binlogfile,
            unsigned long long binlogpos,
            Event** ppevent
        );
        static int ReleaseEvent(
            Event** ppevent
        );
    public:
        virtual int Read(
            Notify* pnotify,
            void* data
        ) = 0;
    }; // class Event
}; // namespace MIXIBINLOG