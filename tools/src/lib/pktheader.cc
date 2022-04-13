//
// Created by mixi on 2017/04/21.
//

#include "mixipgw_tools_def.h"
#include "lib/pktheader.hpp"
#include "lib/const/gtpu.h"

namespace MIXIPGW_TOOLS{
  PktHeader::PktHeader(char* data,int len):data_(data),len_(len),len_vlan_(0),type_(RETERR),len_ipv_(0){
      struct ether_header *eh = (struct ether_header*)data;
      struct ip           *ip = (struct ip*)(data + sizeof(struct ether_header) + LEN_VLAN(eh));
      gtpu_header_ptr     gtp =  (gtpu_header_ptr)(data + sizeof(struct ether_header) + LEN_VLAN(eh) + (ip->ip_hl * 4) + (sizeof(struct udphdr)));
      len_vlan_ = LEN_VLAN(eh);
      // ipv4/6
      if (ip->ip_v==IPVERSION){
          len_ipv_= (ip->ip_hl*4);
      }else{
          len_ipv_= sizeof(struct ip6_hdr);
          gtp = (gtpu_header_ptr)(data + sizeof(struct ether_header) + LEN_VLAN(eh) + len_ipv_ + (sizeof(struct udphdr)));
      }
      //
      offset_[PktHeader::ETHER] = 0;
      offset_[PktHeader::IP]    = sizeof(struct ether_header) + LEN_VLAN(eh);
      offset_[PktHeader::UDP]   = sizeof(struct ether_header) + LEN_VLAN(eh) + (len_ipv_);
      offset_[PktHeader::GTPU]  = sizeof(struct ether_header) + LEN_VLAN(eh) + (len_ipv_) + sizeof(struct udphdr);
      offset_[PktHeader::ARP]   = sizeof(struct ether_header) + LEN_VLAN(eh);
      offset_[PktHeader::BFD]   = offset_[PktHeader::GTPU];
      offset_[PktHeader::IP_I]  = offset_[PktHeader::GTPU] + sizeof(gtpu_header_t) + (gtp->u.flags&GTPU_FLAGMASK?4:0);
      offset_[PktHeader::ICMP]  = offset_[PktHeader::UDP];
      offset_[PktHeader::PAYLOAD] = offset_[PktHeader::GTPU];
      // implemented. ipv4 header only
      offset_[PktHeader::ICMP_I]= offset_[PktHeader::IP_I] + len_ipv_;
  }
  PktHeader::~PktHeader(){}
  //
  void* PktHeader::Header(int id){
      if (id > PktHeader::DC && id < PktHeader::MAX){
          return(data_ + offset_[id]);
      }
      return(NULL);
  }
  int PktHeader::Length(void){ return(len_); }
  void PktHeader::Length(int l) { len_ = l; }
  void PktHeader::Type(int t){ type_ = t; }
  int PktHeader::Type(void){ return(type_); }
  int PktHeader::LengthVlan(void){ return(len_vlan_); }
  int PktHeader::LengthIp(void){ return(len_ipv_); }
}; // namespace MIXIPGW_TOOLS

