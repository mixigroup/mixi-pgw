//
// Created by mixi on 2017/04/21.
//

#ifndef MIXIPGW_TOOLS_MYSQL_HPP
#define MIXIPGW_TOOLS_MYSQL_HPP

#include "mixipgw_tools_def.h"
#include <typeinfo>
#include <string>
#include <map>
#include <mysql.h>


namespace MIXIPGW_TOOLS{
  class Record {
  public:
      enum RecordType { String, Numeric, Both, };
  public:
      RecordType type_;
      std::string sval_;
      uint64_t nval_;
  public:
      Record(const char *sval, uint64_t nval);
      Record(const Record &cpy);
      Record();
      std::string Text(void) const;
      uint64_t Dec(void) const;
      bool IsString(void) const;
  }; // class Record

  typedef std::map<std::string, Record> RECORDS;
  typedef std::map<std::string, Record>::iterator ITRRECORDS;
  typedef std::map<std::string, Record>::const_iterator CITRRECORDS;
  //
  class MysqlCfg {
  public:
      MysqlCfg(const char *, const char *, const char *, const char *, const uint32_t , const uint32_t );
      MysqlCfg(const std::string&,const std::string&,const std::string&,const std::string&);
      MysqlCfg(MysqlCfg &);
  public:
      static void Prefix(const char*);
      static std::string& Prefix(void);
  public:
      std::string host_;
      std::string user_;
      std::string pswd_;
      std::string inst_;
      uint32_t flag_;
      uint32_t port_;
  private:
      MysqlCfg();       // don't use default constructor
  }; // class MysqlCfg
  class MysqlBindIf {
  public:
      virtual void BindField(MYSQL_FIELD *f) = 0;
      virtual MYSQL_BIND *RefBind(void) = 0;
      virtual std::string Sql(void) = 0;

  }; // class MysqlBindIf
  //
  class MysqlBind: public MysqlBindIf {
  public:
      static const size_t MAXBIND = 64;
      static const size_t MAXCOLLEN= 256;
      MysqlBind(int64_t);
      MysqlBind(uint64_t);
      MysqlBind(const char*);
      virtual ~MysqlBind();
  public:
      virtual void BindField(MYSQL_FIELD *f);
      virtual MYSQL_BIND *RefBind(void);
      virtual std::string Sql(void);
      static enum_field_types Ltype(enum_field_types e);

      void Bind(uint64_t);
      void Bind(long);
#ifdef __APPLE__
      void Bind(int64_t);
#endif
      void Bind(const char*);
      void Bind(std::string);
      MysqlBind& operator()(){ return(*this); }
  private:
      MysqlBind();  // can't instanciate.
      MysqlBind(const MysqlBind&);
      void Clear(void);
  public:
      MYSQL_BIND  bind_[MAXBIND];
      bool     erro_[MAXBIND];
      bool     null_[MAXBIND];
      unsigned long   length_[MAXBIND];
      char        data_[MAXBIND][MAXCOLLEN];
      size_t      bindidx_;
      std::string sql_;
  }; // class MysqlBind
  //
  class Mysql {
  public:
      static void Init(void);
  public:
      Mysql(MysqlCfg *);
      virtual ~Mysql();
  public:
      int State(void);
      int Query(const char *);
  private:
      Mysql() {}        // can't copy.
  public:
      int Connect(void);// for reconnect.
      void Disconnect(void);
  public:
      MysqlCfg *cfg_;
      MYSQL *dbhandle_;
      int state_;
  };// class Mysql
  typedef int (*onResult)(uint64_t, const RECORDS*, void *);
  //

  class MysqlStmt {
  public:
      MysqlStmt(Mysql*, MysqlBindIf*,bool prepared_only=false);
      MysqlStmt(Mysql*, const char*);
      virtual ~MysqlStmt();
  public:
      int Execute(onResult, void *, MysqlBindIf*);
      int State(void);
      static void Dump(ITRRECORDS i);
  private:
      MysqlStmt(){} // can't copy.
      void Setup(Mysql*, MysqlBindIf*);
  public:
      int state_;
      bool prepared_only_;
      MYSQL *handle_;
      MYSQL_STMT *stmt_handle_;
      MysqlBind  *local_bind_;
      size_t column_cnt_;
  }; // class MysqlStmt
}; // namespace MIXIPGW_TOOLS


#endif //MIXIPGW_TOOLS_MYSQL_HPP
