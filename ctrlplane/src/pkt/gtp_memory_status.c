/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       gtpc_free_packet.c
    @brief      gtpcpacket  release
*******************************************************************************
*******************************************************************************
    @date       created(12/nov/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 12/nov/2017 
      -# Initial Version
******************************************************************************/

#include "gtpc_ext.h"
#include "pgw_ext.h"

U64  gtp_memory_status_count = 0;
U64  gtp_memory_alloc_count = 0;
U64  gtp_memory_free_count = 0;
U64  gtp_memory_free_err_count = 0;

U64  pgw_memory_status_count = 0;
U64  pgw_memory_alloc_count = 0;
U64  pgw_memory_free_count = 0;
U64  pgw_memory_free_err_count = 0;

pthread_mutex_t  pgw_memory_alloc_count_mtx;
pthread_mutex_t  pgw_memory_free_count_mtx;




/**
 gtp packet : memory status \n
 *******************************************************************************
 get memory status \n
 *******************************************************************************
 @return U64  0==valid,0!=count of invalid
*/
U64   gtpc_memory_status(void){
    return(gtp_memory_status_count);
}

/**
 gtp packet : print out memory status \n
 *******************************************************************************
 \n
 *******************************************************************************
 @return RETCD  0==success, 0!=error
*/
RETCD gtpc_memory_print(void){
    PGW_LOG(PGW_LOG_LEVEL_MIN, "PGW(buffer over run): %llu\n", pgw_memory_status_count);
    PGW_LOG(PGW_LOG_LEVEL_MIN, "PGW(allocate count ): %llu\n", pgw_memory_alloc_count);
    PGW_LOG(PGW_LOG_LEVEL_MIN, "PGW(free count     ): %llu\n", pgw_memory_free_count);
    PGW_LOG(PGW_LOG_LEVEL_MIN, "PGW(free err count ): %llu\n", pgw_memory_free_err_count);


    PGW_LOG(PGW_LOG_LEVEL_MIN, "GTP(buffer over run): %llu\n", gtp_memory_status_count);
    PGW_LOG(PGW_LOG_LEVEL_MIN, "GTP(allocate count ): %llu\n", gtp_memory_alloc_count);
    PGW_LOG(PGW_LOG_LEVEL_MIN, "GTP(free count     ): %llu\n", gtp_memory_free_count);
    PGW_LOG(PGW_LOG_LEVEL_MIN, "GTP(free err count ): %llu\n", gtp_memory_free_err_count);
    //
    return(
        ((gtp_memory_status_count + (gtp_memory_alloc_count==gtp_memory_free_count?0:1)) +
         (pgw_memory_status_count + (pgw_memory_alloc_count==pgw_memory_free_count?0:1))) ==0?OK:ERR);
}

