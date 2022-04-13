//
// Created by mixi on 2017/04/24.
//

#include "mixipgw_tools_def.h"
#include "lib/filter/arp.hpp"
#include "lib/pktheader.hpp"
#include "lib/logger.hpp"
#include "lib/module.hpp"
#include "lib/process.hpp"

typedef MIXIPGW_TOOLS::ProcessParameter PP;
//
namespace MIXIPGW_TOOLS{
  int Arp::OnFilter(PktInterface* pkt ,int* flag){
      struct ether_header *eh  = EH(pkt);
      struct ether_arp    *arp = ARP(pkt);
      struct ether_arp    tarp;
      //
      if (Module::VERBOSE() >= PCAPLEVEL_ARP){
          Logger::APPEND_PCAP(0, (const char*)EH(pkt), pkt->Length());
      }
      if (Module::VERBOSE() >= PCAPLEVEL_DBG){
          ArpLogging("pre_arp_send","--", arp);
          EthLogging("pre_arp_send","--", eh);
      }
      SwapMac(pkt);
      //
      arp->ea_hdr.ar_hrd = ntohs(ARPHRD_ETHER);
      arp->ea_hdr.ar_pro = ntohs(ETHERTYPE_IP);
      arp->ea_hdr.ar_hln = ETHER_ADDR_LEN;
      arp->ea_hdr.ar_pln = sizeof(in_addr_t);
      arp->ea_hdr.ar_op  = ntohs(ARPOP_REPLY);
      // find own mac.
      memcpy(&tarp, arp, sizeof(tarp));
      memcpy(arp->arp_tpa, tarp.arp_spa, sizeof(arp->arp_tpa));
      memcpy(arp->arp_tha, tarp.arp_sha, sizeof(arp->arp_tha));
      memcpy(arp->arp_spa, tarp.arp_tpa, sizeof(arp->arp_spa));

      if ((process_param_->Get(PP::TXT_MAC_DST))[0] == 0){
          Logger::LOGERR("invalid missing mac:(%02X%02X%02X%02X) arp_send",
                         arp->arp_spa[0],arp->arp_spa[1],arp->arp_spa[2],arp->arp_spa[3]);
          return(RETERR);
      }
      memcpy(arp->arp_sha, process_param_->Get(PP::TXT_MAC_DST), ETHER_ADDR_LEN);
      //
      // eh->ether_type = ntohs(ETHERTYPE_ARP);
      memcpy(eh->ether_shost, arp->arp_sha, ETHER_ADDR_LEN);
      //
      if (Module::VERBOSE() >= PCAPLEVEL_ARP){
          EthLogging("arp_send", " - ", eh);
          ArpLogging("arp_send", " - ", arp);
          Logger::LOGINF("send arp packet len(%d)", pkt->Length());
      }
      //
      (*flag) |= PROC_NEED_SEND;
      //
      if (Module::VERBOSE() >= PCAPLEVEL_DBG){
          Logger::APPEND_PCAP(0, (const char*)EH(pkt), pkt->Length());
      }
      return(RETOK);
  }
  // private functions
  void Arp::ArpLogging(const char* prefix, const char* ifname, const struct ether_arp* arp){
      Logger::LOGINF("arp <%s:%s>\n\tsha : %02X%02X%02X%02X%02X%02X\n\tspa : %02X%02X%02X%02X\n\ttha : %02X%02X%02X%02X%02X%02X\n\ttpa : %02X%02X%02X%02X\n",
             prefix,
             ifname,
             arp->arp_sha[0], arp->arp_sha[1], arp->arp_sha[2], arp->arp_sha[3], arp->arp_sha[4], arp->arp_sha[5],
             arp->arp_spa[0], arp->arp_spa[1], arp->arp_spa[2], arp->arp_spa[3],

             arp->arp_tha[0], arp->arp_tha[1], arp->arp_tha[2], arp->arp_tha[3], arp->arp_tha[4], arp->arp_tha[5],
             arp->arp_tpa[0], arp->arp_tpa[1], arp->arp_tpa[2], arp->arp_tpa[3]);
  }
  void Arp::EthLogging(const char* prefix, const char* ifname, const struct ether_header* eh){
      Logger::LOGINF("eth <%s:%s>\n\tshost:%02X%02X%02X%02X%02X%02X\n\tdhost:%02X%02X%02X%02X%02X%02X\n",
             prefix,
             ifname,
             eh->ether_shost[0], eh->ether_shost[1], eh->ether_shost[2], eh->ether_shost[3], eh->ether_shost[4], eh->ether_shost[5],
             eh->ether_dhost[0], eh->ether_dhost[1], eh->ether_dhost[2], eh->ether_dhost[3], eh->ether_dhost[4], eh->ether_dhost[5]);
  }
}; // namespace MIXIPGW_TOOLS
