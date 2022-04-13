//
// Created by mixi on 2017/01/10.
//

#ifndef MIXIPGW_TOOLS_MISC_H
#define MIXIPGW_TOOLS_MISC_H

namespace MIXIPGW_TOOLS{
  class Misc{
  public:
      enum{
          MOD_COUNTER, MOD_TRANSLATER, MOD_CONTROLLER, MOD_ECM,
          MOD_VEPC,
          MAX
          };
  public:
      static int SetAffinity(int);
      static int GetCpucores(void);
      static int GetInterface(const char*, char*, unsigned , char* , unsigned );
      static int IsExists(const char*);
      static unsigned long long GetMillisecond(void);
      static unsigned long long GetMicrosecond(void);
      static unsigned long long GetMicrosecondArround(void);
      static void SigintH(int);
      static void UsageWithExit(int);
      static int GetIpv(const char*,  unsigned char*, unsigned*);
      static int GetIpv4(const char*, unsigned char*, unsigned*);
      static int GetIpv6(const char*, unsigned char*, unsigned*);
      static int GetIpvMask(const char* , unsigned char* , unsigned* , unsigned char*);
      static unsigned Checksum(const void *, unsigned short , unsigned );
      static unsigned short Wrapsum(unsigned );
      static const char* Norm(char*, double);
      //
      static int GetOpt(int , char** , const char* , char*, int);
      static int GetModuleid(void);
      static void SetModuleid(int);
      //
      static void SetModuleCfg(const char*);
      static void GetModuleCfg(char*,int);
      //
      static void InitCommandParse(int , char**,class ProcessParameter*);
      static unsigned short TeidGid(const unsigned);
      static unsigned short TeidGcnt(const unsigned);
  private:
      static int module_id_;
  }; // class Misc
}; // namespace MIXIPGW_TOOLS


#endif //MIXIPGW_TOOLS_MISC_H
