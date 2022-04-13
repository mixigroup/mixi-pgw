//
// Created by mixi on 2017/04/25.
//

#ifndef MIXIPGW_TOOLS_GTPU_COUNTER_EGRESS_HPP
#define MIXIPGW_TOOLS_GTPU_COUNTER_EGRESS_HPP

#include "lib/interface/filter_interface.hpp"

namespace MIXIPGW_TOOLS{
  // gtp-u tunnel process/counter
  class CounterEgress: public FilterIf{
  public:
      virtual int OnInitFilter(void);
      virtual int OnFilter(PktInterface*,int*);
  private:
      uint64_t  calc_buffer_[MAX_STRAGE_PER_GROUP];
      uint64_t  count_buffer_[MAX_STRAGE_PER_GROUP];
  }; // class CounterEgress
}; // namespace MIXIPGW_TOOLS

#endif //MIXIPGW_TOOLS_GTPU_COUNTER_EGRESS_HPP
