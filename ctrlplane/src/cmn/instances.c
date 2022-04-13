/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       instances.c
    @brief      instance
*******************************************************************************
*******************************************************************************
    @date       created(17/apr/2018)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 17/apr/2018 
      -# Initial Version
******************************************************************************/

#include "pgw_ext.h"

// instanciate
pthread_mutex_t __mysql_mutex;
int PGW_LOG_LEVEL = PGW_LOG_LEVEL_ALL;
int PGW_RECOVERY_COUNT = 0;