//
// Created by mixi on 2017/06/22.
//
#include "mysql.h"
#include "caching_sha2_passwordopt-vars.h"
#include "libbinlogevents/include/compression/iterator.h"
#include "sql_common.h"
#include "binlog.h"
#include "binlog_event.h"
#include "field_types.h"
#include "rows_event.h"
#include "sql/log_event.h"
#include "sql/rpl_record.h"

#include "../mixipgw_mod_diameter.hpp"


using namespace MIXIPGW_TOOLS;
using namespace binary_log;

#define TBL_DIAMETER  ("diameter")

typedef enum DIAMETER_COL_{
    INTIDX = 0,
    IMSI,   UEIPV4,     UEIPV6,     NASIPV,
    POLICY, INGRESS,    EGRESS,     THRESHOLD,
    ACTIVE, UPDATED_AT,
    MAX
}DIAMETER_COL;
// instanciate
ulong opt_server_id_mask = 0;
char server_version[SERVER_VERSION_LENGTH];
ulong filter_server_id = 0;
bool force_opt = false, short_form = false, idempotent_mode = false;
Sid_map *global_sid_map = nullptr;
Checkable_rwlock *global_sid_lock = nullptr;
Gtid_set *gtid_set_included = nullptr;
Gtid_set *gtid_set_excluded = nullptr;

// need implements
void error(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  fprintf(stderr, "\n");
  va_end(args);
}
//
void Transaction_payload_log_event::print(
    FILE *,
    PRINT_EVENT_INFO *info
) const { }

//
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
        diameter_link_t& lnk,
        table_def* td
    )
    {
        const uchar *value = m_rows_buf;
        size_t length;
        char txt[256] = {0};
        uint8_t ipv[16] = {0}; unsigned int ipv_len(16);
        value += (bitmap_bits_set(&m_cols) + 7) / 8;

        for (size_t i = 0; i < td->size(); i++) {
            if (bitmap_is_set(&m_cols, i) == 0) { continue; }
            uint64_t nval = (uint64_t)*((long long*)(value));
            //
            switch(td->type(i)) {
            case MYSQL_TYPE_LONGLONG:
                switch(i) {
                case INTIDX:    lnk.mkey = nval; break;
                case IMSI:      lnk.key = nval; break; 
                case INGRESS:   lnk.ingress = nval; break; 
                case EGRESS:    lnk.egress = nval; break; 
                case THRESHOLD: lnk.threshold = nval; break; 
                case ACTIVE:    lnk.stat.type = nval; break;
                }
                length = 8;
                break;
            case MYSQL_TYPE_TIMESTAMP:
            case MYSQL_TYPE_TIMESTAMP2:
                length = 4 + 1 / 2;
                break;
            case MYSQL_TYPE_VARCHAR:
            case MYSQL_TYPE_VAR_STRING:
                if (td->field_metadata(i) >= 256) {
                    error("not support above 256 bytes: %lu", td->field_metadata(i));
                    throw std::runtime_error("not support above 256 bytes(varchar)");
                }
                length = (*value);
                memset(txt, 0, sizeof(txt));
                memcpy(txt, value + 1, MIN(sizeof(txt) -1, length));
                ipv_len = sizeof(ipv);
                switch(i) {
                case UEIPV4: if (Misc::GetIpv4(txt, ipv, &ipv_len) == RETIPV4) { memcpy(&lnk.ipv4, ipv, 4); } break;
                case UEIPV6: if (Misc::GetIpv6(txt, ipv, &ipv_len) == RETIPV6) { memcpy(&lnk.ipv6, ipv, 16); } break;
                case NASIPV: if (Misc::GetIpv4(txt, ipv, &ipv_len) == RETIPV4) { memcpy(&lnk.nasipv, ipv, 4); } break;
                case POLICY: memcpy(&lnk.policy, txt, MIN(sizeof(lnk.policy)-1, strlen(txt))); break;
                }
                length ++;
                break;
            default:
                error("not support type: %lu", td->type(i));
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


void* Diameter::BinLogLoop(void* arg){
    Diameter* inst = (Diameter*)arg;
    int err, nval, ret;
    short sval;
    // initialize mysql client
    MYSQL* cli = mysql_init(NULL);
    mysql_options(cli, MYSQL_OPT_CONNECT_ATTR_RESET, nullptr);
    mysql_options4(cli, MYSQL_OPT_CONNECT_ATTR_ADD, "program_name", "mixi-pgw-mod-radius");
    mysql_options4(cli, MYSQL_OPT_CONNECT_ATTR_ADD, "_client_role", "binary_log_listener");
    set_server_public_key(cli);
    set_get_server_public_key_option(cli);
    
    // connect to replication master.
    if (!mysql_real_connect(
            cli,
            inst->dbhost_.c_str(),
            inst->dbuser_.c_str(),
            inst->dbpswd_.c_str(),
            NULL,
            std::atoi(inst->dbport_.c_str()),
            NULL,
            0))
    {
        Logger::LOGERR("mysql_real_connect(%s)", mysql_error(cli));
        throw std::runtime_error("failed. mysql_real_connect");
    }

    if(mysql_query(cli, "SET @master_binlog_checksum='NONE', @source_binlog_checksum = 'NONE'")) {
        Logger::LOGERR("mysql_query(%s) -  @master_binlog_checksum NONE", mysql_error(cli));
        throw std::runtime_error("mysql_query master_binlog_checksum NONE");
    }
    if (!cli->methods) {
        Logger::LOGERR("cli->methods is NULL", mysql_error(cli));
        throw std::runtime_error("cli->methods is NULL");
    }

    MYSQL_RPL rpl = {
        0,
        inst->diameter_binlog_.c_str(),
        (uint64_t)inst->diameter_binlog_position_,
        1,
        MYSQL_RPL_SKIP_HEARTBEAT,
        0,
        NULL,
        NULL,
        0,
        NULL
    };
    if (mysql_binlog_open(cli, &rpl)) {
        Logger::LOGERR("mysql_binlog_open", mysql_error(cli));
        throw std::runtime_error("mysql_binlog_open");
    }

    const Format_description_event* des_ev = NULL;
    Table_map_log_event* tbl_ev = NULL;

    // mysql binlog api
    while(!Module::ABORT()){
        if (mysql_binlog_fetch(cli, &rpl)) {
            error("Got error reading packet from server: %s", mysql_error(cli));
            throw std::runtime_error("mysql_binlog_fetch");
        } else if (rpl.size == 0) {
            error("EOF : %s", mysql_error(cli));
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
                    if (strncasecmp(TBL_DIAMETER, tbl_ev->m_tblnam.c_str(), strlen(TBL_DIAMETER))== 0){
                        printf("target >> %s\n", TBL_DIAMETER);
                    }
                }
                break;
            case WRITE_ROWS_EVENT:
            case UPDATE_ROWS_EVENT:
            case DELETE_ROWS_EVENT:
                if (tbl_ev) {
                    ParseEvent event(ev, des_ev);
                    diameter_link_t lnk;
                    bzero(&lnk, sizeof(lnk));
                    if (event.parse(lnk, tbl_ev->create_table_def())) {
                        if (type == WRITE_ROWS_EVENT) {
                            // [INSERT]
                            lnk.stat.type = LINKMAPTYPE_UP;
                            inst->OperateTable(&lnk);
                        } else if (type == UPDATE_ROWS_EVENT) {
                            // [UPDATE]
                            if (lnk.stat.type == 0){
                                lnk.stat.type = LINKMAPTYPE_RM;
                                inst->OperateTable(&lnk);
                            }else{
                                lnk.stat.type = LINKMAPTYPE_UP;
                                inst->OperateTable(&lnk);
                            }
                        } else if (type == DELETE_ROWS_EVENT) {
                            // [DELETE]
                            lnk.stat.type = LINKMAPTYPE_RM;
                            inst->OperateTable(&lnk);
                        }
                    }
                }
                break;
            default:    // dont care.
                break;
        }
        usleep(1000);
    }
    return((void*)NULL);
}

