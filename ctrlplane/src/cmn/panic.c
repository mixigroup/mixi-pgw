/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       panic.c
    @brief      logging
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

#include "pgw_ext.h"

/**
 panic. abort \n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]     func    function name
 @parma[in]     line    number of line
 @param[in]     fmt     format
*/
void __pgw_panic(const char *func, const int line, const char *fmt, ...){
    char msg_bf[512] = {0};
    va_list ap;
    fprintf(stderr, "PANIC in %s [%d]\n", func, line);
    va_start(ap, fmt);
    vsnprintf(msg_bf, sizeof(msg_bf)-1,fmt, ap);

    vprintf(fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    syslog(LOG_ERR, ">> %20s<%s", func, msg_bf);
    abort();
}