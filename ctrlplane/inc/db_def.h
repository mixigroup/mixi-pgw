/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       db_def.h
    @brief      mixi_pgw_ctrl_plane database:c structures: common header
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

#ifndef MIXI_PGW_DB_DEF_H
#define MIXI_PGW_DB_DEF_H

#include "pgw_def.h"
#include <mysql.h>
#include <sqlite3.h>


#define MYSQL_MAXBIND           (64)
#define MYSQL_MAXCOLLEN         (128)

#pragma pack(push,4)

/*! @struct dbbind
    @brief
    database access: columns\n
    \n
*/
typedef struct dbbind{
    MYSQL_BIND  bind[MYSQL_MAXBIND];
    char        data[MYSQL_MAXBIND][MYSQL_MAXCOLLEN];
    unsigned long length[MYSQL_MAXBIND];
    bool        errn[MYSQL_MAXBIND];
    bool        null[MYSQL_MAXBIND];
}dbbind_t,*dbbind_ptr;


/*! @struct colmn
    @brief
    database access: columns\n
    \n
*/
typedef struct colmn{
    char    name[64];
}colmn_t,*colmn_ptr;

/*! @struct record
    @brief
    database access: records\n
    \n
*/
typedef struct record{
    U32 flag;       // (1<<31)==sval,!=nval
    union {
        char    sval[MYSQL_MAXCOLLEN*4];
        U64     nval;
    }u;
}record_t,*record_ptr;


#pragma pack(pop)

// recordset callback function
typedef RETCD (*on_event_mysql_execute)(U32, U32, colmn_ptr, record_ptr, void*);

/*! @struct sqlparse
    @brief
    sql parse structure\n
    \n
*/
#define SQLPARSE_ILEN   (32)
typedef struct sqlparse{
    char        q[SQLPARSE_ILEN];
    unsigned    len;
}sqlparse_t,*sqlparse_ptr;



/*! @struct __sqlite3_st
    @brief
    mysql_c_connector compatible\n
    sqlite3 handle\n
    \n
*/
typedef struct __sqlite3_st{
    int         stat;
    sqlite3*    dbh;
    // for store result.
    record_ptr  rec;
    U32         rcnt;
    U32         counter;
    size_t      clmncnt;
    colmn_t     colmn[MYSQL_MAXBIND];
    int         colmntype[MYSQL_MAXBIND];
}__SQLITE3_ST,*__SQLITE3_ST_PTR;

/*! @struct __sqlite3_st_stmt
    @brief
    mysql_c_connector compatible\n
    sqlite3 - statement handle\n
    \n
*/
typedef struct __sqlite3_st_stmt{
    int             stat;
    sqlite3*        dbh;
    char*           sql;
    sqlite3_stmt*   stmt;
}__SQLITE3_ST_STMT,*__SQLITE3_ST_STMT_PTR;


/*! @struct __sqlite3_st_res
    @brief
    mysql_c_connector compatible\n
    sqlite3 - resource handle\n
    \n
*/
typedef struct __sqlite3_st_res{
    int             stat;
    char**          rec;
    U32             rcnt;
    U32             counter;
    U32             curpos;
    size_t          clmncnt;
    colmn_t         colmn[MYSQL_MAXBIND];
    int             colmntype[MYSQL_MAXBIND];
}__SQLITE3_ST_RES,*__SQLITE3_ST_RES_PTR;


/*! @struct __sqlite3_bind
    @brief
    mysql_c_connector compatible\n
    sqlite3 - bind parameters handle\n
    \n
*/
typedef struct __sqlite3_bind{
    int     stat;
    int     buffer_type;
    void*   buffer;
    int     is_null;
}__SQLITE3_BIND;

/*! @struct __sqlite3_bind
    @brief
    mysql_c_connector compatible\n
    sqlite3 - bind parameters handle array\n
    \n
*/
typedef struct __sqlite3_st_bind{
    int     stat;
    __SQLITE3_BIND  bind[64];
}__SQLITE3_ST_BIND,*__SQLITE3_ST_BIND_PTR;


/*! @struct __sqlite3_st_row
    @brief
    mysql_c_connector compatible\n
    sqlite3 - datarows handle\n
    \n
*/
typedef char** __SQLITE3_ST_ROW_PTR;

/*! @struct __sqlite3_skip_sql
    @brief
    mysql_c_connector compatible\n
    sqlite3 - skip dml(tables)\n
    \n
*/
typedef struct __sqlite3_skip_sql{
    const char* first;
    const char* second;
}__sqlite3_skip_sql_t;


/*! @struct __sqlite3_container
    @brief
    mysql_c_connector compatible\n
    sqlite3 - callback user container(internal)\n
    \n
*/
typedef struct __sqlite3_container{
    on_event_mysql_execute callback;
    void*       udata;
    sqlite3*    dbh;
    sqlite3_stmt* stmt;
    int         exists;
    U32         counter;
    size_t      clmncnt;
    colmn_t     colmn[MYSQL_MAXBIND];
    int         colmntype[MYSQL_MAXBIND];
}__sqlite3_container_t,*__sqlite3_container_ptr;


/*! @name dbdriver exchange */
/* @{ */
#ifndef __USESQLITE3_ON_TEST__
#define DBPROVIDER_HANDLE       MYSQL*
#define DBPROVIDER_RES_HANDLE   MYSQL_RES*
#define DBPROVIDER_ROW_HANDLE   MYSQL_ROW*
#define DBPROVIDER_STMT_HANDLE  MYSQL_STMT*
#define DBPROVIDER_ST_BIND_I    dbbind_t
#define DBPROVIDER_STBIND       dbbind_ptr
#define DBPROVIDER_BIND         MYSQL_BIND

#else
#define DBPROVIDER_HANDLE       __SQLITE3_ST_PTR
#define DBPROVIDER_STMT_HANDLE  __SQLITE3_ST_STMT_PTR
#define DBPROVIDER_RES_HANDLE   __SQLITE3_ST_RES_PTR
#define DBPROVIDER_ROW_HANDLE   __SQLITE3_ST_ROW_PTR
#define DBPROVIDER_ST_BIND_I    __SQLITE3_ST_BIND
#define DBPROVIDER_STBIND       __SQLITE3_ST_BIND_PTR
#define DBPROVIDER_BIND         __SQLITE3_BIND
#endif /* __USESQLITE3_ON_TEST__ */
/* @} */

#endif //MIXI_PGW_DB_DEF_H
