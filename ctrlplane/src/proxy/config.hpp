/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane_proxy
    @file       config.hpp
    @brief      mixi_pgw_ctrl_plane_proxy / c++ config header
*******************************************************************************
*******************************************************************************
    @date       created(09/apr/2018)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 09/apr/2018 
      -# Initial Version
******************************************************************************/

#ifndef XPGW_CONFIG_HH
#define XPGW_CONFIG_HH
#include "proxy_def.hpp"



/** *****************************************************
 * @brief
 * Config\n
 * \n
 */
class Config{
public:
    Config(const char*);
    virtual ~Config();
public:
    int GetInt(const char*);
    const char* GetText(const char*);
private:
    int loadConfig(const char*);
private:
    CFGMAP          keyvalues_;
private:
    Config(){}
};



#endif //XPGW_CONFIG_HH
