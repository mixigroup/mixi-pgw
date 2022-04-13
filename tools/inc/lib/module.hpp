//
// Created by mixi on 2017/04/21.
//

#ifndef MIXIPGW_TOOLS_INST_HPP
#define MIXIPGW_TOOLS_INST_HPP

namespace MIXIPGW_TOOLS{
  // module object(module == process, unique instance in process)
  // available global access to abort flag, debug level
  class Module{
  public:
      static int  Init(Module**);
      static int  Uninit(Module**);
  public:
      static int  ABORT(void);
      static void ABORT_INCR(void);
      static void ABORT_CLR(void);
      static int  VERBOSE(void);
      static void VERBOSE_INCR(void);
      static void VERBOSE_CLR(void);
  private:
      int abort_;
      int verbose_;
      static Module* pthis_;
  private:
      Module();
      ~Module();
  }; // class Module
}; // namespace MIXIPGW_TOOLS
#endif //MIXIPGW_TOOLS_INST_HPP
