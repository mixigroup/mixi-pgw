/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       entcy.cc
    @brief      main entry point
*******************************************************************************
	make ARCH=[x]\n
    Specify Application type at compile time by ARCH=[x]\n
	\n
	\n
	\n
*******************************************************************************
    @date       created(27/sep/2017)

    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 27/sep/2017 
      -# Initial Version
******************************************************************************/
#include "mixi_pgw_data_plane_def.hpp"

#ifdef ARCH
#if ARCH==15 || ARCH==16 || ARCH==17 || ARCH==18 || ARCH==19 || ARCH==20 || ARCH==21 
        #include "../inc/app_v2.hpp"
    #else
        #include "../inc/app.hpp"
    #endif
#else
#error not found architecture.
#endif

#include <stdexcept>
#include <memory>
#include <functional>
#include <vector>
#include <string>
#include <map>

/**
   mixi_pgw_data_plane : main entry.\n
   *******************************************************************************
   initialize application, and start main loop.\n
   startup arguments are passed directly to
   dpdk initialization function(rte_eal_init)\n
   *******************************************************************************
   @param[in]     argc   count of arguments.
   @param[in]     argv   arguments
   @return        0/success , !=0/error
 */
int main(int argc, char **argv){
    int ret = 0;
#ifndef __NODAEMON__
if (daemon(1,0) != 0){
        exit(1);
    }else {
#else
    {
#endif

#ifdef ARCH
    #if ARCH==15 || ARCH==16 || ARCH==17 || ARCH==18 || ARCH==19 || ARCH==20 || ARCH==21
        std::unique_ptr<MIXIPGW::AppV2> app(new MIXIPGW::AppV2(argc, argv));
	app.get()->LoadConfig();
	app.get()->SwapConfig();
    #else
        std::unique_ptr<MIXIPGW::App> app(MIXIPGW::App::Init(argc, argv));
    #endif
        ret = app.get()->Run();
#else
#error not found architecture.
#endif
    }
    return(ret);
}
