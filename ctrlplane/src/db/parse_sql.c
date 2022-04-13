/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       parse_sql.c
    @brief      SQLoperation
*******************************************************************************
*******************************************************************************
    @date       created(31/aug/2018)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2018 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 31/aug/2018 
      -# Initial Version
******************************************************************************/

#include "db_ext.h"


/**
 sql parser\n
 *******************************************************************************
 \n
 *******************************************************************************
 @param[in]     sql    SQL string
 @param[in/out] parsed parsed pointer array
 @param[in]     count  buffer length
 @return int    0==OK,0!=error
*/
RETCD pgw_parse_sql(const char* sql, sqlparse_ptr parsed, unsigned count) {
    char *p = NULL;
    char *src = NULL;
    int cnt = 0;
    //
    if (!parsed || !count){ return(ERR); }
    if (strlen(sql)==0) { return(ERR); }
    if ((src = (char*)malloc(strlen(sql)+1)) == NULL){ return(ERR); }
    memcpy(src, sql, strlen(sql));
    src[strlen(sql)] = '\0';

    //
    if ((p = strtok(src, " ")) == NULL){
        free(src);
        return(ERR);
    }
    memcpy(parsed[0].q, p, MIN(SQLPARSE_ILEN -1, strlen(p)));
    parsed[0].len = strlen(p);
    do{
        if ((p = strtok(NULL," ")) == NULL){
            break;
        }
        cnt++;
        if ((cnt+1) >= count){ break; }
        memcpy(parsed[cnt].q, p, MIN(SQLPARSE_ILEN -1, strlen(p)));
        parsed[cnt].len = strlen(p);
    }while(p!=NULL);
    //
    free(src);
    return((cnt+1));
}

