/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       thread.c
    @brief      node = 1 thread
*******************************************************************************
*******************************************************************************
    @date       created(09/nov/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 09/nov/2017 
      -# Initial Version
******************************************************************************/

#include "node_ext.h"
#include "pgw_ext.h"
#include "gtpc_ext.h"
#include "db_ext.h"
#include <mysql.h>
/**
 node thread.\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]     arg     thread args
 @return void* not supported
*/

static pthread_mutex_t __mysql_connect_mutex;


void* node_thread(void* arg){
    node_ptr    pnode = (node_ptr)arg;
    char        reconnect = 1;
    DBPROVIDER_HANDLE      dbh = NULL;
    PGW_LOG(PGW_LOG_LEVEL_INF, " >> start(%p : %p: %u)\n", (void*)pthread_self(), (void*)pnode, pnode->index);

    PTR host, user, pswd, inst, port, flag;
    RETCD ret = OK;
    // get properties about database connection
    ret = pgw_get_property(pnode->handle, DB_HOST, &host, NULL)
        | pgw_get_property(pnode->handle, DB_USER, &user, NULL)
        | pgw_get_property(pnode->handle, DB_PSWD, &pswd, NULL)
        | pgw_get_property(pnode->handle, DB_INST, &inst, NULL)
        | pgw_get_property(pnode->handle, DB_PORT, &port, NULL)
        | pgw_get_property(pnode->handle, DB_FLAG, &flag, NULL);

    if (ret != OK){
        pgw_panic("invalid parameters (%p:%d)\n", (void*)pthread_self(), ret);
    }

    pthread_mutex_lock(&__mysql_connect_mutex);

    // initialize connection every node thread
    DBPROVIDER_THREAD_INIT();
    if ((dbh = DBPROVIDER_INIT(NULL)) == NULL){
        pgw_panic("failed. mysql_init (%p)\n", (void*)pthread_self());
    }
    DBPROVIDER_OPTIONS(dbh, MYSQL_OPT_RECONNECT, &reconnect);
    if (!DBPROVIDER_REAL_CONNECT(dbh,
                           (TXT)host,
                           (TXT)user,
                           (TXT)pswd,
                           (TXT)inst,
                           (*(U32*)port),
                           NULL,
                           (*(U32*)flag))){
        pgw_panic("failed. mysql_real_connect (%p : %s/%s/%s/%s/%u/%u)\n",
        (void*)pthread_self(),
                  (TXT)host,
                  (TXT)user,
                  (TXT)pswd,
                  (TXT)inst,
                  (*(U32*)port),
                  (*(U32*)flag));
    }
    DBPROVIDER_SET_CHAR(dbh, "utf8");
    pnode->dbhandle = dbh;
    //
    if (pnode->handle->callback){
        ((pgw_callback)pnode->handle->callback)(EVENT_POST_INIT, pnode->handle, 0, 0, NULL, 0, NULL, 0, NULL, 0, pnode);
    }
    pthread_mutex_unlock(&__mysql_connect_mutex);
    //
    while(!pnode->halt) {
        if (pnode->handle->callback){
            if (((pgw_callback)pnode->handle->callback)(EVENT_ENTRY, pnode->handle, 0, 0, NULL, 0, NULL, 0, NULL, 0, pnode) != OK){
                break;
            }
        }
    }
    if (pnode->handle->callback){
        ((pgw_callback)pnode->handle->callback)(EVENT_UNINIT, pnode->handle, 0, 0, NULL, 0, NULL, 0, NULL, 0, pnode);
    }
    // cleanup database connection
    DBPROVIDER_CLOSE(dbh);
    PGW_LOG(PGW_LOG_LEVEL_INF, "<< finish(%p)\n", (void*)pthread_self());
    return(NULL);
}
