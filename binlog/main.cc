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
// binlog event parser 
class ParseEvent:
    protected Rows_log_event,
    protected binary_log::Write_rows_event {
public:
    ParseEvent(
        const char* buf,
        const Format_description_event* desc
    ) : binary_log::Rows_event(buf, desc),
        Rows_log_event(buf, desc),
        binary_log::Write_rows_event(buf, desc)
    {}
public:
    bool parse(
        table_def* td
    )
    {
        const uchar *value = m_rows_buf;
        size_t length;
        char txt[256] = {0};
        uint64_t nval = 0;
        value += (bitmap_bits_set(&m_cols) + 7) / 8;
        
        for (size_t i = 0; i < td->size(); i++) {
            if (bitmap_is_set(&m_cols, i) == 0) { continue; }
            // 
            switch(td->type(i)) {
            case MYSQL_TYPE_LONGLONG:
                // numeric value
                length = 8;
                nval = *(uint64_t*)value;
                printf("bigint(%d): %llu\n", (int)i, nval);
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
                printf("varchar(%d): %s\n", (int)i, txt);
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
    Log_event_type get_general_type_code() override {
        return(binary_log::WRITE_ROWS_EVENT);
    }
    void print(FILE *, PRINT_EVENT_INFO *) const override {
        // noop
    }
}; // class ParseEvent 

#define TARGET_TBL  ("target")
#define DB_HOST     ("127.0.0.1")
#define DB_USER     ("root")
#define DB_PWD      ("develop")
#define BINLOG_FILE ("binlog.000001")
#define BINLOG_POS  (154)
//
int main(
    int argc,
    char* argv[])
{
    int err, nval, ret;
    short sval;
    
    // initialize mysql client
    MYSQL* cli = mysql_init(NULL);
    mysql_options(cli, MYSQL_OPT_CONNECT_ATTR_RESET, nullptr);
    mysql_options4(cli, MYSQL_OPT_CONNECT_ATTR_ADD, "program_name", "mixi-pgw-mod-radius");
    mysql_options4(cli, MYSQL_OPT_CONNECT_ATTR_ADD, "_client_role", "binary_log_listener");
    set_server_public_key(cli);
    set_get_server_public_key_option(cli);

    // connect to server.
    if (!mysql_real_connect(
            cli,
            DB_HOST, 
            DB_USER,
            DB_PWD,
            NULL,
            3306,
            NULL,
            0))
    {
        throw std::runtime_error("failed. mysql_real_connect");
    }
    
    if(mysql_query(cli, "SET @master_binlog_checksum='NONE', @source_binlog_checksum = 'NONE'")) {
        throw std::runtime_error("mysql_query master_binlog_checksum NONE");
    }
    if (!cli->methods) {
        throw std::runtime_error("cli->methods is NULL");
    }
    
    MYSQL_RPL rpl = {
        0,
        BINLOG_FILE,
        (uint64_t)BINLOG_POS,
        1,
        MYSQL_RPL_SKIP_HEARTBEAT,
        0,
        NULL,
        NULL,
        0,
        NULL
    };
    if (mysql_binlog_open(cli, &rpl)) {
        throw std::runtime_error("mysql_binlog_open");
    }

    const Format_description_event* des_ev = NULL;
    Table_map_log_event* tbl_ev = NULL;
    
    // main loop. 
    while(true) {
        
        if (mysql_binlog_fetch(cli, &rpl)) {
            throw std::runtime_error("mysql_binlog_fetch");
        } else if (rpl.size == 0) {
            throw std::runtime_error("mysql_binlog_fetch rpl.size == 0");
        }
        Log_event_type type = (Log_event_type)rpl.buffer[1 + EVENT_TYPE_OFFSET];
        const char* ev = (const char*)(rpl.buffer + 1);

        // processing by event type
        switch(type){
            case ROTATE_EVENT:
                printf("ROTATE_EVENT\n");
                break;
            case FORMAT_DESCRIPTION_EVENT:
                printf("FORMAT_DESCRIPTION_EVENT\n");
                if (des_ev) { delete des_ev; }
                des_ev = new Format_description_event(ev, des_ev);
                break;
            case TRANSACTION_PAYLOAD_EVENT:
                printf("TRANSACTION_PAYLOAD_EVENT\n");
                break;
            case TABLE_MAP_EVENT:
                printf("TABLE_MAP_EVENT\n");
                if (des_ev) {
                    if (tbl_ev) { delete tbl_ev; }
                    tbl_ev = new Table_map_log_event(ev, des_ev); 
                    // target table
                    if (strncasecmp(TARGET_TBL, tbl_ev->m_tblnam.c_str(), strlen(TARGET_TBL))== 0){
                        printf("target >> %s\n", TARGET_TBL);
                    }
                }
                break;
            case WRITE_ROWS_EVENT:
            case UPDATE_ROWS_EVENT:
            case DELETE_ROWS_EVENT:
                if (tbl_ev) {
                    ParseEvent event(ev, des_ev);
                    if (event.parse(tbl_ev->create_table_def())) {
                        if (type == WRITE_ROWS_EVENT) {
                        } else if (type == UPDATE_ROWS_EVENT) {
                        } else if (type == DELETE_ROWS_EVENT) {
                        }
                    }
                }
                break;
            default:
                break;
        }
    }
    // cleanup 
    mysql_close(cli);

    return(0);
}


