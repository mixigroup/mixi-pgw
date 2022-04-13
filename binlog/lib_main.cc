#include "binlog.h"
#include <stdlib.h>
#include <errno.h>
#include <string.h>
// stl
#include <iostream>
#include <ostream>
#include <iomanip>
#include <map>
#include <sstream>
#include <algorithm>

#include "mysql.h"
#include "caching_sha2_passwordopt-vars.h"
#include "libbinlogevents/include/compression/iterator.h"
#include "sql_common.h"
#include "binlog.h"
#include "binlog_event.h"
#include "rows_event.h"
#include "sql/log_event.h"
#include "sql/rpl_record.h"

#ifndef MIN
    #define MIN(a,b) (a<b?a:b)
#endif

using namespace binary_log;

// instanciate variable
ulong opt_server_id_mask = 0;
char server_version[SERVER_VERSION_LENGTH];
ulong filter_server_id = 0;
bool force_opt = false, short_form = false, idempotent_mode = false;
Sid_map *global_sid_map = nullptr;
Checkable_rwlock *global_sid_lock = nullptr;
Gtid_set *gtid_set_included = nullptr;
Gtid_set *gtid_set_excluded = nullptr;

// instanciate function 
void error(const char *format, ...) {
}
// instanciate class interface
void Transaction_payload_log_event::print(
    FILE *,
    PRINT_EVENT_INFO *info
) const { }

namespace MIXIBINLOG {
    // binlog event parser
    class ParseEvent:
        protected Rows_log_event,
        protected Write_rows_event {
    public:
        ParseEvent(
            const char* buf,
            const Format_description_event* desc,
            Log_event_type type,
            Notify* pnotify,
            void* data
        ) : type_(type),
            pnotify_(pnotify),
            data_(data),
            Rows_event(buf, desc),
            Rows_log_event(buf, desc),
            Write_rows_event(buf, desc)
        {}
    public:
        bool Parse(
            table_def* td
        )
        {
            const uchar *value = m_rows_buf;
            size_t length;
            char txt[256] = {0};
            uint64_t nval = 0;
            value += (bitmap_bits_set(&m_cols) + 7) / 8;
            if (!pnotify_) {
                return(false);
            }
            //
            for (size_t i = 0; i < td->size(); i++) {
                if (bitmap_is_set(&m_cols, i) == 0) { continue; }
                // 
                switch(td->type(i)) {
                case MYSQL_TYPE_LONGLONG:
                    // numeric value
                    length = 8;
                    nval = *(uint64_t*)value;
                    pnotify_->OnNotify(i, Notify::TYPE_NUMERIC, (void*)value, 8, data_);
                    break;
                case MYSQL_TYPE_TIMESTAMP:
                case MYSQL_TYPE_TIMESTAMP2:
                    length = 4 + 1 / 2;
                    break;
                case MYSQL_TYPE_VARCHAR:
                case MYSQL_TYPE_VAR_STRING:
                    if (td->field_metadata(i) >= 256) {
                        throw std::runtime_error("not support above 256 bytes(varchar)");
                    }
                    length = (*value);
                    memset(txt, 0, sizeof(txt));
                    memcpy(txt, value + 1, MIN(sizeof(txt) -1, length));
                    // string value
                    length ++;
                    pnotify_->OnNotify(i, Notify::TYPE_TEXT, txt, strlen(txt), data_);
                    break;
                default:
                    throw std::runtime_error("not support type");
                    break;
                }
                value += length;
            }
            return(true);
        }
    private:
        Log_event_type type_;
        Notify* pnotify_;
        void* data_;
    private:
        Log_event_type get_general_type_code() override {
            return(type_);
        }
        void print(FILE *, PRINT_EVENT_INFO *) const override {
            // noop
        }
    };  // class ParseEvent
};// namespace MIXIBINLOG

//
namespace MIXIBINLOG {
    //
    class EventImpl: public Event {
    public:
        EventImpl(
            const char* table,
            const char* host,
            const char* user,
            const char* pwd,
            const char* binlogfile,
            unsigned long long binlogpos
        );
        virtual ~EventImpl();
    public:
        virtual int Read(Notify* pnotify, void* data) override;
    public:
        int Connect(
            const char* table,
            const char* host,
            const char* user,
            const char* pwd,
            const char* binlogfile,
            unsigned long long binlogpos
        );
        int DisConnect(void);
    private:
        MYSQL* conn_;
        MYSQL_RPL rpl_;
        const Format_description_event* des_ev_;
        Table_map_log_event* tbl_ev_;
        std::string table_;
    };
};
// interface implement.
int
MIXIBINLOG::Event::CreateEvent(
    const char* table,
    const char* host,
    const char* user,
    const char* pwd,
    const char* binlogfile,
    unsigned long long binlogpos,
    Event** ppevent
)
{
    (*ppevent) = new EventImpl(
        table,
        host,
        user,
        pwd,
        binlogfile,
        binlogpos
    );
    return(0);
}
//
int
MIXIBINLOG::Event::ReleaseEvent(
    Event** ppevent
)
{
    if (*ppevent) {
        delete (EventImpl*)(*ppevent);
    }
    *ppevent = NULL;
    return(0);
}
// ---------------------
// Event Implementaion.
//
MIXIBINLOG::EventImpl::EventImpl(
    const char* table,
    const char* host,
    const char* user,
    const char* pwd,
    const char* binlogfile,
    unsigned long long binlogpos
):
    conn_(NULL),
    des_ev_(NULL),
    tbl_ev_(NULL)
{
    Connect(
        table,
        host,
        user,
        pwd,
        binlogfile,
        binlogpos
    );
}
//
MIXIBINLOG::EventImpl::~EventImpl()
{
    DisConnect();
}
//
int
MIXIBINLOG::EventImpl::Connect(
    const char* table,
    const char* host,
    const char* user,
    const char* pwd,
    const char* binlogfile,
    unsigned long long binlogpos
)
{
    table_ = table?table:"";
    conn_ = mysql_init(NULL);
    mysql_options(conn_, MYSQL_OPT_CONNECT_ATTR_RESET, nullptr);
    mysql_options4(conn_, MYSQL_OPT_CONNECT_ATTR_ADD, "program_name", "mixi-pgw-mod-radius");
    mysql_options4(conn_, MYSQL_OPT_CONNECT_ATTR_ADD, "_client_role", "binary_log_listener");
    set_server_public_key(conn_);
    set_get_server_public_key_option(conn_);

    // connect to server.
    if (!mysql_real_connect(
            conn_,
            host, 
            user,
            pwd,
            NULL,
            3306,
            NULL,
            0))
    {
        throw std::runtime_error("failed. mysql_real_connect");
    }
    
    if(mysql_query(conn_, "SET @master_binlog_checksum='NONE', @source_binlog_checksum = 'NONE'")) {
        throw std::runtime_error("mysql_query master_binlog_checksum NONE");
    }
    if (!conn_->methods) {
        throw std::runtime_error("cli->methods is NULL");
    }

    rpl_.file_name_length = 0;
    rpl_.file_name = binlogfile;
    rpl_.start_position = binlogpos;
    rpl_.server_id = 1;
    rpl_.flags = MYSQL_RPL_SKIP_HEARTBEAT;
    rpl_.gtid_set_encoded_size = 0;
    rpl_.fix_gtid_set = NULL;
    rpl_.gtid_set_arg = NULL;
    rpl_.size = 0;
    rpl_.buffer = NULL;
    //
    if (mysql_binlog_open(conn_, &rpl_)) {
        throw std::runtime_error("mysql_binlog_open");
    }
    return(0);
}
//
int
MIXIBINLOG::EventImpl::DisConnect(void)
{
    if (conn_) {
        mysql_close(conn_);
    }
    return(0);
}
//
int
MIXIBINLOG::EventImpl::Read(
    Notify* pnotify,
    void* data
)
{
    if (mysql_binlog_fetch(conn_, &rpl_)) {
        throw std::runtime_error("mysql_binlog_fetch");
    } else if (rpl_.size == 0) {
        throw std::runtime_error("mysql_binlog_fetch rpl.size == 0");
    }
    Log_event_type type = (Log_event_type)rpl_.buffer[1 + EVENT_TYPE_OFFSET];
    const char* ev = (const char*)(rpl_.buffer + 1);

    // processing by event type
    switch(type){
        case ROTATE_EVENT:
            printf("ROTATE_EVENT\n");
            break;
        case FORMAT_DESCRIPTION_EVENT:
            printf("FORMAT_DESCRIPTION_EVENT\n");
            if (des_ev_) { delete des_ev_; }
            des_ev_ = new Format_description_event(ev, des_ev_);
            break;
        case TRANSACTION_PAYLOAD_EVENT:
            printf("TRANSACTION_PAYLOAD_EVENT\n");
            break;
        case TABLE_MAP_EVENT:
            printf("TABLE_MAP_EVENT\n");
            if (des_ev_)
            {
                if (tbl_ev_) { delete tbl_ev_; }
                tbl_ev_ = new Table_map_log_event(ev, des_ev_); 
                // target table
                if (strncasecmp(table_.c_str(), tbl_ev_->m_tblnam.c_str(), strlen(table_.c_str()))== 0){
                    printf("target >> %s\n", table_.c_str());
                }
            }
            break;
        case WRITE_ROWS_EVENT:
        case UPDATE_ROWS_EVENT:
        case DELETE_ROWS_EVENT:
            if (tbl_ev_)
            {
                ParseEvent pe(ev, des_ev_, type, pnotify, data);
                pe.Parse(tbl_ev_->create_table_def());
            }
            break;
        default:
            break;
    }
    return(0);
}
