//
// Created by mixi on 2017/07/05.
//

#ifndef MIXIPGW_TOOLS_POLICY_HPP
#define MIXIPGW_TOOLS_POLICY_HPP

#include "lib/interface/filter_interface.hpp"

namespace MIXIPGW_TOOLS{
  // policy filter
  class Policy: public FilterIf{
  public:
      virtual int OnFilter(PktInterface*,int*);
  }; // class Policy
}; // namespace MIXIPGW_TOOLS
#endif //MIXIPGW_TOOLS_POLICY_HPP
