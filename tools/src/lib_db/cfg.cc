//
// Created by mixi on 2016/12/16.
//
#include "lib_db/mysql.hpp"
#include "lib/logger.hpp"

using namespace MIXIPGW_TOOLS;

namespace MIXIPGW_TOOLS{
  static std::string prefix_ = "";
  MysqlCfg::MysqlCfg() : host_(""), user_(""), pswd_(""), inst_(""), flag_(0), port_(0) {}
  MysqlCfg::MysqlCfg(const char *host, const char *user, const char *pswd, const char *inst, const uint32_t flag,
                     const uint32_t port) {
      host_ = host ? host : "localhost";
      user_ = user ? user : "root";
      pswd_ = pswd ? pswd : "password";
      inst_ = inst ? inst : "";
      port_ = port ? port : 3306;
      flag_ = flag ? flag : 0;
      if (inst != NULL){
          inst_ += MysqlCfg::Prefix();
      }else if (MysqlCfg::Prefix().empty()){
          inst_ = "mixipgw";
      }
  }
  MysqlCfg::MysqlCfg(const std::string& host,const std::string& user,const std::string& pswd,const std::string& inst){
      host_ = !host.empty() ? host : "localhost";
      user_ = !user.empty() ? user : "root";
      pswd_ = !pswd.empty() ? pswd : "password";
      inst_ = !inst.empty() ? inst : "mixipgw";
      port_ = 3306;
      flag_ = 0;
      if (inst.empty()){
          inst_ += MysqlCfg::Prefix();
      }
  }
  MysqlCfg::MysqlCfg(MysqlCfg &cpy) {
      host_ = cpy.host_;
      user_ = cpy.user_;
      pswd_ = cpy.pswd_;
      inst_ = cpy.inst_;
      flag_ = cpy.flag_;
      port_ = cpy.port_;
  }
  void MysqlCfg::Prefix(const char* prefix){
      if (prefix != NULL){
          prefix_ = prefix;
      }
  }
  std::string& MysqlCfg::Prefix(void){
      return(prefix_);
  }
}; // namespace MIXIPGW_TOOLS
