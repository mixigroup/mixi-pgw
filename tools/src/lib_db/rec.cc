//
// Created by mixi on 2016/12/16.
//
#include "mixipgw_tools_def.h"
#include "lib_db/mysql.hpp"
#include "lib/logger.hpp"


using namespace MIXIPGW_TOOLS;

namespace MIXIPGW_TOOLS{
  Record::Record():type_(String), sval_(""), nval_(0){}
  Record::Record(const char* sval, uint64_t nval){
      sval_ = "";
      if (sval){ sval_ = sval;}
      nval_ = nval;
      if (sval && nval){ type_ = Both;
      }else if (sval){   type_ = String;
      }else{             type_ = Numeric;
      }
  }
  Record::Record(const Record& cpy){
      type_ = cpy.type_;
      sval_ = cpy.sval_;
      nval_ = cpy.nval_;
  }
  bool Record::IsString(void) const{
      return(type_ == String?true:false);
  }
  std::string Record::Text(void) const{
      char bf[64] = {0};
      if (type_ == String){
          return(sval_);
      }else{
          snprintf(bf,sizeof(bf)-1,FMT_LLU, nval_);
      }
      return(std::string(bf));
  }
  uint64_t Record::Dec(void) const{
      if (type_ == String){
          return(0);
      }
      return(nval_);
  }
}; // namespace MIXIPGW_TOOLS
