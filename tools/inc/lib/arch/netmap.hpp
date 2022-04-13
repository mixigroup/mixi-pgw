//
// Created by mixi on 2017/04/24.
//

#ifndef MIXIPGW_TOOLS_ARCH_NETMAP_HPP
#define MIXIPGW_TOOLS_ARCH_NETMAP_HPP
#include "lib/interface/arch_interface.hpp"

namespace MIXIPGW_TOOLS{
  // implemetation for actual requipment.
  // -----
  // architecture dependent
  class Netmap:public ArchIf{
  public:
      Netmap(ProcessParameter*);
      virtual ~Netmap();
  public:
      // Send: netmap architecture : zerocopy + swap index
      virtual int Open(const char*,int);
      virtual int Poll(int,PktInterface*);
      virtual int Send(int,PktInterface*);
  private:
      int SearchValidRing(void);
      int ProcessRing(void* ,void*);
  }; // class Netmap
}; // namespace MIXIPGW_TOOLS

#endif //MIXIPGW_TOOLS_ARCH_NETMAP_HPP
