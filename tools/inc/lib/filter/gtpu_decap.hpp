//
// Created by mixi on 2017/04/26.
//

#ifndef MIXIPGW_TOOLS_GTPU_DECAP_HPP
#define MIXIPGW_TOOLS_GTPU_DECAP_HPP
#include "lib/interface/filter_interface.hpp"

namespace MIXIPGW_TOOLS{
  // gtp-u tunnel process/decap
  class TranslatorDecapEnforcer: public FilterIf{
  public:
      virtual int OnInitFilter(void);
      virtual int OnFilter(PktInterface*,int*);
  private:
      uint32_t recalc_counter_;
  }; // class TranslatorDecapEnforcer

  // gtp-u tunnel process/decap - only mark
  class TranslatorDecapMarker: public FilterIf{
  public:
      virtual int OnInitFilter(void);
      virtual int OnFilter(PktInterface*,int*);
  private:
      uint32_t recalc_counter_;
  }; // class TranslatorDecapMarker
}; // namespace MIXIPGW_TOOLS


#endif //MIXIPGW_TOOLS_GTPU_DECAP_HPP
