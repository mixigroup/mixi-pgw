//
// Created by mixi on 2017/04/21.
//

#include "mixipgw_tools_def.h"
#include "lib/interface/filter_interface.hpp"
#include "lib/pktheader.hpp"

namespace MIXIPGW_TOOLS{
  FilterContainer::FilterContainer(){
      bzero(filters_,sizeof(filters_));
  }
  FilterContainer::~FilterContainer(){
      for(int id = (FilterContainer::DC+1);id < FilterContainer::MAX;id++){
          if (filters_[id] != NULL){
              filters_[id]->OnUnInitFilter();
          }
          filters_[id] = NULL;
      }
  }
  void FilterContainer::SetFilter(int id,FilterIf* intf,ProcessParameter* process_param){
      if (!intf) { return; }
      if (id > FilterContainer::DC && id < FilterContainer::MAX){
          // Can overridden when initialization processing is required
          intf->OnInitFilter();
          intf->process_param_ = process_param;
          filters_[id] = intf;
      }
  }
  int FilterContainer::Exec(PktInterface* pkt, int* flag){
      int type = pkt->Type();
      if (type == RETPTGTPU && filters_[GTPU]){
          return(filters_[GTPU]->OnFilter(pkt, flag));
      }else if (type == RETPTOVERIP && filters_[OVERIP]){
          return(filters_[OVERIP]->OnFilter(pkt, flag));
      }else if (type == RETPTICMP && filters_[ICMP]){
          return(filters_[ICMP]->OnFilter(pkt, flag));
      }else if (type == RETPTCTRL && filters_[CTRL]){
          return(filters_[CTRL]->OnFilter(pkt, flag));
      }else if (type == RETPTBFDC && filters_[BFDC]){
          return(filters_[BFDC]->OnFilter(pkt, flag));
      }else if (type == RETPTARP && filters_[ARP]){
          return(filters_[ARP]->OnFilter(pkt, flag));
      }else if (type ==RETPTGTPUECHOREQ && filters_[GTPUECHOREQ]){
          return(filters_[GTPUECHOREQ]->OnFilter(pkt, flag));
      }
      return(RETOK);
  }
  // interface implemented.
  int FilterIf::OnInitFilter(void){ return(0); }
  int FilterIf::OnUnInitFilter(void){ return(0); }
  //
  void FilterIf::SwapMac(PktInterface* pkt){
      // swap dst <-> src
      uint8_t tmp_mac[ETHER_ADDR_LEN];
      memcpy(tmp_mac, EH(pkt)->ether_dhost, ETHER_ADDR_LEN);
      memcpy(EH(pkt)->ether_dhost, EH(pkt)->ether_shost, ETHER_ADDR_LEN);
      memcpy(EH(pkt)->ether_shost, tmp_mac, ETHER_ADDR_LEN);
  }
  void FilterIf::SwapIp(PktInterface* pkt){

  }
  void FilterIf::ModifyIpEpc(PktInterface* pkt){
      // case inner ecn in(00,11)                    -> transfer without change.
      // case inner ecn in(10,01) and outer ecn(!11) -> transfer without change.
      // case inner ecn in(10,01) and outer ecn(11)  -> transfer with change inner ecn(11).
      if (IPTOS_ECN(IP_I(pkt)->ip_tos) == IPTOS_ECN_NOTECT/* 00 */){
          ;; // without change.
      }else if (IPTOS_ECN(IP_I(pkt)->ip_tos) == IPTOS_ECN_CE/* 11 */){
          ;; // without change.
      }else if (IPTOS_ECN(IP(pkt)->ip_tos) == IPTOS_ECN_CE){
          // update ecn bit in external to internal.
          IP_I(pkt)->ip_tos |= IPTOS_ECN_CE;
      }else{
          ;; // without change.
      }
  }
}; // namespace MIXIPGW_TOOLS

