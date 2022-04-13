//
// Created by mixi on 2017/04/21.
//

#ifndef MIXIPGW_TOOLS_ICMP_HPP
#define MIXIPGW_TOOLS_ICMP_HPP

#include "lib/interface/filter_interface.hpp"

namespace MIXIPGW_TOOLS{
  // ICMP filters do not require initial processing or termination processing,
  // so event override is not necesarry.
  class Icmp: public FilterIf{
  public:
      virtual int OnFilter(PktInterface*,int*);
  }; // class Icmp
}; // namespace MIXIPGW_TOOLS

#endif //MIXIPGW_TOOLS_ICMP_HPP
