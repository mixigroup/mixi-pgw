//
// Created by mixis on 24/nov/16.
//
#include "mixipgw_tools_def.h"
#include "lib/process.hpp"
#include "lib/logger.hpp"
#include "lib/misc.hpp"
#include "lib/module.hpp"
//
#include "lib/const/link.h"
#include <map>
#include <vector>
#include <string>

using namespace MIXIPGW_TOOLS;

//
namespace MIXIPGW_TOOLS{
  typedef ProcessParameter PP;
  typedef std::vector<std::string>  VECSTR;
  typedef std::vector<std::string>::iterator VECSTRITR;
  //
  int Misc::module_id_ = 0;
  std::string module_cfg_ = "";
  static int StrSplit(const char* src, const char* sep,std::vector<std::string>& splitted);
  //
  static inline const char *Norm2(char *buf, double val, const char *fmt){
      const char *units[] = { "", "K", "M", "G", "T" };
      u_int i;
      for (i = 0; val >=1000 && i < sizeof(units)/sizeof(char *) - 1; i++)
          val /= 1000;
      sprintf(buf, fmt, val, units[i]);
      return(buf);
  }
  const char *Misc::Norm(char *buf, double val){
      return Norm2(buf, val, "%.3f %s");
  }
  //
  int Misc::SetAffinity(int cpuid){
#ifndef __APPLE__
      cpu_set_t cpumask;
      CPU_ZERO(&cpumask);
      CPU_SET(cpuid, &cpumask);
      //
      if (cpuid >= 0){
          if (pthread_setaffinity_np(pthread_self(), sizeof(cpumask), &cpumask) != 0) {
              Logger::LOGERR("Unable to set affinity: %s", strerror(errno));
              return(1);
          }
          return(RETOK);
      }
      return(RETERR);
#else
      return(RETOK);
#endif
  }

  int Misc::GetCpucores(void){
      int ncpu;
      ncpu = sysconf(_SC_NPROCESSORS_ONLN);
      return(ncpu);
  }
  unsigned long long Misc::GetMillisecond(void){
    return(GetMicrosecond()/1000);
  }
  unsigned long long Misc::GetMicrosecond(void){
      struct timeval	tv;
      gettimeofday(&tv,NULL);
      return((((uint64_t)tv.tv_sec * 1000000) + ((uint64_t)tv.tv_usec)));
  }
  unsigned long long Misc::GetMicrosecondArround(void){
      struct timeval	tv;
      gettimeofday(&tv,NULL);
      return((((uint64_t)tv.tv_sec <<19) + ((uint64_t)tv.tv_usec)));
  }
  // signal
  void Misc::SigintH(int sig) {
      (void)sig;	/* UNUSED */
      Module::ABORT_INCR();
      Logger::LOGINF("received ctrl-C.");
//    signal(SIGINT, SIG_DFL);
  }
  void Misc::UsageWithExit(int mod) {
      if (mod == Misc::MOD_COUNTER){
          fprintf(stderr, "usage: <program> [-v] -i <interface .ex)eth2-0> -c <cpuid> -p <ctrlport> -f <ingress/egress> -m <moduleid> -l <interface basename .ex)eth2> -p <reserved> -p <> -n ifa-idx(254==not use) -n ifb-idx -n if[a/b]-txidx");
      }else if (mod == Misc::MOD_TRANSLATER){
          fprintf(stderr, "usage: <program> [-v] -i ifa -i ifb -p sport -p dport -p cport -n ifa-idx(254==not use) -n ifb-idx -n if[a/b]-txidx");
      }
      exit(1);
  }
  int Misc::GetInterface(const char* ifname, char* hwaddr, unsigned hwaddrlen, char* in_addr, unsigned in_addrlen){
      int		layer2_socket;
      //
      struct ifreq		ifr;
      memset(&ifr,0,sizeof(ifr));

      if (hwaddrlen != ETHER_ADDR_LEN || in_addrlen != sizeof(uint32_t)){
          Logger::LOGERR("buffer length\n");
          return(RETERR);
      }
#ifdef __APPLE__
      if ((layer2_socket = socket(PF_ROUTE,SOCK_RAW,htons(SOCK_RAW))) < 0){
#else
      if ((layer2_socket = socket(PF_PACKET,SOCK_RAW,htons(ETH_P_ALL))) < 0){
#endif
          Logger::LOGERR("wrong socket\n");
          return(RETERR);
      }
      strcpy(ifr.ifr_name,ifname);
#ifdef __APPLE__
      struct ifaddrs *ifa_list, *ifa;
      struct sockaddr_dl *dl = NULL;

      if (getifaddrs(&ifa_list) < 0) {
          Logger::LOGERR("not found..interfaces.");
          return(RETERR);
      }
      for (ifa = ifa_list; ifa != NULL; ifa = ifa->ifa_next) {
          dl = (struct sockaddr_dl*)ifa->ifa_addr;
          if (dl->sdl_family == AF_LINK && dl->sdl_type == IFT_ETHER &&
              memcmp(dl->sdl_data, ifname, MIN(sizeof(dl->sdl_data), strlen(ifname)))== 0) {
              // set result(mac)
              memcpy(hwaddr, LLADDR(dl), ETHER_ADDR_LEN);
              // set result(ipaddr)
              memcpy(in_addr, &((struct sockaddr_in *)&dl->sdl_data)->sin_addr,4);
              goto INTERFACE_FOUND;
          }
      }
      Logger::LOGERR("not found...(%s)", ifname);
      return(RETOK);
INTERFACE_FOUND:
      freeifaddrs(ifa_list);
#else
      if (ioctl(layer2_socket, SIOCGIFHWADDR, &ifr) < 0){
          Logger::LOGERR("failed.ioctl.(SIOCGIFHWADDR) %s", ifname);
          close(layer2_socket);
          return(RETERR);
      }
      // set result(mac)
      memcpy(hwaddr, ifr.ifr_hwaddr.sa_data,ETHER_ADDR_LEN);
      if (ioctl(layer2_socket, SIOCGIFADDR, &ifr) < 0){
          Logger::LOGERR("failed.ioctl.(SIOCGIFADDR) %s", ifname);
      }else{
          // set result(ipaddr)
          memcpy(in_addr, &((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr,4);
      }
#endif
      close(layer2_socket);

      return(RETOK);
  }
  void Misc::InitCommandParse(int argc, char** argv,ProcessParameter* ph){
      int ch,ifcount=0;
      struct ether_arp  arp;
      const char* zeros[ETHER_ADDR_LEN] = {0};
      if (!argc || !argv){ goto skip_command_parse; }
      //
      while ( (ch = getopt(argc, argv, "n:p:i:c:vf:s:dg:u:t:m:I:k:")) != -1) {
          switch (ch) {
              default:
                  Logger::LOGERR("bad option %c %s", ch, optarg);
                  Misc::UsageWithExit(0);
                  break;
              case 'm': /* module id */
                  break;
              case 'i':	/* interface */
                  if (!(ph->Get(PP::TXT_IF_SRC))[0] && optarg != NULL){
                      ph->Set(PP::TXT_IF_SRC, optarg);
                      ifcount++;
                  } else if (!(ph->Get(PP::TXT_IF_DST))[0] && optarg != NULL) {
                      ph->Set(PP::TXT_IF_DST, optarg);
                      ifcount++;
                  } else {
                      Logger::LOGERR("%s ignored, already has 2 interfaces", optarg);
                  }
                  break;
              case 'I':
                  if (!(ph->Get(PP::TXT_IF_BASE))[0] && optarg != NULL){
                      ph->Set(PP::TXT_IF_BASE, optarg);
                  }
                  break;
              case 'p':
                  if (!ph->Get(PP::USH_UDP_SRC_PORT) && optarg != NULL){
                      ph->Set(PP::USH_UDP_SRC_PORT, (unsigned short)atoi(optarg));
                  } else if (!ph->Get(PP::USH_UDP_DST_PORT) && optarg != NULL) {
                      ph->Set(PP::USH_UDP_DST_PORT, (unsigned short)atoi(optarg));
                  } else if (!ph->Get(PP::USH_UDP_CTRL_PORT) && optarg != NULL) {
                      ph->Set(PP::USH_UDP_CTRL_PORT, (unsigned short)atoi(optarg));
                  } else {
                      Logger::LOGERR("%s ignored, already has 3 portsettings", optarg);
                  }
                  break;
              case 'n':
                  if ((uint8_t)ph->Get(PP::USH_IF_SRC_EXT_IDX)==0xff && optarg != NULL){
                      ph->Set(PP::USH_IF_SRC_EXT_IDX, (unsigned short)atoi(optarg));
                  } else if ((uint8_t)ph->Get(PP::USH_IF_DST_EXT_IDX)==0xff && optarg != NULL) {
                      ph->Set(PP::USH_IF_DST_EXT_IDX, (unsigned short)atoi(optarg));
                  } else if ((uint8_t)ph->Get(PP::USH_IF_TX_EXT_IDX)==0xff && optarg != NULL){
                      ph->Set(PP::USH_IF_TX_EXT_IDX, (unsigned short)atoi(optarg));
                  } else {
                      Logger::LOGERR("%s ignored, already has 2 interfaces extend index", optarg);
                  }
                  break;
              case 'c':
                  if (optarg != NULL){
                      ph->Set(PP::USG_ATTACH_CPUID, (uint32_t)atoi(optarg));
                      Logger::LOGINF("attach cpuid(%u)", ph->Get(PP::USG_ATTACH_CPUID));
                  }
                  break;
              case 'g':   /* notify target port */
                  if (optarg != NULL){
                      ph->Set(PP::USH_UDP_NTY_PORT, (unsigned short)atoi(optarg));
                      Logger::LOGINF("nofify udp port(%u)", ph->Get(PP::USH_UDP_NTY_PORT));
                  }
                  break;
              case 'u':	/* notify target */
                  if (!(ph->Get(PP::TXT_TRGT_NTY))[0] && optarg != NULL){
                      ph->Set(PP::TXT_TRGT_NTY, optarg);
                  } else {
                      Logger::LOGERR("%s ignored, already has a notify target address", optarg);
                  }
                  break;

              case 'v':
              case 'd':
                  Module::VERBOSE_INCR();
                  break;
              case 'f':
                  if (optarg != NULL){
                      if (strcasecmp(optarg, "decap")==0){
                          ph->Set(PP::USG_MODULE_FLAG, MF_ORDER_DECAP);
                      }else if (strcasecmp(optarg, "encap")==0){
                          ph->Set(PP::USG_MODULE_FLAG, MF_ORDER_ENCAP);
                      }else if (strcasecmp(optarg, "ingress")==0){
                          ph->Set(PP::USG_MODULE_FLAG, MF_ORDER_INGRESS);
                      }else if (strcasecmp(optarg, "egress")==0){
                          ph->Set(PP::USG_MODULE_FLAG, MF_ORDER_EGRESS);
                      }
                  }
                  break;
              case 't':
                  if (optarg != NULL){
                      ph->Set(PP::USG_THREAD_CNT, atoi(optarg));
                      Logger::LOGINF("thread_cnt (%d)", ph->Get(PP::USG_THREAD_CNT));
                  }
                  break;
              case 'k':	/*  */
                  if (!(ph->Get(PP::TXT_CLASS))[0] && optarg != NULL){
                      ph->Set(PP::TXT_CLASS, optarg);
                  } else {
                      Logger::LOGERR("%s ignored, already has a notify target klass", optarg);
                  }
                  break;

          }
      }
      if (!(ph->Get(PP::TXT_IF_SRC))[0] || !(ph->Get(PP::TXT_IF_BASE))[0] || (!(ph->Get(PP::TXT_IF_DST))[0] && ifcount==2)) {
          Logger::LOGERR("missing interface / same interface, endpoint 0 goes to host");
          UsageWithExit(0);
      }
      if (ifcount == 1){
          if (!ph->Get(PP::USH_UDP_SRC_PORT)){
              Logger::LOGERR("missing port settings.(single . interface)");
              UsageWithExit(0);
          }
          ph->Copy(PP::USH_UDP_CTRL_PORT, PP::USH_UDP_SRC_PORT);
      }else{
          if (!ph->Get(PP::USH_UDP_SRC_PORT) || !ph->Get(PP::USH_UDP_DST_PORT) || !ph->Get(PP::USH_UDP_CTRL_PORT)){
              Logger::LOGERR("missing port settings.");
              UsageWithExit(0);
          }
      }

      if (ifcount == 2){
          if ((uint8_t)ph->Get(PP::USH_IF_DST_EXT_IDX)==0xff || (uint8_t)ph->Get(PP::USH_IF_SRC_EXT_IDX)==0xff || (uint8_t)ph->Get(PP::USH_IF_TX_EXT_IDX)==0xff){
              Logger::LOGERR("missing interface extend settings.");
              UsageWithExit(0);
          }
      }else if (ifcount == 1){
          Logger::LOGINF("not need . extends.");
      }else{
          if ((uint8_t)ph->Get(PP::USH_IF_DST_EXT_IDX)==0xff){
              Logger::LOGERR("missing interface extend settings.(%d)", ifcount);
              UsageWithExit(0);
          }
      }
skip_command_parse:
      Logger::LOGINF("find mac(%s)", ph->Get(PP::TXT_IF_BASE));

      // setup interface arp dual_buffered_lookup_table
      memset(&arp, 0, sizeof(arp));
      if (IS_RETOK(GetInterface(ph->Get(PP::TXT_IF_BASE), (char*)arp.arp_sha, sizeof(arp.arp_sha), (char*)arp.arp_spa, sizeof(arp.arp_spa)))){
          arp.ea_hdr.ar_hrd = ntohs(ARPHRD_ETHER);
          arp.ea_hdr.ar_pro = ntohs(ETHERTYPE_IP);
          arp.ea_hdr.ar_hln = ETHER_ADDR_LEN;
          arp.ea_hdr.ar_pln = sizeof(in_addr_t);
          arp.ea_hdr.ar_op  = ntohs(ARPOP_REPLY);
          //
          ph->Set(PP::TXT_MAC_DST, (const char*)arp.arp_sha);
          Logger::LOGINF("exist mac[%02X:%02X:%02X:%02X:%02X:%02X]",
                         arp.arp_sha[0],arp.arp_sha[1],arp.arp_sha[2],
                         arp.arp_sha[3],arp.arp_sha[4],arp.arp_sha[5]);
          if (*((uint32_t*)&arp.arp_spa[0]) != 0){
              ph->Set(PP::USG_IF_SRC_IPV4, *((unsigned*)&arp.arp_spa[0]));
          }
      }else{
          Logger::LOGERR("missing mac(%s)", ph->Get(PP::TXT_IF_BASE));
      }

      if (!(ph->Get(PP::TXT_IF_DST))[0]){ goto resume_00; }
      memset(&arp, 0, sizeof(arp));
      if (IS_RETOK(GetInterface(ph->Get(PP::TXT_IF_DST), (char*)arp.arp_sha, sizeof(arp.arp_sha), (char*)arp.arp_spa, sizeof(arp.arp_spa)))){
          arp.ea_hdr.ar_hrd = ntohs(ARPHRD_ETHER);
          arp.ea_hdr.ar_pro = ntohs(ETHERTYPE_IP);
          arp.ea_hdr.ar_hln = ETHER_ADDR_LEN;
          arp.ea_hdr.ar_pln = sizeof(in_addr_t);
          arp.ea_hdr.ar_op  = ntohs(ARPOP_REPLY);

      }
      ph->Set(PP::TXT_MAC_SRC, (const char*)arp.arp_sha);
      if (*((uint32_t*)&arp.arp_spa[0]) != 0){
          ph->Set(PP::USG_IF_DST_IPV4, (*(unsigned*)&arp.arp_spa[0]));
      }
resume_00:
      // attach cpuid
      if (ph->Get(PP::USG_ATTACH_CPUID) != 0xffffffff){
          SetAffinity((int)ph->Get(PP::USG_ATTACH_CPUID));
      }
  }

  int Misc::GetIpvMask(const char* src, unsigned char* dst, unsigned* len, unsigned char* mask){
      VECSTR    splitted;
      // sepalate mask.
      if (!src || !dst || !len){
          return(RETERR);
      }
      if (strstr(src,"/") == NULL){
          return(GetIpv(src, dst, len));
      }
      // 192.168.111.123/16
      if (StrSplit(src, "/",splitted) != RETOK){
          return(RETERR);
      }
      if (splitted.size() != 2){ return(RETERR); }
      //
      if (GetIpv4(splitted[0].c_str(), dst, len) != RETOK){ return(RETERR); }
      if (mask){
          (*mask) = (uint8_t)strtoul(splitted[1].c_str(),NULL,10);
      }
      return(RETOK);
  }

  int Misc::GetIpv(const char* src, unsigned char* dst, unsigned* len){
      if (!src || !dst || !len){
          return(RETERR);
      }
      if (strstr(src,":") != NULL){
          // ipv6
          return(GetIpv6(src, dst, len));
      }else if (strstr(src,".") != NULL){
          // ipv4
          return(GetIpv4(src, dst, len));
      }
      return(RETERR);
  }
  int Misc::GetIpv4(const char* src, unsigned char* dst, unsigned* len){
      VECSTR    splitted;
      VECSTRITR itr;
      int       n;
      //
      if ((*len) < sizeof(uint32_t)){
          return(RETERR);
      }
      // 192.168.111.123
      if (StrSplit(src, ".",splitted) != RETOK){
          return(RETERR);
      }
      if (splitted.size() != 4){ return(RETERR); }
      //
      for(n = 0, itr = splitted.begin();itr != splitted.end();++itr,n++){
          dst[n] = (uint8_t)strtoul((*itr).c_str(),NULL,10);
      }
      (*len) = sizeof(uint32_t);
      return(RETIPV4);
  }
  int Misc::GetIpv6(const char* src, unsigned char* dst, unsigned* len){
      VECSTR    splitted;
      VECSTRITR itr;
      int       n;
      //
      if ((*len) < 16/*128bit*/){
          return(RETERR);
      }
      // 2001:0db8:8823:0000:0000:8a82:0123:7445
      if (StrSplit(src, ":",splitted) != RETOK){
          return(RETERR);
      }
      if (splitted.size() != 8){
          return(RETERR);
      }
      //
      for(n = 0, itr = splitted.begin();itr != splitted.end();++itr,n+=2){
          uint16_t doctet = (uint16_t)strtoul((*itr).c_str(),NULL,16);
          memcpy(&dst[n], &doctet, sizeof(doctet));
      }
      (*len) = 16;/*128bit*/
      return(RETIPV6);
  }
  int Misc::GetOpt(int argc, char** argv, const char* key, char* ret, int len){
      int ch,r = RETERR;
      optind = optopt = 1;
      opterr = 0;
      //
      while ( (ch = getopt(argc, argv, key)) != -1) {
          if (ch == key[0]){
              if (optarg && ret && len > 0){
                  memcpy(ret, optarg, MIN(len, strlen(optarg)));
                  r = RETOK;
              }
              break;
          }
      }
      return(r);
  }
  int Misc::GetModuleid(void){
      return(Misc::module_id_);
  }
  void Misc::SetModuleid(int mid){
      Misc::module_id_ = mid;
  }
  void Misc::SetModuleCfg(const char* cfg){
      module_cfg_ = cfg?cfg:"";
  }
  void Misc::GetModuleCfg(char* cfg,int len){
      if (cfg != NULL){
          strncpy(cfg, module_cfg_.c_str(), MIN(len, strlen(module_cfg_.c_str())));
      }
  }

  int StrSplit(const char* src, const char* sep,std::vector<std::string>& splitted){
      char*		finded = NULL;
      char*		tmpsep = NULL;
      char*		tmpsrc = NULL;
      char*		current = NULL;
      uint32_t	seplen;
      uint32_t	srclen;
      uint32_t	busyloop_counter = 0;
      std::string tmpstr("");
      //
      if (!src || !sep)		{ return(RETERR); }
      if (!strlen(src))		{ return(RETERR); }
      if (!strlen(sep))		{ return(RETERR); }
      //
      seplen	= strlen(sep);
      srclen	= strlen(src);
      //
      tmpsep	= (char*)malloc(seplen + 1);
      memset(tmpsep,0,(seplen + 1));
      memcpy(tmpsep,sep,seplen);
      //
      tmpsrc	= (char*)malloc(srclen + 1);
      memset(tmpsrc,0,(srclen + 1));
      memcpy(tmpsrc,src,srclen);
      //
      current	= tmpsrc;
      //
      while(true){
          // find separator
          if ((finded = strstr(current,tmpsep)) == NULL){
              splitted.push_back(current);
              break;
          }
          // first char is separator.
          if (finded == current){
              finded += seplen;
          }else{
              tmpstr.assign(current,(finded - current));
              splitted.push_back(tmpstr);
          }
          current += (finded - current);
          // end of string
          if (current >= (tmpsrc + srclen)){
              break;
          }
          busyloop_counter ++;
          if (busyloop_counter > 1000){
              return(RETERR);
          }
      }
      free(tmpsrc);
      free(tmpsep);
      //
      return(RETOK);
  }

  unsigned Misc::Checksum(const void *data, unsigned short len, unsigned sum){
      unsigned  _sum   = sum,_checksum = 0;
      unsigned  _count = len;
      unsigned short* _addr  = (unsigned short*)data;
      //
      while( _count > 1 ) {
          _sum += ntohs(*_addr);
          _addr++;
          _count -= 2;
      }
      if(_count > 0 ){
          _sum += ntohs(*_addr);
      }
      while (_sum>>16){
          _sum = (_sum & 0xffff) + (_sum >> 16);
      }
      return(~_sum);
  }
  unsigned short Misc::Wrapsum(unsigned sum){
      sum = sum & 0xFFFF;
      return (htons(sum));
  }
  unsigned short Misc::TeidGid(const unsigned teid){
      link_t cnv;
      cnv.w.pgw_teid_uc = teid;
      return(cnv.w.pgw_teid.gid);
  }
  unsigned short Misc::TeidGcnt(const unsigned teid){
      link_t cnv;
      cnv.w.pgw_teid_uc = teid;
      return(cnv.w.pgw_teid.gcnt);
  }

  int Misc::IsExists(const char* filepath){
      struct stat st;
      if (stat(filepath, &st) == 0){
          return(RETOK);
      }
      return(RETERR);
  }

}; // namespace MIXIPGW_TOOLS
