/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane_proxy
    @file       database_connection.hpp
    @brief      mixi_pgw_ctrl_plane_proxy / database connection header
*******************************************************************************
*******************************************************************************
    @date       created(10/apr/2018)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 NetworkTeam. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 10/apr/2018 
      -# Initial Version
******************************************************************************/

#ifndef XPGW_DBCON_HPP
#define XPGW_DBCON_HPP
#include "proxy_def.hpp"

class DatabaseConfig;

typedef int (*OnSelect)(MYSQL_ROW, U32, void*);

/** *****************************************************
 * @brief
 * DatabaseConnection\n
 * \n
 */
class DatabaseConnection{
public:
    DatabaseConnection(DatabaseConfig*);
    virtual ~DatabaseConnection();
public:
    int Select(const char*, OnSelect, void*);
    int Execute(const char*);
    int Ping(void);

private:
    MYSQL*  dbh_;
};

#endif //XPGW_DBCON_HPP
