/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane_proxy
    @file       database_config.hpp
    @brief      mixi_pgw_ctrl_plane_proxy / database config header
*******************************************************************************
*******************************************************************************
    @date       created(09/apr/2018)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 NetworkTeam. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 09/apr/2018
      -# Initial Version
******************************************************************************/

#ifndef XPGW_DATABASE_CONFIG_HH
#define XPGW_DATABASE_CONFIG_HH
#include "proxy_def.hpp"


class Config;
/** *****************************************************
 * @brief
 * Database Config\n
 * \n
 */
class DatabaseConfig{
public:
    DatabaseConfig(const char*,const char*);
    virtual ~DatabaseConfig();
public:
    const int Port(void);
    const char* Host(void);
    const char* User(void);
    const char* Pswd(void);
    const char* Inst(void);
private:
    Config  *config_;
    SSTR    suffix_;
private:
    DatabaseConfig(){}
};


#endif //XPGW_DATABASE_CONFIG_HH
