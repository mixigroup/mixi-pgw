/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       alloc_sgwpeers.c
    @brief      sgw peers
*******************************************************************************
*******************************************************************************
    @date       created(08/may/2018)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 08/may/2018 
      -# Initial Version
******************************************************************************/

#include "pgw_ext.h"

#ifdef __USE_SQLITE3__
#include <sqlite3.h>

#define SQLITE3_DBNM    (":memory:")
struct _sqlite3_container{
    void*   udata;
    pgw_sgw_peer_callback callback;
    int     exists;
};

enum _SGW_PEER{
    _SGW_PEER_IP = 0,
    _SGW_PEER_CNT,
    _SGW_PEER_EXPIRE,
    _SGW_PEER_MAX
};
static int _sqlite3_callback(void *, int, char **, char **);
#endif

static const U8 _magic[4] = {0xde,0xad,0xbe,0xaf};

/**
 allocate sgw peer \n
 *******************************************************************************
 initialize sgw peers\n
 *******************************************************************************
 @param[in]     type    not supported : reserved
 @param[out]    pppeer  sgw peersinstance address
 @return RETCD  0==success,0!=error
*/
RETCD pgw_create_sgw_peers(INT type, sgw_peers_ptr *pppeer){
#ifdef __USE_SQLITE3__
    const char *sql = "CREATE TABLE sgw_peers(`ip` INT, `counter` INT, `expire` INT); CREATE INDEX ip ON sgw_peers(ip);";
    char* errmsg = NULL;
#endif
    // set magic code on last 4 octet
    (*pppeer) = (sgw_peers_ptr)malloc(sizeof(sgw_peers_t) + sizeof(_magic));
    if ((*pppeer) == NULL){
        return(ERR);
    }
    bzero((*pppeer), sizeof(sgw_peers_t) + sizeof(_magic));
    (*pppeer)->updated  = (U32)time(NULL);
    memcpy((((U8*)(*pppeer)) + sizeof(sgw_peers_t)), _magic, sizeof(_magic));
#ifndef __USE_SQLITE3__
    TAILQ_INIT(&(*pppeer)->peers);
#else
    // on memory database
    if (sqlite3_open(SQLITE3_DBNM, &((*pppeer)->peers)) != SQLITE_OK){
        pgw_panic("sqlite3_open(%s)", sqlite3_errmsg((*pppeer)->peers));
    }
    // create tagble
    if (sqlite3_exec((*pppeer)->peers, sql, NULL, NULL, &errmsg) != SQLITE_OK){
        pgw_panic("sqlite3_exec(%s)", sqlite3_errmsg((*pppeer)->peers));
    }
    if (errmsg){
        sqlite3_free(errmsg);
    }
#endif
    return(OK);
}

/**
 deallocate sgw peers \n
 *******************************************************************************
 cleanup sgw peers\n
 *******************************************************************************
 @param[in/out]   pppeer  sgw peersinstance address
 @return RETCD    0==success,0!=error
*/
RETCD pgw_free_sgw_peers(sgw_peers_ptr *pppeer){
#ifndef __USE_SQLITE3__
    sgw_peer_ptr p1,p2;
#endif
    if ((*pppeer) == NULL){
        return(ERR);
    }
    if (memcmp(((U8*)(*pppeer)) + sizeof(sgw_peers_t), _magic, sizeof(_magic)) != 0){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "memory status(%p)", (*pppeer));
        return(ERR);
    }
#ifndef __USE_SQLITE3__
    p1 = TAILQ_FIRST(&(*pppeer)->peers);
    while(p1 != NULL){
        p2 = TAILQ_NEXT(p1, link);
        free(p1);
        p1 = p2;
    }
#else
    if (sqlite3_close(((*pppeer)->peers)) != SQLITE_OK){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "sqlite3_close(%s)", sqlite3_errmsg((*pppeer)->peers));
    }
    unlink(SQLITE3_DBNM);
#endif

    free((*pppeer));
    (*pppeer) = NULL;
    return(OK);
}

/**
 sgw peers : peer SET \n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]     peers   sgw peersinstance
 @param[in]     ip      ip address
 @param[in]     counter counter default= 0
 @param[in]     expire  expire epoch default = time(NULL) + x
 @return RETCD  0==success,0!=error
*/
RETCD pgw_set_sgw_peer(sgw_peers_ptr peers, U32 ip, U32 counter, U32 expire){
#ifdef __USE_SQLITE3__
    char  sql[256] = {0};
    char* errmsg = NULL;
#else
    sgw_peer_ptr    pi;
    sgw_peer_ptr    p1,p2;
    int exists = 0;
#endif
    if (!peers || !ip){
        return(ERR);
    }
#ifndef __USE_SQLITE3__
    p1 = TAILQ_FIRST(&peers->peers);
    while(p1 != NULL){
        p2 = TAILQ_NEXT(p1, link);
        if (p1->ip == ip){
            p1->counter = counter;
            p1->expire = expire;

            exists = 1;
            break;
        }
        p1 = p2;
    }
    if (!exists){
        // empty -> add
        if (!(pi = (sgw_peer_ptr)malloc(sizeof(sgw_peer_t)))){
            return(ERR);
        }
        bzero(pi, sizeof(*pi));
        //
        pi->ip      = ip;
        pi->counter = counter;
        pi->expire  = expire;

        // add list
        TAILQ_INSERT_TAIL(&peers->peers, pi, link);
        // increment count of list
        peers->count ++;
    }
#else
    snprintf(sql, sizeof(sql)-1,"INSERT INTO sgw_peers(`ip`,`counter`,`expire`) VALUES(%u,%u,%u);", ip, counter, expire);
    // add to database
    if (sqlite3_exec(peers->peers, sql, NULL, NULL, &errmsg) != SQLITE_OK){
        pgw_panic("sqlite3_exec(%s)", sqlite3_errmsg(peers->peers));
    }
    if (errmsg){
        sqlite3_free(errmsg);
    }
#endif

    return(OK);
}

/**
 sgw peers : peer GET \n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]     peers    sgw peersinstance
 @param[in]     ip       target ip address, -1=all
 @param[in]     callback find callback
 @param[in]     udata    callback , user data
 @return RETCD  0==success,0!=error
*/
RETCD pgw_get_sgw_peer(sgw_peers_ptr peers, U32 ip, pgw_sgw_peer_callback callback, void* udata){
    int exists = 0;
#ifdef __USE_SQLITE3__
    char  sql[256] = {0};
    char* errmsg = NULL;
    struct _sqlite3_container container;
#endif
    if (!callback || !peers){
        return(ERR);
    }
#ifndef __USE_SQLITE3__
    sgw_peer_ptr p1,p2;
    p1 = TAILQ_FIRST(&peers->peers);
    while(p1 != NULL){
        p2 = TAILQ_NEXT(p1, link);
        if (ip == ((U32)-1)){
            callback(udata, p1);
            exists = 1;
        }else{
            if (p1->ip == ip){
                callback(udata, p1);
                exists = 1;
                break;
            }
        }
        p1 = p2;
    }
#else
    if (ip == ((U32)-1)){
        snprintf(sql, sizeof(sql)-1,"SELECT ip, counter, expire FROM sgw_peers;");
    }else{
        snprintf(sql, sizeof(sql)-1,"SELECT ip, counter, expire FROM sgw_peers WHERE ip = %u;", ip);
    }
    bzero(&container, sizeof(container));
    container.udata = udata;
    container.callback = callback;
    //
    if (sqlite3_exec(peers->peers, sql, _sqlite3_callback, &container, &errmsg) != SQLITE_OK){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "sqlite3_exec(%s)", sqlite3_errmsg(peers->peers));
    }
    if (errmsg){
        sqlite3_free(errmsg);
    }
    exists = container.exists;
#endif


    return(exists?OK:ERR);
}
#ifdef __USE_SQLITE3__
static int _sqlite3_callback(void * udata, int cnt, char ** vals, char ** columns){
    struct _sqlite3_container *container = (struct _sqlite3_container*)udata;
    sgw_peer_t peer;

    if (cnt != _SGW_PEER_MAX || !container){
        return(-1);
    }
    if (!container->callback){
        return(-1);
    }
    container->exists ++;
    //
    peer.ip = (U32)atoi(vals[_SGW_PEER_IP]);
    peer.counter = (U32)atoi(vals[_SGW_PEER_CNT]);
    peer.expire = (U32)atoi(vals[_SGW_PEER_EXPIRE]);
    //
    return(container->callback(container->udata, &peer));
}
#endif

/**
 sgw peers : delete peer\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]     peers    sgw peersinstance
 @param[in]     ip       target ip address
 @return RETCD  0==success,0!=error
*/
RETCD pgw_delete_sgw_peer(sgw_peers_ptr peers, U32 ip){
#ifdef __USE_SQLITE3__
    char  sql[256] = {0};
    char* errmsg = NULL;
#else
    sgw_peer_ptr p1,p2;
#endif
    if (!peers){
        return(ERR);
    }
#ifndef __USE_SQLITE3__

    // delete one item
    p1 = TAILQ_FIRST(&peers->peers);
    while(p1 != NULL){
        p2 = TAILQ_NEXT(p1, link);
        if (p1->ip == ip) {
            /* Remove the item from the tail queue. */
            TAILQ_REMOVE(&peers->peers, p1, link);
            free(p1);
            break;
        }
        p1 = p2;
    }
#else
    snprintf(sql, sizeof(sql)-1,"DELETE FROM sgw_peers WHERE ip = %u;", ip);
    // delete from database
    if (sqlite3_exec(peers->peers, sql, NULL, NULL, &errmsg) != SQLITE_OK){
        pgw_panic("sqlite3_exec(%s)", sqlite3_errmsg(peers->peers));
    }
    if (errmsg){
        sqlite3_free(errmsg);
    }
#endif
    return(OK);
}

