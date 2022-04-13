//
// Created by mixi on 2016/12/16.
//
#include "lib_db/mysql.hpp"
#include "lib/logger.hpp"
#include "precompile.cc"

using namespace MIXIPGW_TOOLS;

namespace MIXIPGW_TOOLS{
  void MysqlStmt::Setup(Mysql *con, MysqlBindIf *bind){
      MYSQL_BIND *pbind = NULL;
      std::string sql = bind->Sql().c_str();
      handle_ = con->dbhandle_;
      //
      while (1) {
          if ((stmt_handle_ = mysql_stmt_init(handle_)) == NULL) {
              Logger::LOGERR("failed. mysql_stmt_init(%s)", mysql_error(handle_));
              break;
          }
          if (mysql_stmt_prepare(stmt_handle_, sql.c_str(), strlen(sql.c_str())) != 0) {
              if (mysql_errno(handle_) == 2006){
                  if (mysql_stmt_prepare(stmt_handle_, sql.c_str(), strlen(sql.c_str())) != 0) {
                      Logger::LOGERR("failed. mysql_stmt_prepare : %s / sql : %s", mysql_stmt_error(stmt_handle_), sql.c_str());
                      break;
                  }
              }else{ break; }
          }
          if (!prepared_only_){
              if ((pbind = bind->RefBind()) == NULL) { break; }
              if (mysql_stmt_bind_param(stmt_handle_, pbind) != 0) { break; }
          }
          state_ = RETOK;
          break;
      }
      if (state_ != RETOK && stmt_handle_ != NULL) {
          Logger::LOGERR("stmt    : state : %s", mysql_stmt_error(stmt_handle_));
          Logger::LOGERR("stmt sql: %s", sql.c_str());
      }else if (state_ != RETOK){
          Logger::LOGERR("failed. conn : state : %s", mysql_error(handle_));
          Logger::LOGERR("failed. conn sql: %s", sql.c_str());
      }
  }
  MysqlStmt::MysqlStmt(Mysql *con, MysqlBindIf *bind, bool prepared_only) :
          stmt_handle_(NULL), handle_(NULL), state_(RETERR), column_cnt_(0), prepared_only_(prepared_only),local_bind_(NULL) {
      Setup(con, bind);
  }
  MysqlStmt::MysqlStmt(Mysql *con, const char* sql):
          stmt_handle_(NULL), handle_(NULL), state_(RETERR), column_cnt_(0), prepared_only_(true),local_bind_(new MysqlBind(sql)){
      Setup(con, local_bind_);
  }
  MysqlStmt::~MysqlStmt() {
      if (stmt_handle_ != NULL) {
          mysql_stmt_close(stmt_handle_);
      }
      stmt_handle_ = NULL;
      if (local_bind_ != NULL){
          delete local_bind_;
      }
      local_bind_ = NULL;
  }
  int MysqlStmt::Execute(onResult fnc, void *u, MysqlBindIf* bind) {
      MYSQL_FIELD *fields = NULL;
      MysqlBind rbind("");
      uint64_t counter = 0;
      MYSQL_RES* stmt_meta = NULL;
      MYSQL_BIND *pbind = NULL;

      if (prepared_only_ && bind){
          if (mysql_stmt_reset(stmt_handle_) != 0) { return(RETERR); }
          if ((pbind = bind->RefBind()) == NULL) { return(RETERR); }
          if (mysql_stmt_bind_param(stmt_handle_, pbind) != 0) { return(RETERR); }
      }
      if (fnc == NULL){
          if ((state_ = mysql_stmt_execute(stmt_handle_)) != RETOK){
              Logger::LOGERR("MysqlStmt::execute/stmt : state :--++ %s", mysql_stmt_error(stmt_handle_));
              Logger::LOGERR("MysqlStmt::execute/failed. conn :--++ state : %s", mysql_error(handle_));
          }
          return(state_);
      }
      state_ = RETERR;
      //
      while (1) {
          if (mysql_stmt_execute(stmt_handle_) != 0) { break; }
          if ((stmt_meta = mysql_stmt_result_metadata(stmt_handle_)) == NULL) { break; }
          if ((column_cnt_ = mysql_num_fields(stmt_meta)) == 0) { break; }
          if ((fields = mysql_fetch_fields(stmt_meta)) == NULL) { break; }
          // bind all columns.
          for (size_t n = 0; n < column_cnt_; n++) {
              rbind.BindField(&fields[n]);
          }
          // bind result buffer.
          if (mysql_stmt_bind_result(stmt_handle_, rbind.RefBind()) != 0) { break; }
          if (mysql_stmt_store_result(stmt_handle_) != 0) { break; }
          // fetch all records.
          while (mysql_stmt_fetch(stmt_handle_) == 0) {
              RECORDS records;
              MYSQL_BIND *r = rbind.RefBind();
              for (size_t n = 0; n < column_cnt_; n++) {
                  bool error = *r[n].error;
                  enum_field_types et = rbind.Ltype(r[n].buffer_type);
                  std::string cnm(fields[n].name, fields[n].name_length);
                  if (et == MYSQL_TYPE_STRING) {
                      std::string txt((char *) r[n].buffer, r[n].buffer_length);
                      records[cnm.c_str()] = Record(txt.c_str(), 0);
                  } else if (et == MYSQL_TYPE_DATE) {
                      char bf[64] = {0};

                      snprintf(bf, sizeof(bf) - 1, "%04d/%02d/%02d %02d:%02d:%02d",
                               ((MYSQL_TIME *) r[n].buffer)->year,
                               ((MYSQL_TIME *) r[n].buffer)->month,
                               ((MYSQL_TIME *) r[n].buffer)->day,
                               ((MYSQL_TIME *) r[n].buffer)->hour,
                               ((MYSQL_TIME *) r[n].buffer)->minute,
                               ((MYSQL_TIME *) r[n].buffer)->second
                      );
                      records[cnm.c_str()] = Record(bf, 0);
                  } else if (et == MYSQL_TYPE_NEWDECIMAL){
                      std::string txt((char *) r[n].buffer, r[n].buffer_length);
                      uint64_t ndec = strtoull(txt.c_str(), NULL, 10);
                      records[cnm.c_str()] = Record(txt.c_str(), ndec);
                  } else {
                      records[cnm.c_str()] = Record(NULL, (uint64_t)*((long long *) r[n].buffer));
                  }
              }
              if (fnc) { fnc(counter, &records, u); }
          }
          state_ = RETOK;
          if (mysql_stmt_errno(stmt_handle_) != 0){
              Logger::LOGERR("MysqlStmt::execute/stmt : state.. : %s", mysql_stmt_error(stmt_handle_));
          }
          if (mysql_errno(handle_) != 0){
              Logger::LOGERR("MysqlStmt::execute/failed. conn.. : state : %s", mysql_error(handle_));
          }
          break;
      }
      if (state_ != RETOK){
          if (mysql_stmt_errno(stmt_handle_) != 0){
              Logger::LOGERR("MysqlStmt::execute/stmt : state : %s", mysql_stmt_error(stmt_handle_));
          }
          if (mysql_errno(handle_) != 0){
              Logger::LOGERR("MysqlStmt::execute/failed. conn : state : %s", mysql_error(handle_));
          }
      }
      if (stmt_meta != NULL) {
          mysql_free_result(stmt_meta);
      }
      //
      return (RETOK);
  }
  int MysqlStmt::State(void) { return (state_); }

  //
  void MysqlStmt::Dump(ITRRECORDS i) {
      if ((i->second).type_ == Record::String || (i->second).type_ == Record::Both) {
          Logger::LOGINF("%s : %s", (i->first).c_str(), (i->second).sval_.c_str());
      } else {
          Logger::LOGINF("%s : " FMT_LLU, (i->first).c_str(), (i->second).nval_);
      }
  }
}; // namespace MIXIPGW_TOOLS
