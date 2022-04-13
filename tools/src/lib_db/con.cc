//
// Created by mixi on 2016/12/16.
//
#include "lib_db/mysql.hpp"
#include "lib/logger.hpp"

using namespace MIXIPGW_TOOLS;

namespace MIXIPGW_TOOLS{
  Mysql::Mysql(MysqlCfg *cfg):dbhandle_(NULL), cfg_(cfg) {
      if ((state_ = Connect()) != 0){
          Logger::LOGERR("failed.connect(%s)", mysql_error(dbhandle_));
      }
  }
  Mysql::~Mysql() { Disconnect(); }
  void Mysql::Init(void) {
      mysql_library_init(0,0,0);
  }
  int Mysql::State(void) { return (state_); }
  int Mysql::Query(const char *sql) { return (mysql_query(dbhandle_, sql)); }
  int Mysql::Connect(void) {
      mysql_thread_init();
      bool reconnect = 1;
      if ((dbhandle_ = mysql_init(NULL)) == NULL) {
          Logger::LOGERR("failed.mysql_init");
          return (RETERR);
      }
      mysql_options(dbhandle_, MYSQL_OPT_RECONNECT, &reconnect);
      if (mysql_real_connect(dbhandle_, cfg_->host_.c_str(),
                             cfg_->user_.c_str(),
                             cfg_->pswd_.c_str(),
                             cfg_->inst_.c_str(),
                             cfg_->port_, NULL, cfg_->flag_)) {
          mysql_set_character_set(dbhandle_, "utf8");
          return (RETOK);
      }
      return (RETERR);
  }
  void Mysql::Disconnect(void) {
      if (dbhandle_ != NULL) {
          mysql_close(dbhandle_);
      }
      dbhandle_ = NULL;
  }
}; // namespace MIXIPGW_TOOLS
