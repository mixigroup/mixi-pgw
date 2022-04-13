//
// Created by mixi on 2017/01/10.
//

#include "mixipgw_tools_def.h"
#include "lib/buffer.hpp"
#include "lib/logger.hpp"
#include <assert.h>
using namespace MIXIPGW_TOOLS;

//
namespace MIXIPGW_TOOLS{
  static const uint32_t ALLOCATE_MAXSIZE = (4096);
  static const uint32_t MMHEAD = 0xDEADC0DE;
  static const uint32_t MMFOOT = 0xDEADBEEF;
  //
  static LONGLONG MMSIZE = 0;
  static LONGLONG MMTOTAL_ALLOC = 0;
  static LONGLONG MMTOTAL_ALLOC_COUNT = 0;
  //
  Buffer::Buffer(unsigned len):len_(len),buffer_(AllocateBuffer(len)),lhost_(NULL),host_(NULL) { }
  Buffer::Buffer(const void* src,unsigned len):len_(len), buffer_(AllocateBuffer(len)),lhost_(NULL),host_(NULL) {
      memcpy(buffer_, src, len);
  }
  Buffer::Buffer(const void* src,unsigned len,const char* host, unsigned short port,const char* lhost, unsigned short lport):Buffer(src, len, host, port) {
      lhost_ = lhost==NULL?NULL:strdup(lhost);
      lport_ = lport;
  }
  Buffer::Buffer(const void* src,unsigned len,const char* host, unsigned short port):len_(len), buffer_(AllocateBuffer(len)),lhost_(NULL),host_(NULL) {
      memcpy(buffer_, src, len);
      host_ = host==NULL?NULL:strdup(host);
      port_ = port;
  }
  Buffer::~Buffer() {
      if (buffer_ != NULL) { FreeBuffer(buffer_); }
      buffer_ = NULL;
      if (lhost_ != NULL){ free(lhost_); }
      lhost_ = NULL;
      if (host_ != NULL){ free(host_); }
      host_ = NULL;
  }
  void *Buffer::Ref(unsigned* len) {
      if (len != NULL){ (*len) = len_; }
      return (buffer_);
  }
  unsigned short Buffer::Len(void){ return(len_); }
  const char* Buffer::Host(void){ return(host_?host_:""); };
  unsigned short Buffer::Port(void){ return(port_); }
  const char* Buffer::Lhost(void){ return(lhost_?lhost_:""); };
  unsigned short Buffer::Lport(void){ return(lport_); }
  int Buffer::Stat(unsigned* len){ return(MemoryTest(len)); }
  //
  void* Buffer::AllocateBuffer(unsigned sz){
      unsigned t = sz+(sizeof(uint32_t)*4);
      char* p = (char*)malloc(t);
      if (p == NULL){
          assert(!"malloc failed."); _exit(0);
      }
      if (t >= ALLOCATE_MAXSIZE){
          assert(!"too big buffer size."); _exit(0);
      }
      memset(p, 0, t);
      memcpy(&p[0], &MMHEAD, sizeof(MMHEAD));
      memcpy(&p[4], &sz, sizeof(uint32_t));
      memcpy(&p[t-4], &MMFOOT, sizeof(MMFOOT));
      MMSIZE += (LONGLONG)sz;
      MMTOTAL_ALLOC += (LONGLONG)sz;
      MMTOTAL_ALLOC_COUNT ++;
      //
      if (MMTOTAL_ALLOC >= (LONGLONG)0x7ffffffffffffffe){
          MMTOTAL_ALLOC = 0;
          MMTOTAL_ALLOC_COUNT = 0;
      }
      if (MMSIZE >= (LONGLONG)0x7ffffffffffffffe){
          assert(!"too many memory leaking."); _exit(0);
      }
      return((void*)&p[8]);
  }
  void  Buffer::FreeBuffer(void* bf){
      char* p = (((char*)bf)-8);
      //
      if ((__u8)(p[0]) == (__u8)(((char*)&MMHEAD)[0]) &&
          (__u8)(p[1]) == (__u8)(((char*)&MMHEAD)[1]) &&
          (__u8)(p[2]) == (__u8)(((char*)&MMHEAD)[2]) &&
          (__u8)(p[3]) == (__u8)(((char*)&MMHEAD)[3]))
      {
          uint32_t sz = *((uint32_t*)(&p[4]));
          uint32_t ttl = sz + (sizeof(uint32_t)*4);
          if (sz == 0){ assert(!"broken heap."); _exit(0); }
          if ((__u8)(p[ttl-4]) == (__u8)(((char*)&MMFOOT)[0]) &&
              (__u8)(p[ttl-3]) == (__u8)(((char*)&MMFOOT)[1]) &&
              (__u8)(p[ttl-2]) == (__u8)(((char*)&MMFOOT)[2]) &&
              (__u8)(p[ttl-1]) == (__u8)(((char*)&MMFOOT)[3]))
          {
              free(p);
              MMSIZE -= (LONGLONG)sz;
          }else{
              Logger::LOGERR("invalid free memory.(broken heap)(%08u/%p : %X:%X:%X:%X)", sz, bf, (__u8)p[ttl-4], (__u8)p[ttl-3], (__u8)p[ttl-2], (__u8)p[ttl-1]);
          }
      }else{
          Logger::LOGERR("invalid free memory.(%p : %X:%X:%X:%X)", bf, (__u8)p[0], (__u8)p[1], (__u8)p[2], (__u8)p[3]);
      }
  }
  int Buffer::MemoryTest(unsigned* len){
      Logger::LOGINF("leak: %04lld/ total_alloc: %08lld/ total_alloc_count: %08lld", MMSIZE, MMTOTAL_ALLOC, MMTOTAL_ALLOC_COUNT);
      if (len != NULL){
          (*len) = MMSIZE;
      }
      return(MMSIZE==0?RETOK:RETERR);
  }

}; // namespace MIXIPGW_TOOLS
