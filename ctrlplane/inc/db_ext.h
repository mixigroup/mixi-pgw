/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       db_ext.h
    @brief      mixi_pgw_ctrl_plane database c function defined: common headers
*******************************************************************************
*******************************************************************************
    @date       created(14/nov/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license
*******************************************************************************
    @par        History
    - 14/nov/2017 
      -# Initial Version
******************************************************************************/
#ifndef MIXI_PGW_DB_EXT_H
#define MIXI_PGW_DB_EXT_H

#include "db_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 mysql SQL call + records reference\n
 *******************************************************************************
 \n
 *******************************************************************************
 @parma[in]   stmt      mysql statement
 @param[in]   bind      mysql bind
 @parma[in]   callback  every record / callback
 @param[in]   userdata  user data on callback
 @return int  0==OK,0!=error
*/
RETCD pgw_mysql_execute(MYSQL_STMT* stmt, MYSQL_BIND* bind, on_event_mysql_execute callback, void* userdata);

/**
 mysql initialize bind parameters\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   bind      mysql bind
 @return int  0==OK,0!=error
*/
RETCD pgw_mysql_bind_init(dbbind_ptr bind);

/**
 sql parser\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]     sql    SQL string
 @param[in/out] parsed parsed pointer array
 @param[in]     count  buffer length
 @return int    0==OK,0!=error
*/
RETCD pgw_parse_sql(const char* sql, sqlparse_ptr parsed, unsigned count);

/**
 for local testing, change to sqlite3\n
*/
#ifndef __USESQLITE3_ON_TEST__
#define DBPROVIDER_APP_INIT     void
#define DBPROVIDER_THREAD_INIT  mysql_thread_init
#define DBPROVIDER_INIT         mysql_init
#define DBPROVIDER_OPTIONS      mysql_options
#define DBPROVIDER_REAL_CONNECT mysql_real_connect
#define DBPROVIDER_QUERY        mysql_query
#define DBPROVIDER_STMT_INIT    mysql_stmt_init
#define DBPROVIDER_STMT_PREPARE mysql_stmt_prepare
#define DBPROVIDER_STMT_CLOSE   mysql_stmt_close
#define DBPROVIDER_STMT_ERROR   mysql_stmt_error
#define DBPROVIDER_STMT_RESET   mysql_stmt_reset
#define DBPROVIDER_STMT_BIND_PARAM  mysql_stmt_bind_param
#define DBPROVIDER_CLOSE        mysql_close
#define DBPROVIDER_SET_CHAR     mysql_set_character_set
#define DBPROVIDER_ERROR        mysql_error
#define DBPROVIDER_STORE_RESULT mysql_store_result
#define DBPROVIDER_NUM_ROWS     mysql_num_rows
#define DBPROVIDER_FETCH_ROW    mysql_fetch_row
#define DBPROVIDER_FREE_RESULT  mysql_free_result
#define DBPROVIDER_ERRNO        mysql_errno

#define DBPROVIDER_BIND_INIT    pgw_mysql_bind_init
#define DBPROVIDER_EXECUTE      pgw_mysql_execute

#else

/**
 sqlite3 - application initialize only once\n
 *******************************************************************************
 \n
 *******************************************************************************
 @return int  0==OK,0!=error
*/
void __sqlite3_app_init(void);
/**
 sqlite3 - thread initialize only once\n
 *******************************************************************************
 not implemented\n
 *******************************************************************************
 @return int  0==OK,0!=error
*/
int __sqlite3_thread_init(void);
/**
 sqlite3 - get database provider handle\n
 *******************************************************************************
 instanciate sqlite3 driver\n
 *******************************************************************************
 @param[in]   handle  database handle, not used
 @return int  0==OK,0!=error
*/
DBPROVIDER_HANDLE __sqlite3_init(DBPROVIDER_HANDLE handle);
/**
 sqlite3 - execute query\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   handle    database handle
 @param[in]   q         query:SQL
 @return int  0==OK,0!=error
*/
int __sqlite3_query(DBPROVIDER_HANDLE handle, const char *q);
/**
 sqlite3 - set options\n
 *******************************************************************************
not implemented\n
 *******************************************************************************
 @param[in]   handle    database handle
 @param[in]   option    option key
 @param[in]   arg       option value
 @return int  0==OK,0!=error
*/
int __sqlite3_options(DBPROVIDER_HANDLE handle,unsigned option, const void *arg);
/**
 sqlite3 - connect to database\n
 *******************************************************************************
 sqlite3 - onlye single instance\n
 *******************************************************************************
 @param[in]   handle    database handle
 @param[in]   host      <not implemented> database host
 @param[in]   user      <not implemented> database user
 @param[in]   passwd    <not implemented> database password
 @param[in]   db        <not implemented> database instance
 @param[in]   port      <not implemented> database port
 @param[in]   unix_socket <not implemented> database unix socket
 @param[in]   clientflag <not implemented> option flags
 @return void* NULL!=OK,NULL==error
*/
DBPROVIDER_HANDLE __sqlite3_real_connect(DBPROVIDER_HANDLE handle, const char *host,
                                         const char *user,
                                         const char *passwd,
                                         const char *db,
                                         unsigned int port,
                                         const char *unix_socket,
                                         unsigned long clientflag);
/**
 sqlite3 - generate statement instance\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   handle    database handle
 @return void* NULL!=OK:statement instance,NULL==error
*/
DBPROVIDER_STMT_HANDLE __sqlite3_stmt_init(DBPROVIDER_HANDLE handle);

/**
 sqlite3 - preparing statement\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   handle    statement handle
 @param[in]   query     query string
 @param[in]   lenght    <not implemented> length of query string
 @return int  0==OK,0!=error
*/
int __sqlite3_stmt_prepare(DBPROVIDER_STMT_HANDLE handle, const char *query, unsigned long length);
/**
 sqlite3 - release statement handle\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   stmt      statement handle
 @return int  0==OK,0!=error
*/
int __sqlite3_stmt_close(DBPROVIDER_STMT_HANDLE stmt);

/**
 sqlite3 - string of error statement\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   stmt      statement handle
 @return const char* error message,NULL available
*/
const char *__sqlite3_stmt_error(DBPROVIDER_STMT_HANDLE stmt);

/**
 sqlite3 - reset statement\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   stmt      statement handle
 @return int 0==ok,0!=error
*/
int __sqlite3_stmt_reset(DBPROVIDER_STMT_HANDLE stmt);


/**
 sqlite3 - statement bind parameters\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   stmt      statement handle
 @param[in]   bnd       bind handle
 @return int 0==ok,0!=error
*/
int __sqlite3_stmt_bind_param(DBPROVIDER_STMT_HANDLE stmt, DBPROVIDER_BIND * bnd);



/**
 sqlite3 - release database handle\n
 *******************************************************************************
 release when reference count == 0\n
 *******************************************************************************
 @param[in]   handle    database handle
*/
void __sqlite3_close(DBPROVIDER_HANDLE handle);
/**
 sqlite3 - initialize bind parameters\n
 *******************************************************************************
 <not implemented> \n
 *******************************************************************************
 @param[in]   bind   bind parameters
 @return int  0==OK,0!=error
*/
int __sqlite3_bind_init(DBPROVIDER_STBIND bind);

/**
 sqlite3 - execute sql query to get records\n
 *******************************************************************************
 <not implemented> \n
 *******************************************************************************
 @param[in]   stmt     statement handle
 @param[in]   bind     bind parameters
 @param[in]   callback callback
 @param[in]   userdata userdata
 @return int  0==OK,0!=error
*/
int __sqlite3_execute(DBPROVIDER_STMT_HANDLE stmt, DBPROVIDER_BIND* bind, on_event_mysql_execute callback, void* userdata);
/**
 sqlite3 - set client charactor set\n
 *******************************************************************************
not implemented\n
 *******************************************************************************
 @param[in]   handle    database handle
 @param[in]   csname    charactor set(ex.utf-8)
 @return int  0==OK,0!=error
*/
int __sqlite3_set_character_set(DBPROVIDER_HANDLE mysql, const char *csname);

/**
 sqlite3 - last error\n
 *******************************************************************************
 <not implemented> \n
 *******************************************************************************
 @param[in]   handle   instance  handle
 @return char* !=NULL/message,==NULL/OK
*/
const char * __sqlite3_error(DBPROVIDER_HANDLE handle);
/**
 sqlite3 - save records\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   mysql     database instance  handle
 @return resource handle NULL==error,!=NULL,OK
*/
DBPROVIDER_RES_HANDLE __sqlite3_store_result(DBPROVIDER_HANDLE mysql);
/**
 sqlite3 - count of saved records\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   res      resource  handle
 @return unsigned long long unsigned long long(-1):error, count of records
*/
unsigned long long __sqlite3_num_rows(DBPROVIDER_RES_HANDLE res);
/**
 sqlite3 - access saved records\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   res      resource  handle
 @return struct __sqlite3_st_row*=records, NULL=EOF
*/
DBPROVIDER_ROW_HANDLE __sqlite3_fetch_row(DBPROVIDER_RES_HANDLE res);
/**
 sqlite3 - release records\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   res      resource  handle
*/
void __sqlite3_free_result(DBPROVIDER_RES_HANDLE res);

/**
 sqlite3 - last error\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   handle      database handle
 @return unsigned int error code
*/
unsigned int __sqlite3_errno(DBPROVIDER_HANDLE handle);






/**
 swithing to sqlite3 when local testing.\n
*/
#define DBPROVIDER_APP_INIT     __sqlite3_app_init
#define DBPROVIDER_THREAD_INIT  __sqlite3_thread_init
#define DBPROVIDER_INIT         __sqlite3_init
#define DBPROVIDER_OPTIONS      __sqlite3_options
#define DBPROVIDER_REAL_CONNECT __sqlite3_real_connect
#define DBPROVIDER_QUERY        __sqlite3_query
#define DBPROVIDER_STMT_INIT    __sqlite3_stmt_init
#define DBPROVIDER_STMT_PREPARE __sqlite3_stmt_prepare
#define DBPROVIDER_STMT_CLOSE   __sqlite3_stmt_close
#define DBPROVIDER_STMT_ERROR   __sqlite3_stmt_error
#define DBPROVIDER_STMT_RESET   __sqlite3_stmt_reset
#define DBPROVIDER_STMT_BIND_PARAM  __sqlite3_stmt_bind_param
#define DBPROVIDER_CLOSE        __sqlite3_close
#define DBPROVIDER_SET_CHAR     __sqlite3_set_character_set
#define DBPROVIDER_ERROR        __sqlite3_error
#define DBPROVIDER_STORE_RESULT __sqlite3_store_result
#define DBPROVIDER_BIND_INIT    __sqlite3_bind_init
#define DBPROVIDER_EXECUTE      __sqlite3_execute
#define DBPROVIDER_NUM_ROWS     __sqlite3_num_rows
#define DBPROVIDER_FETCH_ROW    __sqlite3_fetch_row
#define DBPROVIDER_FREE_RESULT  __sqlite3_free_result
#define DBPROVIDER_ERRNO        __sqlite3_errno







#endif  /* __USESQLITE3_ON_TEST__ */



#ifdef __cplusplus
}
#endif


#endif //MIXI_PGW_DB_EXT_H
