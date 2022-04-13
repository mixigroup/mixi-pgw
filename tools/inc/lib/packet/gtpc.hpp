//
// Created by mixi on 2017/01/10.
//

#ifndef MIXIPGW_TOOLS_GTPPACKET_H
#define MIXIPGW_TOOLS_GTPPACKET_H
#include "lib/const/gtpc.h"
#include <vector>
#include <string>

// gtpc version 2.
// 3GPP TS 29.274v11.5.0
namespace MIXIPGW_TOOLS{
  class Buffer;
  // packet generator
  class GtpcItem;
  typedef std::vector<Buffer*>  GTPCITEMS;
  typedef std::vector<Buffer*>::iterator GTPCITEMITR;
  typedef int (*iterate_gtpc)(GtpcItem*, void*);
  // total GTPC packet
  class GtpcPkt{
  public:
      GtpcPkt(int);
      virtual ~GtpcPkt();
  public:
      int append(GtpcItem&);
      int append(const GtpcItem*);
      int append(const GtpcItem&);
      size_t len(void);
      void*  ref(void);
      void   dump(void);
      void   iterate(iterate_gtpc,void*);
      template<typename T> T* find(void);
  public:
      static void dump_impl(const char*, const void*, int);
      static GtpcPkt* attach(void*,size_t);
      static int attach_impl(GtpcPkt*, void*,size_t);
      static std::string digits(gtpc_numberdigit_ptr,size_t, __u8 mask=GTPC_DIGITS_MASKOFF);
      static int StrSplit(const char* , const char* ,std::vector<std::string>& );
  protected:
      Buffer*         buffer_;
      size_t          pktlen_;
      gtpc_header_ptr header_;
      int             type_;
      GTPCITEMS       items_;
  private:
      GtpcPkt();
  }; // class GtpcGen

  // interface of gtpc packet item.
  class GtpcItem{
  public:
      static int type(void*,size_t);
  public:
      virtual int type(void) const = 0;
      virtual size_t len(void) const = 0;
      virtual void* ref(void) const = 0;
      virtual int attach(void*,size_t) = 0;
  public:
      virtual void  dump(const char*);
      virtual std::string digits(void);
  }; // class GtpcItem
}; // namespace MIXIPGW_TOOLS
#include "src/lib/pkt/v2_gtpcpkt.hh"


#endif //MIXIPGW_TOOLS_GTPPACKET_H
