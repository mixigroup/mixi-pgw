/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane_proxy
    @file       dbcon.cc
    @brief      general: database connection
*******************************************************************************
*******************************************************************************
    @date       created(10/apr/2018)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 10/apr/2018 
      -# Initial Version
******************************************************************************/
#include "proxy.hpp"
#include "database_config.hpp"
#include "database_connection.hpp"



/**
   database connection  :  constructor \n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]     cfg  config instance
 */
DatabaseConnection::DatabaseConnection(DatabaseConfig* cfg){
    ASSERT(cfg);
    dbh_ = mysql_init(NULL);
    ASSERT(dbh_);
    auto ret = mysql_real_connect(dbh_,
                                  cfg->Host(),
                                  cfg->User(),
                                  cfg->Pswd(),
                                  cfg->Inst(),
                                  cfg->Port(),
                                  NULL,0);
    if (!ret){
        ASSERT(!"failed. mysql_real_connect");
    }
    mysql_set_character_set(dbh_, "utf8");
}
/**
   database connection  : destructor\n
   *******************************************************************************
   destructor\n
   *******************************************************************************
 */
DatabaseConnection::~DatabaseConnection(){
    ASSERT(dbh_);
    mysql_close(dbh_);
}
/**
  database connection : select\n
 *******************************************************************************
 + \n
 *******************************************************************************
 @param[in]     sql    SQL string
 @param[in/out] fnc    callback
 @param[in]     dat    callback data
 @return        0/success , !=0/error
 */
int DatabaseConnection::Select(const char* sql, OnSelect fnc, void* dat){
    ASSERT(dbh_);
    ASSERT(fnc);
    MYSQL_ROW   row;
    auto ret = mysql_query(dbh_, sql);
    auto res = mysql_store_result(dbh_);
    if (!res || ret){
        // empty
        return(ERR);
    }
    auto coln = mysql_num_fields(res);
    auto rown = mysql_num_rows(res);
    if (rown > 0){
        while ((row = mysql_fetch_row(res))) {
            if (fnc(row, coln, dat) != OK){ break; }
        }
    }
    mysql_free_result(res);
    return(OK);
}
/**
  database connection : exec\n
 *******************************************************************************
 + \n
 *******************************************************************************
 @param[in]     sql    SQL string
 @return        0/success , !=0/error
 */
int DatabaseConnection::Execute(const char* sql){
    ASSERT(dbh_);
    auto ret = mysql_query(dbh_, sql);
    if (ret){
        return(ERR);
    }
    return(OK);
}

/**
  database connection : ping\n
 *******************************************************************************
 + \n
 *******************************************************************************
 @return        0/success , !=0/error
 */
int DatabaseConnection::Ping(void){
    ASSERT(dbh_);
    auto ret = mysql_ping(dbh_);
    if (ret){
        return(ERR);
    }
    return(OK);
}


