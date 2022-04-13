/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       mysql_execute.c
    @brief      database : operation
*******************************************************************************
*******************************************************************************
    @date       created(14/nov/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 14/nov/2017 
      -# Initial Version
******************************************************************************/

#include "db_ext.h"


/**
 mysql initialize bind regions\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]   bind      mysql bind
 @return int  0==OK,0!=error
*/
RETCD pgw_mysql_bind_init(dbbind_ptr bind){
    int n;
    // initialize bind regions
    for(n = 0;n < MYSQL_MAXBIND;n++){
        bind->bind[n].error = &bind->errn[n];
        bind->bind[n].length = &bind->length[n];
        bind->bind[n].is_null = &bind->null[n];
        bind->bind[n].buffer = &bind->data[n];
    }
    return (OK);
}
