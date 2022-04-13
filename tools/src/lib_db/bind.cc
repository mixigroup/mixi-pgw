//
// Created by mixi on 2016/12/16.
//

#include "lib_db/mysql.hpp"
#include "lib/logger.hpp"

using namespace MIXIPGW_TOOLS;

namespace MIXIPGW_TOOLS{
  template<typename T> static enum_field_types type(void){
      if (typeid(T) == typeid(short) || typeid(T) == typeid(int) || typeid(T) == typeid(long) || typeid(T) == typeid(long long) ||
          typeid(T) == typeid(unsigned short) || typeid(T) == typeid(unsigned int) || typeid(T) == typeid(unsigned long) || typeid(T) == typeid(unsigned long long)) {
          return (MYSQL_TYPE_LONGLONG);
      } else if (typeid(T) == typeid(std::string)) {
          return (MYSQL_TYPE_STRING);
      } else {
          return (MYSQL_TYPE_STRING);
      }
  }
  void MysqlBind::Clear(void){
      bindidx_= 0;
      memset(bind_, 0, sizeof(bind_));
      memset(erro_, 0, sizeof(erro_));
      memset(length_,0,sizeof(length_));
      memset(null_, 0, sizeof(null_));
      memset(data_, 0, sizeof(data_));
      //
      for(size_t n = 0;n < MAXBIND;n++){
          bind_[n].error = &erro_[n];
          bind_[n].length = &length_[n];
          bind_[n].is_null = &null_[n];
          bind_[n].buffer = &data_[n];
      }
  }
  MysqlBind::MysqlBind(){
      Clear();
  }
  MysqlBind::MysqlBind(int64_t bind_param){ Clear();  Bind(bind_param); }
  MysqlBind::MysqlBind(uint64_t bind_param){ Clear(); Bind(bind_param); }
  MysqlBind::MysqlBind(const MysqlBind& cp){
      sql_ = cp.sql_;
      memcpy(bind_, cp.bind_, sizeof(bind_));
      memcpy(erro_, cp.erro_, sizeof(erro_));
      memcpy(null_, cp.null_, sizeof(null_));
      memcpy(length_, cp.length_, sizeof(length_));
      memcpy(data_, cp.data_, sizeof(data_));
      bindidx_ = cp.bindidx_;
  }
  MysqlBind::MysqlBind(const char *sql) {
      Clear();
      sql_ = (sql!=NULL?sql:"");
  }

  MysqlBind::~MysqlBind() {
      Clear();
  }
  void MysqlBind::BindField(MYSQL_FIELD *f) {
      if (bindidx_ + 1 >= MAXBIND) {
          Logger::LOGERR("[ERR]MysqlBind<T>::bind_field.. bindidx[%d] type: %d, len: %d",bindidx_, f->type, f->length);
          Logger::LOGERR("[ERR]MysqlBind<T>::bind_field.. sql(%s)", sql_.c_str());
          return;
      }
      bind_[bindidx_].buffer_length = f->length;
      bind_[bindidx_].buffer_type = f->type;
      bind_[bindidx_].is_null = 0;
      //
      bindidx_++;
  }
  MYSQL_BIND *MysqlBind::RefBind(void) {
      return (bind_);
  }
  std::string MysqlBind::Sql(void) {
      return(sql_);
  }
#ifdef __APPLE__
  void MysqlBind::Bind(int64_t u){
      Bind((uint64_t) u);
  }
#endif
  void MysqlBind::Bind(uint64_t u){
      u_long s = 0;
      if (bindidx_ + 1 >= MAXBIND) { return; }
      s = sizeof(uint64_t);
      memcpy(bind_[bindidx_].buffer , &u, s);
      bind_[bindidx_].buffer_length = s;
      bind_[bindidx_].buffer_type = type<unsigned long long>();
      //
      bindidx_++;
  }
  void MysqlBind::Bind(long l){
      u_long s = 0;
      if (bindidx_ + 1 >= MAXBIND) { return; }
      s = sizeof(long);
      memcpy(bind_[bindidx_].buffer , &l, s);
      bind_[bindidx_].buffer_length = s;
      bind_[bindidx_].buffer_type = type<unsigned long long>();
      //
      bindidx_++;
  }
  void MysqlBind::Bind(const char* c){
      u_long s = 0;
      if (bindidx_ + 1 >= MAXBIND || !c) { return; }
      s = strlen(c);
      memcpy(bind_[bindidx_].buffer , c, s);
      bind_[bindidx_].buffer_length = s;
      *(bind_[bindidx_].length) = s;
      *(bind_[bindidx_].is_null) = false;
      bind_[bindidx_].buffer_type = type<std::string>();
      //
      bindidx_++;
  }
  void MysqlBind::Bind(std::string t){
      Bind(t.c_str());
  }

  enum_field_types MysqlBind::Ltype(enum_field_types e) {
      switch (e) {
          case MYSQL_TYPE_DECIMAL:
          case MYSQL_TYPE_TINY:
          case MYSQL_TYPE_SHORT:
          case MYSQL_TYPE_LONG:
          case MYSQL_TYPE_FLOAT:
          case MYSQL_TYPE_DOUBLE:
          case MYSQL_TYPE_LONGLONG:
          case MYSQL_TYPE_INT24:
          case MYSQL_TYPE_ENUM:
              return (MYSQL_TYPE_LONGLONG);
          case MYSQL_TYPE_DATE:
          case MYSQL_TYPE_TIME:
          case MYSQL_TYPE_TIME2:
          case MYSQL_TYPE_DATETIME:
          case MYSQL_TYPE_DATETIME2:
          case MYSQL_TYPE_YEAR:
          case MYSQL_TYPE_NEWDATE:
          case MYSQL_TYPE_TIMESTAMP:
          case MYSQL_TYPE_TIMESTAMP2:
              return (MYSQL_TYPE_DATE);
          case MYSQL_TYPE_NEWDECIMAL:
              return (MYSQL_TYPE_NEWDECIMAL);
          default:
              return (MYSQL_TYPE_STRING);
      }
  }
}; // namespace MIXIPGW_TOOLS
