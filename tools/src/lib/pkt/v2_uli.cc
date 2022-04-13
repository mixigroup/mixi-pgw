//
// Created by mixi on 2017/01/13.
//

#include "mixipgw_tools_def.h"
#include "lib/logger.hpp"
#include "lib/module.hpp"
#include "lib/packet/gtpc_items.hpp"
//
namespace MIXIPGW_TOOLS{
  Uli::Uli(){
      memset(&uli_,0,sizeof(uli_));
      uli_.head.type = type();
  }
  Uli::Uli(void* src,size_t len){
      attach(src,len);
  }
  Uli::Uli(gtpc_uli_cgi_ptr cgi, gtpc_uli_sai_ptr sai, gtpc_uli_rai_ptr rai, gtpc_uli_tai_ptr tai, gtpc_uli_ecgi_ptr ecgi, gtpc_uli_lai_ptr lai){
      memset(&uli_,0,sizeof(uli_));
      set(cgi, sai, rai, tai, ecgi, lai);
      uli_.head.type = type();
  }
  Uli::~Uli(){ }
  void Uli::set(gtpc_uli_cgi_ptr cgi, gtpc_uli_sai_ptr sai, gtpc_uli_rai_ptr rai, gtpc_uli_tai_ptr tai, gtpc_uli_ecgi_ptr ecgi, gtpc_uli_lai_ptr lai){
      if (Module::VERBOSE() > PCAPLEVEL_PARSE){
          Logger::LOGINF("Uli::set(cgi:%p, sai:%p, rai:%p, tai:%p, ecgi:%p, lai:%p)", cgi,sai,rai,tai,ecgi,lai);
      }
      size_t offset = 0;
      uli_.head.length = 0;
      uli_.c.flags = 0;
#define ATTACH_ULI_BLOCK(t,o) \
            if (t != NULL){\
                uli_.c.bit.t = 1;\
                memcpy(&uli_.blk[o], t, sizeof(*t));\
                o += sizeof(*t);\
            }
      //
      ATTACH_ULI_BLOCK(cgi, offset);
      ATTACH_ULI_BLOCK(sai, offset);
      ATTACH_ULI_BLOCK(rai, offset);
      ATTACH_ULI_BLOCK(tai, offset);
      ATTACH_ULI_BLOCK(ecgi,offset);
      ATTACH_ULI_BLOCK(lai, offset);
#undef ATTACH_ULI_BLOCK
      //
      uli_.head.length = htons(offset + 1);
  }
  int Uli::type(void) const{
      return(GTPC_TYPE_ULI);
  }
  void* Uli::ref(void) const{
      return((void*)&uli_);
  }
  size_t Uli::len(void) const{
      return(ntohs(uli_.head.length) + sizeof(gtpc_comm_header_t));
  }
  int Uli::attach(void* src,size_t len){
      gtpc_uli_ptr p = (gtpc_uli_ptr)src;
      if (Module::VERBOSE() > PCAPLEVEL_PARSE){
          Logger::LOGINF("Uli::attach. (%d)..%d", len, sizeof(gtpc_uli_t));
      }
      if (len < 12){    // dg 10-1-173:User Location Information
          Logger::LOGERR("Uli::attach failed.(%d, %d)", len, sizeof(gtpc_comm_header_t));
          return(RETERR);
      }
      if (p->head.type == type() && p->head.length >= htons(8)){
          memcpy(&uli_, p, MIN(sizeof(uli_), len));
          return(RETOK);
      }
      Logger::LOGERR("Uli::attach... failed.(%d, %d)", p->head.length, p->head.type);
      return(RETERR);
  }
}; // namespace MIXIPGW_TOOLS