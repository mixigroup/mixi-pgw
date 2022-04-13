/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       app_v2.hpp
    @brief      application define(version 2)
*******************************************************************************
   \n
*******************************************************************************
    @date       created(26/apr/2018)

    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 26/apr/2018 
      -# Initial Version
******************************************************************************/
#ifndef __APP_V2_MIXIPGW_HPP
#define __APP_V2_MIXIPGW_HPP
#include "app.hpp"

namespace MIXIPGW{
  class CoreInterface;
  typedef std::vector<CoreInterface*>         VCores;
  typedef std::unordered_map<COREID, VCores>  VCoresMap;

  /** *****************************************************
  * @brief
  * application Version 2\n
  * \n
  */
  class AppV2 : public App{
  public:
      AppV2(int , char **);
      virtual ~AppV2();
  public:
      void AddCores(COREID, VCores&);   /*!< add Cores group */
      virtual RETCD Run(void);          /*!< main loop */
      virtual RETCD Connect(COREID, COREID, SIZE);  /*!< connect to ring */
      virtual RETCD Commit(StaticConf*);
  protected:
      static int DispatchV2(void*);     /*!< dispatch(v2) */
  private:
      VCoresMap vcores_map_;            /*!< cores group  */
  private:
      AppV2();
  };
}; // namespace MIXIPGW

#endif //__APP_V2_MIXIPGW_HPP
