/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       driver.c
    @brief      database operation
*******************************************************************************
*******************************************************************************
    @date       created(30/Aug/2018)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2018 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 30/aug/2018
      -# Initial Version
******************************************************************************/

#include "db_ext.h"
#define __SQLITE3_DBNM ("sqlite3_on_test")

//
static __SQLITE3_ST_PTR __sqlite3_single_instance = NULL;
static pthread_mutex_t __sqlite3_mutex;
static unsigned __sqlite3_reference_count = 0;
// internal usecallback -> data store
static int __sqlite3_callback_store(void *udata, int cnt, char **vals, char **columns);
//
const __sqlite3_skip_sql_t __skip_sql[] = {
    {"DROP", "DATABASE"},
    {"CREATE", "DATABASE"},
    {"USE", ""},
    {NULL, NULL}};

/**
 sqlite3 - initialize once in application\n
 *******************************************************************************
 \n
 *******************************************************************************
 @return int  0==OK,0!=error
*/
void __sqlite3_app_init(void)
{
    pthread_mutex_init(&__sqlite3_mutex, NULL);
}

/**
 sqlite3 - initialize once in thread\n
 *******************************************************************************
not implemented\n
 *******************************************************************************
 @return int  0==OK,0!=error
*/
int __sqlite3_thread_init(void)
{
    return (0);
}
/**
 sqlite3 - get database provider handle\n
 *******************************************************************************
 sqlite3 driver , only allocate instance\n
 *******************************************************************************
 @param[in]   handle  database handle , not supported
 @return int  0==OK,0!=error
*/
DBPROVIDER_HANDLE __sqlite3_init(DBPROVIDER_HANDLE handle)
{
    DBPROVIDER_HANDLE p = NULL;
    // singleton
    pthread_mutex_lock(&__sqlite3_mutex);
    if (!__sqlite3_single_instance)
    {
        __sqlite3_single_instance = (DBPROVIDER_HANDLE)malloc(sizeof(__SQLITE3_ST));
        __sqlite3_reference_count = 1;
    }
    else
    {
        __sqlite3_reference_count++;
    }
    p = __sqlite3_single_instance;
    pthread_mutex_unlock(&__sqlite3_mutex);
    if (!p)
    {
        return (NULL);
    }
    bzero(p, sizeof(*p));
    return (p);
}
/**
 sqlite3 - set client charactor set\n
 *******************************************************************************
not implemented\n
 *******************************************************************************
 @param[in]   handle    database handle
 @param[in]   csname    charactor set(ex, utf-8)
 @return int  0==OK,0!=error
*/
int __sqlite3_set_character_set(DBPROVIDER_HANDLE handle, const char *csname)
{
    return (0);
}
/**
 sqlite3 - execute query\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   handle    database handle
 @param[in]   q         query : SQL
 @return int  0==OK,0!=error
*/
int __sqlite3_query(DBPROVIDER_HANDLE handle, const char *sql)
{
    sqlparse_t parsed_sql[32];
    int n = 0;
    const char *cn;
    char *errmsg = NULL;
    sqlite3_stmt *stmt;

    //
    if (!handle || !sql)
    {
        return (ERR);
    }
    if (strlen(sql) == 0)
    {
        return (ERR);
    }

    bzero(parsed_sql, sizeof(parsed_sql));
    if (pgw_parse_sql(sql, parsed_sql, 31) < 2)
    {
        return (ERR);
    }

    // check SQL skipped
    for (n = 0; __skip_sql[n].first != NULL; n++)
    {
        if (strlen(parsed_sql[0].q) == strlen(__skip_sql[n].first) &&
            strncasecmp(parsed_sql[0].q, __skip_sql[n].first, strlen(__skip_sql[n].first)) == 0 &&
            strlen(parsed_sql[1].q) == strlen(__skip_sql[n].second) &&
            strncasecmp(parsed_sql[1].q, __skip_sql[n].second, strlen(__skip_sql[n].second)) == 0)
        {
            return (OK);
        }
        else if (strlen(parsed_sql[0].q) == strlen(__skip_sql[n].first) &&
                 strncasecmp(parsed_sql[0].q, __skip_sql[n].first, strlen(__skip_sql[n].first)) == 0 &&
                 strlen(__skip_sql[n].second) == 0)
        {
            return (OK);
        }
    }
    if (handle->rec)
    {
        free(handle->rec);
    }
    handle->counter = 0;
    handle->rcnt = 0;
    handle->rec = NULL;

    pthread_mutex_lock(&__sqlite3_mutex);
    // meta information
    if (sqlite3_prepare_v2(handle->dbh, sql, -1, &stmt, NULL) != SQLITE_OK)
    {
        pthread_mutex_unlock(&__sqlite3_mutex);
        return (ERR);
    }
    sqlite3_reset(stmt);

    handle->clmncnt = sqlite3_column_count(stmt);
    for (n = 0; n < handle->clmncnt; n++)
    {
        cn = sqlite3_column_decltype(stmt, n);
        // all columns are retrieved as NULL when sqlite3_column_type is used.
        if (cn == NULL)
        {
            handle->colmntype[n] = SQLITE_INTEGER;
        }
        else if (strncasecmp(cn, "TEXT", 4) == 0)
        {
            handle->colmntype[n] = SQLITE_TEXT;
        }
        else
        {
            handle->colmntype[n] = SQLITE_INTEGER;
        }
    }
    sqlite3_finalize(stmt);
    stmt = NULL;
    //
    if (sqlite3_exec(handle->dbh, sql, __sqlite3_callback_store, handle, &errmsg) != SQLITE_OK)
    {
        printf("sqlite3_exec(%s)\n", sqlite3_errmsg(handle->dbh));
        if (errmsg)
        {
            sqlite3_free(errmsg);
        }
        pthread_mutex_unlock(&__sqlite3_mutex);
        return (ERR);
    }
    if (errmsg)
    {
        sqlite3_free(errmsg);
    }
    pthread_mutex_unlock(&__sqlite3_mutex);

    return (0);
}
/**
 sqlite3 - setup option values\n
 *******************************************************************************
not implemented\n
 *******************************************************************************
 @param[in]   handle    database handle
 @param[in]   option    option key
 @param[in]   arg       option value pointer
 @return int  0==OK,0!=error
*/
int __sqlite3_options(DBPROVIDER_HANDLE handle, unsigned option, const void *arg)
{
    return (0);
}
/**
 sqlite3 - connect to database\n
 *******************************************************************************
 sqlite3 - driver, only single instance\n
 *******************************************************************************
 @param[in]   handle    database handle
 @param[in]   host      <not implemented> database host
 @param[in]   user      <not implemented> database user
 @param[in]   passwd    <not implemented> database password
 @param[in]   db        <not implemented> database instance
 @param[in]   port      <not implemented> database port number 
 @param[in]   unix_socket <not implemented> database UNIX socket name
 @param[in]   clientflag <not implemented> option flags
 @return void* NULL!=OK , NULL==error
*/
DBPROVIDER_HANDLE __sqlite3_real_connect(DBPROVIDER_HANDLE handle,
                                         const char *host,
                                         const char *user,
                                         const char *passwd,
                                         const char *db,
                                         unsigned int port,
                                         const char *unix_socket,
                                         unsigned long clientflag)
{
    if (!handle)
    {
        return (NULL);
    }
    if (handle != __sqlite3_single_instance)
    {
        printf("invalid instance[%p/%p]", handle, __sqlite3_single_instance);
        return (NULL);
    }
    // allready connected
    if (handle->dbh != NULL)
    {
        printf("already connected[%p/%p]", handle, __sqlite3_single_instance);
        return (handle);
    }
    // on memory database(re-connect)
    pthread_mutex_lock(&__sqlite3_mutex);
    if (sqlite3_open(__SQLITE3_DBNM, &handle->dbh) != SQLITE_OK)
    {
        pthread_mutex_unlock(&__sqlite3_mutex);
        printf("failed.sqlite3_open[%s]", sqlite3_errmsg(handle->dbh));
        return (NULL);
    }
    pthread_mutex_unlock(&__sqlite3_mutex);
    return (handle);
}

/**
 sqlite3 - generate statement instance\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   handle    database handle
 @return void* NULL!=OK:statement instance , NULL==error
*/
DBPROVIDER_STMT_HANDLE __sqlite3_stmt_init(DBPROVIDER_HANDLE handle)
{
    __SQLITE3_ST_STMT_PTR p = NULL;
    //
    if (!handle)
    {
        return (NULL);
    }
    pthread_mutex_lock(&__sqlite3_mutex);
    if (handle != __sqlite3_single_instance)
    {
        printf("invalid instance[%p/%p]", handle, __sqlite3_single_instance);
        pthread_mutex_unlock(&__sqlite3_mutex);
        return (NULL);
    }
    // must be connected
    if (handle->dbh == NULL)
    {
        printf("invalid connection status[%p/%p]", handle, __sqlite3_single_instance);
        pthread_mutex_unlock(&__sqlite3_mutex);
        return (NULL);
    }
    p = (__SQLITE3_ST_STMT_PTR)malloc(sizeof(__SQLITE3_ST_STMT));
    bzero(p, sizeof(*p));
    p->dbh = handle->dbh;
    pthread_mutex_unlock(&__sqlite3_mutex);
    //
    return (p);
}
/**
 sqlite3 - prepare statement\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   handle    statement handle
 @param[in]   query     query string
 @param[in]   lenght    <not implemented>  query string length
 @return int  0==OK,0!=error
*/
int __sqlite3_stmt_prepare(DBPROVIDER_STMT_HANDLE handle, const char *query, unsigned long length)
{
    if (!handle || !query)
    {
        return (ERR);
    }
    if (handle->sql)
    {
        free(handle->sql);
    }
    handle->sql = (char *)malloc(strlen(query) + 1);
    memcpy(handle->sql, query, strlen(query));
    handle->sql[strlen(query)] = '\0';
    //
    pthread_mutex_lock(&__sqlite3_mutex);
    // prepare meta-information
    if (sqlite3_prepare_v2(handle->dbh, handle->sql, -1, &handle->stmt, NULL) != SQLITE_OK)
    {
        pthread_mutex_unlock(&__sqlite3_mutex);
        return (ERR);
    }
    pthread_mutex_unlock(&__sqlite3_mutex);
    return (0);
}
/**
 sqlite3 - statement release\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   stmt      statement handle
 @return int  0==OK,0!=error
*/
int __sqlite3_stmt_close(DBPROVIDER_STMT_HANDLE stmt)
{
    if (stmt)
    {
        pthread_mutex_lock(&__sqlite3_mutex);
        if (stmt->stmt)
        {
            sqlite3_finalize(stmt->stmt);
        }
        if (stmt->sql)
        {
            free(stmt->sql);
        }
        free(stmt);
        pthread_mutex_unlock(&__sqlite3_mutex);
    }
    return (OK);
}

/**
 sqlite3 - get statement error string\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   stmt      statement handle
 @return const char*  error message,NULL available
*/
const char *__sqlite3_stmt_error(DBPROVIDER_STMT_HANDLE stmt)
{
    return ("unknown");
}

/**
 sqlite3 - reset statement\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   stmt      statement handle
 @return int 0==ok,0!=error
*/
int __sqlite3_stmt_reset(DBPROVIDER_STMT_HANDLE stmt)
{
    return (sqlite3_reset(stmt->stmt));
}

/**
 sqlite3 - bind statement parameter\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   stmt      statement handle
 @param[in]   bnd       bind handle
 @return int 0==ok,0!=error
*/
int __sqlite3_stmt_bind_param(DBPROVIDER_STMT_HANDLE stmt, DBPROVIDER_BIND *bnd)
{
    int n;
    if (!stmt || !bnd)
    {
        return (ERR);
    }
    //
    sqlite3_clear_bindings(stmt->stmt);
    //
    for (n = 0; bnd[n].buffer != NULL; n++)
    {
        switch (bnd[n].buffer_type)
        {
        case MYSQL_TYPE_LONGLONG:
            if (sqlite3_bind_int64(stmt->stmt, (n + 1), *((sqlite3_int64 *)((unsigned long long *)bnd[n].buffer))) != SQLITE_OK)
            {
                return (ERR);
            }
            break;
        }
    }
    return (OK);
}

/**
 sqlite3 - databaserelease\n
 *******************************************************************************
 release when reference count is 0.\n
 *******************************************************************************
 @param[in]   handle    database handle
*/
void __sqlite3_close(DBPROVIDER_HANDLE handle)
{

    pthread_mutex_lock(&__sqlite3_mutex);
    if (!__sqlite3_reference_count)
    {
        // cleanup resource
        if (handle->rec)
        {
            free(handle->rec);
        }
        handle->rcnt = 0;
        handle->rec = NULL;
        if (sqlite3_close(handle->dbh) != SQLITE_OK)
        {
            printf("sqlite3_close(%s)\n", sqlite3_errmsg(handle->dbh));
        }
        unlink(__SQLITE3_DBNM);
        __sqlite3_single_instance = NULL;
    }
    else
    {
        __sqlite3_reference_count--;
    }
    pthread_mutex_unlock(&__sqlite3_mutex);
}
/**
 sqlite3 - initialize bind regions\n
 *******************************************************************************
 <not implemented> \n
 *******************************************************************************
 @param[in]   bind   bind regions
 @return int  0==OK,0!=error
*/
int __sqlite3_bind_init(DBPROVIDER_STBIND bind)
{
    return (0);
}
/**
 sqlite3 - execute SQLquery, get records\n
 *******************************************************************************
 <not implemented> \n
 *******************************************************************************
 @param[in]   stmt     statement handle
 @param[in]   bind     bind region
 @param[in]   callback callback
 @param[in]   userdata user data
 @return int  0==OK,0!=error
*/
int __sqlite3_execute(DBPROVIDER_STMT_HANDLE stmt, DBPROVIDER_BIND *bind, on_event_mysql_execute callback, void *userdata)
{
    __sqlite3_container_t container;
    int n;
    const char *cn;
    record_t record[MYSQL_MAXBIND];
    //
    if (!stmt)
    {
        return (ERR);
    }
    if (!stmt->sql || !stmt->dbh)
    {
        return (ERR);
    }

    bzero(&container, sizeof(container));
    container.udata = userdata;
    container.callback = callback;
    container.dbh = stmt->dbh;
    container.stmt = stmt->stmt;
    container.clmncnt = sqlite3_column_count(container.stmt);
    for (n = 0; n < container.clmncnt; n++)
    {
        // all columns are retrieved as NULL when sqlite3_column_type is used
        cn = sqlite3_column_decltype(container.stmt, n);
        if (cn == NULL)
        {
            container.colmntype[n] = SQLITE_INTEGER;
        }
        else if (strncasecmp(cn, "TEXT", 4) == 0)
        {
            container.colmntype[n] = SQLITE_TEXT;
        }
        else
        {
            container.colmntype[n] = SQLITE_INTEGER;
        }
        if ((cn = sqlite3_column_name(container.stmt, n)) != NULL)
        {
            memcpy(container.colmn[n].name, cn, MIN(strlen(cn), sizeof(container.colmn[n].name) - 1));
        }
    }
    pthread_mutex_lock(&__sqlite3_mutex);

    while (sqlite3_step(stmt->stmt) == SQLITE_ROW)
    {
        // parse record
        bzero(record, sizeof(record));
        for (n = 0; n < container.clmncnt; n++)
        {
            if (container.colmntype[n] == SQLITE_TEXT)
            {
                record[n].flag = (1 << 31);
                if ((cn = (const char *)sqlite3_column_text(stmt->stmt, n)) != NULL)
                {
                    memcpy(record[n].u.sval, cn, MIN(strlen(cn), sizeof(record[n].u.sval) - 1));
                }
            }
            else if (container.colmntype[n] == SQLITE_INTEGER)
            {
                record[n].flag = 0;
                record[n].u.nval = sqlite3_column_int64(stmt->stmt, n);
            }
            else
            {
                record[n].flag = 0;
                record[n].u.nval = sqlite3_column_int64(stmt->stmt, n);
            }
        }
        //
        if (callback)
        {
            callback(container.counter, container.clmncnt, container.colmn, record, container.udata);
        }
        container.counter++;
    }
    pthread_mutex_unlock(&__sqlite3_mutex);
    return (0);
}

/**
 sqlite3 - save record\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   mysql     databaseinstance handle
 @return resource handle NULL==error , !=NULL , OK
*/
DBPROVIDER_RES_HANDLE __sqlite3_store_result(DBPROVIDER_HANDLE handle)
{
    __SQLITE3_ST_RES_PTR p;
    int n;

    if (!handle)
    {
        return (NULL);
    }
    if (!handle->rec || !handle->rcnt)
    {
        return (NULL);
    }
    if ((p = (__SQLITE3_ST_RES_PTR)malloc(sizeof(__SQLITE3_ST_RES))) == NULL)
    {
        return (NULL);
    }
    bzero(p, sizeof(*p));

    // duplicate auto saved resource of database instance.
    p->rcnt = handle->rcnt;

    // duplicate to char**
    if ((p->rec = (char **)malloc(sizeof(char **) * p->rcnt)) == NULL)
    {
        return (NULL);
    }
    for (n = 0; n < handle->rcnt; n++)
    {
        record_ptr rp = &handle->rec[n];
        p->rec[n] = (char *)malloc(MYSQL_MAXCOLLEN * 4);
        memcpy(p->rec[n], rp->u.sval, MYSQL_MAXCOLLEN * 4);
    }
    p->counter = handle->counter;
    p->clmncnt = handle->clmncnt;
    memcpy(p->colmn, handle->colmn, sizeof(handle->colmn));
    memcpy(p->colmntype, handle->colmntype, sizeof(handle->colmntype));
    //
    return (p);
}
/**
 sqlite3 - count of saved record\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   res      resource  handle
 @return unsigned long long unsigned long long(-1): error , record count
*/
unsigned long long __sqlite3_num_rows(DBPROVIDER_RES_HANDLE res)
{
    if (!res)
    {
        return (0LL);
    }
    if (!res->rec)
    {
        return (0LL);
    }
    //
    return ((unsigned long long)res->counter);
}
/**
 sqlite3 - access to saved record\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   res      resource  handle
 @return struct __sqlite3_st_row* = record , NULL = EOF
*/
DBPROVIDER_ROW_HANDLE __sqlite3_fetch_row(DBPROVIDER_RES_HANDLE res)
{
    U32 curpos;
    if (!res)
    {
        return (NULL);
    }
    if (!res->rec)
    {
        return (NULL);
    }

    // EOR(end of record)
    if (res->curpos >= res->rcnt)
    {
        return (NULL);
    }
    curpos = res->curpos;
    res->curpos += res->clmncnt;
    //
    return (&res->rec[curpos]);
}

/**
 sqlite3 - recordrelease\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   res      resource  handle
*/
void __sqlite3_free_result(DBPROVIDER_RES_HANDLE res)
{
    int n;
    if (!res)
    {
        if (!res->rec)
        {
            for (n = 0; n < res->rcnt; n++)
            {
                free(res->rec[n]);
            }
        }
        res->rec = NULL;
        free(res);
    }
}

/**
 sqlite3 - latest error\n
 *******************************************************************************
 <not implemented> \n
 *******************************************************************************
 @param[in]   handle   instance handle
 @return char* !=NULL: message , ==NULL: OK
*/
const char *__sqlite3_error(DBPROVIDER_HANDLE handle)
{
    return ("unknown exception");
}
/**
 sqlite3 - latest error code\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   handle      database handle
 @return unsigned int error code
*/
unsigned int __sqlite3_errno(DBPROVIDER_HANDLE handle)
{
    return (0);
}

/**
 sqlite3 - internal usecallback\n
 *******************************************************************************
 auto data store\n
 *******************************************************************************
 @param[in]   udata    user data
 @param[in]   cnt      callback counter
 @param[in]   vals     value
 @param[in]   columns  columns
 @return int  0=OK/0!=ERR
*/
int __sqlite3_callback_store(void *udata, int cnt, char **vals, char **columns)
{
    __SQLITE3_ST_PTR p = (__SQLITE3_ST_PTR)udata;
    int n, m, reccnt;

    // save name of columns in first callback
    if (p->counter == 0)
    {
        for (n = 0; n < p->clmncnt; n++)
        {
            memcpy(p->colmn[n].name, columns[n], MIN(sizeof(p->colmn[n].name) - 1, strlen(columns[n])));
        }
    }
    // extend data region
    if ((p->rec = realloc(p->rec, sizeof(record_t) * (p->rcnt + p->clmncnt))) == NULL)
    {
        return (ERR);
    }
    reccnt = p->rcnt;
    for (n = reccnt, m = 0; n < (reccnt + p->clmncnt); n++, m++)
    {
        p->rec[n].flag = (1 << 31);
        if (vals[m])
        {
            memcpy(p->rec[n].u.sval, vals[m], MIN(sizeof(p->rec[n].u.sval) - 1, strlen(vals[m])));
        }
        else
        {
            bzero(p->rec[n].u.sval, sizeof(p->rec[n].u.sval));
        }
        p->rcnt++;
    }
    // save data
    p->counter++;
    //
    return (OK);
}
