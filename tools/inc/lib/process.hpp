//
// Created by mixi on 2017/04/21.
//

#ifndef MIXIPGW_TOOLS_PROCESS_HPP
#define MIXIPGW_TOOLS_PROCESS_HPP
//
namespace MIXIPGW_TOOLS{
  class FilterContainer;
  // process parameters
  class ProcessParameter{
  public:
      typedef enum _ULL_{
          ULL_PACKET_CNT, ULL_EVENT_CNT, ULL_ERROR_CNT, ULL_MAX
      }ULL_;
      typedef enum _USH_{
          USH_UDP_SRC_PORT, USH_UDP_DST_PORT, USH_UDP_CTRL_PORT, USH_UDP_NTY_PORT,
          USH_IF_SRC_EXT_IDX, USH_IF_DST_EXT_IDX, USH_IF_TX_EXT_IDX,
          USH_MAX
      }USH_;
      typedef enum _USG_{
          USG_THREAD_CNT, USG_IF_SRC_IPV4, USG_IF_DST_IPV4, USG_ATTACH_CPUID,
          USG_MODULE_FLAG, USG_DBG_PARAM, USG_FILTER_EXT, USG_NEXT_HOP_IPV4,
          USG_NEXT_HOP_VLANID, 
          USG_MAX
      }USG_;
      typedef enum _TXT_{
          TXT_CLASS, TXT_IF_SRC, TXT_IF_DST, TXT_IF_BASE,
          TXT_MAC_SRC, TXT_MAC_DST, TXT_TRGT_NTY, TXT_IF_NAME,
          TXT_MAX
      }TXT_;
      typedef enum _TBL_{
          TBL_TEID, TBL_POLICER, TBL_COUNTER_I, TBL_COUNTER_E,
          TBL_POLICY,
          TBL_MAX
      }TBL_;
      static const unsigned TBL_SKEY_MAX = 16;
  public:
      ProcessParameter();
      ProcessParameter(FilterContainer*);
      ~ProcessParameter();
  public:
      void Set(_TXT_,const char*);
      void Set(_USG_,const unsigned);
      void Set(_USH_,const unsigned short);
      void Set(_ULL_,const unsigned long long);
      void Set(_TBL_,const unsigned,void*);
      void Set(FilterContainer*);
      //
      const char* Get(_TXT_);
      const unsigned Get(_USG_);
      const unsigned short Get(_USH_);
      const unsigned long long Get(_ULL_);
      void* Get(_TBL_,unsigned);
      FilterContainer* Get(void);
      //
      void Copy(_TXT_,_TXT_);
      void Copy(_USG_,_USG_);
      void Copy(_USH_,_USH_);
      void Copy(_ULL_,_ULL_);
  private:
      char str_buf_[TXT_MAX][64];
      unsigned long long ull_buf_[ULL_MAX];
      unsigned short ush_buf_[USH_MAX];
      unsigned usg_buf_[USG_MAX];
      void* lookup_tbl_[TBL_MAX][TBL_SKEY_MAX];
      FilterContainer* filters_;
  };
}; // namespace MIXIPGW_TOOLS
#endif //MIXIPGW_TOOLS_PROCESS_HPP
