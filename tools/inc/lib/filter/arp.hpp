//
// Created by mixi on 2017/04/21.
//

#ifndef MIXIPGW_TOOLS_ARP_HPP
#define MIXIPGW_TOOLS_ARP_HPP

#include "lib/interface/filter_interface.hpp"

namespace MIXIPGW_TOOLS{
  // arp filter
  class Arp: public FilterIf{
  public:
      virtual int OnFilter(PktInterface*,int*);
  public:
      static void ArpLogging(const char* , const char* , const struct ether_arp* );
      static void EthLogging(const char* , const char* , const struct ether_header* );
  }; // class Arp
}; // namespace MIXIPGW_TOOLS

#endif //MIXIPGW_TOOLS_ARP_HPP
