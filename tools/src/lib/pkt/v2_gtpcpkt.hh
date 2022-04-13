//
// Created by mixi on 19/dec/2016.
//
#include "lib/packet/gtpc.hpp"

namespace MIXIPGW_TOOLS {
  typedef struct find_container{
      int     retcd;
      int     typecd;
      void*   instance;
  }find_container_t,*find_container_ptr;

  template<typename T> T* GtpcPkt::find(void){
      find_container_t    container{RETERR, 0, NULL};
      T                   t;
      container.typecd = t.type();
      //
      iterate([](GtpcItem* i, void* u)->int{
          find_container_ptr pc = (find_container_ptr)u;
          if (i->type() == pc->typecd){
              T* itm = new T();
              if (itm->attach(i->ref(), i->len()) != RETOK){
                  delete itm;
                  return(RETERR);
              }
              pc->retcd = RETOK;
              pc->instance = itm;
              //
              return(RETFOUND);
          }
          return(RETNEXT);
      },&container);
      if (container.retcd == RETOK){
          return((T*)container.instance);
      }
      return(NULL);
  }
} // namespace MIXIPGW_TOOLS
