//
// Created by mixi on 2017/04/21.
//

#ifndef MIXIPGW_TOOLS_FILTERCONTAINER_HPP
#define MIXIPGW_TOOLS_FILTERCONTAINER_HPP

namespace MIXIPGW_TOOLS{
  class PktInterface;
  class ProcessParameter;
  // Filters can be replaced to override implementation process
  // for each packet type.
  //
  // After buffer is rewritten,
  // It can be swapped to same NIC by turning on flags to swap.
  // Supported only [swap].
  //
  // When overriding swap process, it's posible to override Process::MoveRing/ProcessRing.
  // -------
  // Thereforre, use factory pattern intarface for classes for which override implementations are valid.
  //   + Implement BridgeProcess, which has same interface as Process.
  //   + Execute bridge processign when go though FilterIf.
  // -------
  // 

  // OnInitFilter(void)[1], for initialization.
  // OnFilter(..) [2], filter every packet
  // OnUnInitFilter(void)[3], for cleanup
  // <life cycle>
  // [1]. when registering filter ( almost module start ) 
  // [2]. when notify packet, 1 packet
  // [3]. when detach filter( almost module end )  , from destructor
  class FilterIf{
      friend class FilterContainer;
  public:
      virtual int OnInitFilter(void);
      virtual int OnFilter(PktInterface*,int*) = 0;
      virtual int OnUnInitFilter(void);
  protected:
      virtual void SwapMac(PktInterface*);
      virtual void SwapIp(PktInterface*);
      virtual void ModifyIpEpc(PktInterface*);
  public:
      ProcessParameter* process_param_;
  };// class FilterIf

  // filter container
  // dispatch 1 packet to filter interface
  class FilterContainer{
  public:
      // important: Consider CPU cache balance carefully.
      enum{
          DC, ARP, ICMP, GTPU,
          OVERIP, CTRL, GTPUECHOREQ, BFD,
          GTPC, BFDC, POLICY,
          MAX
          };
  public:
      FilterContainer();
      ~FilterContainer();
  public:
      void SetFilter(int,FilterIf*,ProcessParameter*);
      int  Exec(PktInterface*,int*);
  private:
      // it is more generalizable to use std::vector<DitributeInterface*> or
      // std::map<type, function pointer> for implementation, for CPU cache efficiency,
      //  define it as continuous pointer array.
      FilterIf* filters_[FilterContainer::MAX];
  }; // class FilterContainer
}; // namespace MIXIPGW_TOOLS


#endif //MIXIPGW_TOOLS_FILTERCONTAINER_HPP
