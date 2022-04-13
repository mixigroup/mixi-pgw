/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       create_instance.c
    @brief      generate instance
*******************************************************************************
*******************************************************************************
    @date       created(07/nov/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 07/nov/2017
      -# Initial Version
******************************************************************************/

#include "pgw_ext.h"
#include <mysql.h>

extern pthread_mutex_t  pgw_memory_alloc_count_mtx;
extern pthread_mutex_t  pgw_memory_free_count_mtx;


/**
 instanciate \n
 *******************************************************************************
 initialize client library\n
 *******************************************************************************
 @param[in]     type    not supported : reserved
 @param[out]    ppinst  instance address
 @return RETCD  0==success,0!=error
*/
RETCD pgw_create_instance(INT type, handle_ptr *ppinst){
    static INT singleton = 0;

    if (singleton == 0){
        evthread_use_pthreads();
        mysql_library_init(0,0,0);
        singleton ++;
    }
    (*ppinst) = (handle_ptr)malloc(sizeof(handle_t) + (sizeof(server_t) * 2));
    if (!(*ppinst)){
        return(ERR);
    }
    memset((*ppinst), 0, sizeof(handle_t) + sizeof(server_t) * 2);
    (*ppinst)->server_gtpc = (server_ptr)(&(*ppinst)[1]);
    (*ppinst)->server_gtpu = &((*ppinst)->server_gtpc)[1];
    (*ppinst)->type = SERVER;
    TAILQ_INIT(&(*ppinst)->properties);
    (*ppinst)->server_gtpc->sock = -1;
    (*ppinst)->server_gtpu->sock = -1;
    (*ppinst)->server_gtpc->halt = 0;
    (*ppinst)->server_gtpu->halt = 0;
    (*ppinst)->server_gtpc->handle  = (*ppinst);
    (*ppinst)->server_gtpu->handle  = (*ppinst);
    //
    pthread_mutex_init(&pgw_memory_alloc_count_mtx, NULL);
    pthread_mutex_init(&pgw_memory_free_count_mtx, NULL);

    return(OK);
}
