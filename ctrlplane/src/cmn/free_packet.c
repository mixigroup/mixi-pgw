/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       free_packet.c
    @brief      release packet
*******************************************************************************
*******************************************************************************
    @date       created(08/nov/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 08/nov/2017
      -# Initial Version
******************************************************************************/

#include "pgw_ext.h"

extern U64  pgw_memory_free_count;
extern U64  pgw_memory_free_err_count;
extern pthread_mutex_t  pgw_memory_free_count_mtx;

/**
 free packet.\n
 *******************************************************************************
 1 packet release\n
 *******************************************************************************
 @param[in]     packet  packet 
 @return RETCD  0==success,0!=error
*/
RETCD pgw_free_packet(packet_ptr packet){
    if (packet){
        pthread_mutex_lock(&pgw_memory_free_count_mtx);
        pgw_memory_free_count++;
        pthread_mutex_unlock(&pgw_memory_free_count_mtx);
        //
        packet->data = NULL;
        packet->datalen = 0;
        free(packet);
    }else{
        pthread_mutex_lock(&pgw_memory_free_count_mtx);
        pgw_memory_free_err_count++;
        pthread_mutex_unlock(&pgw_memory_free_count_mtx);
        return(ERR);
    }
    return(OK);
}
