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
    2017 mixi. Released Under the MIT license
*******************************************************************************
    @par        History
    - 14/nov/2017 
      -# Initial Version
******************************************************************************/

#include "db_ext.h"
#include "pgw_ext.h"

static enum enum_field_types mysql_conv_type(enum enum_field_types e);

/**
 mysql execute SQL + record\n
 *******************************************************************************
 \n
 *******************************************************************************
 @parma[in]   stmt      mysql statement 
 @param[in]   bind      mysql bind
 @parma[in]   callback  every record : callback
 @param[in]   userdata  callback : user data
 @return int  0==OK,0!=error
*/
RETCD pgw_mysql_execute(MYSQL_STMT* stmt, MYSQL_BIND* bind, on_event_mysql_execute callback, void* userdata) {
    MYSQL_FIELD*fields = NULL;
    MYSQL_RES*  stmt_meta = NULL;
    size_t      clmncnt = 0,n;
    U32         counter = 0;
    RETCD       ret = ERR;
    char        clmnval[MYSQL_MAXCOLLEN*4]; // ex, utf8mb4
    record_t    record[MYSQL_MAXBIND];
    colmn_t     colmn[MYSQL_MAXBIND];
    //
    while (1) {
        if (mysql_stmt_execute(stmt) != 0) { break; }
        if ((stmt_meta  = mysql_stmt_result_metadata(stmt)) == NULL) { break; }
        if ((clmncnt    = mysql_num_fields(stmt_meta)) == 0) { break; }
        if ((fields     = mysql_fetch_fields(stmt_meta)) == NULL) { break; }
        // bind all columns.
        for (n = 0; n < clmncnt; n++) {
            bind[n].buffer_length = fields[n].length;
            bind[n].buffer_type = fields[n].type;
            bind[n].is_null = 0;
        }
        // bind result buffer.
        if (mysql_stmt_bind_result(stmt, bind) != 0) { break; }
        if (mysql_stmt_store_result(stmt) != 0) { break; }

        // column data
        bzero(colmn,  sizeof(colmn));
        for (n = 0; n < clmncnt; n++) {
            memcpy(colmn[n].name, fields[n].name, fields[n].name_length);
            colmn[n].name[fields[n].name_length] = 0;
        }

        // fetch all records.
        while (mysql_stmt_fetch(stmt) == 0) {
            bzero(record, sizeof(record));
            // access column value
            for (n = 0; n < clmncnt; n++) {
                enum enum_field_types et = mysql_conv_type(bind[n].buffer_type);
                if (et == MYSQL_TYPE_STRING) {
                    record[n].flag = (1<<31);
                    memcpy(record[n].u.sval, bind[n].buffer, bind[n].buffer_length);
                    record[n].u.sval[bind[n].buffer_length] = 0;
                } else if (et == MYSQL_TYPE_DATE) {
                    record[n].flag = (1<<31);
                    snprintf(record[n].u.sval, sizeof(record[n].u.sval) - 1, "%04d/%02d/%02d %02d:%02d:%02d",
                             ((MYSQL_TIME *) bind[n].buffer)->year,
                             ((MYSQL_TIME *) bind[n].buffer)->month,
                             ((MYSQL_TIME *) bind[n].buffer)->day,
                             ((MYSQL_TIME *) bind[n].buffer)->hour,
                             ((MYSQL_TIME *) bind[n].buffer)->minute,
                             ((MYSQL_TIME *) bind[n].buffer)->second
                    );

                } else if (et == MYSQL_TYPE_NEWDECIMAL){
                    record[n].flag = 0;
                    memcpy(clmnval, bind[n].buffer, bind[n].buffer_length);
                    clmnval[bind[n].buffer_length] = 0;
                    record[n].u.nval = strtoull(clmnval, NULL, 10);
                } else {
                    record[n].flag = 0;
                    record[n].u.nval = (uint64_t)*((long long *) bind[n].buffer);
                }
            }
            // 1 record
            if (callback){
                callback(counter, clmncnt, colmn, record, userdata);
                // ------------------- debug print ------------------
                if (PGW_LOG_LEVEL >= PGW_LOG_LEVEL_DBG){
                    // column information
                    if (counter == 0){
                        printf("--------\n");
                        for(n = 0;n < clmncnt;n++){
                            printf("%s%s", n==0?"":"\t", colmn[n].name);
                        }
                        printf("\n");
                        printf("--------\n");
                    }
                    // record
                    for(n = 0;n < clmncnt;n++){
                        if (record[n].flag == 0){
                            printf("%s%llu", n==0?"":"\t", record[n].u.nval);
                        }else{
                            printf("%s%s", n==0?"":"\t", record[n].u.sval);
                        }
                    }
                    printf("\n");
                }
            }
            counter++;
        }
        ret = OK;
        break;
    }
    if (stmt_meta != NULL) {
        mysql_free_result(stmt_meta);
    }
    if (stmt != NULL){
        mysql_stmt_free_result(stmt);
    }
    //
    return (ret);
}
/**
 mysql round field type\n
 *******************************************************************************
 \n
 *******************************************************************************
 @parma[in]   e       field type
 @return enum_field_types  rounded field type
*/
enum enum_field_types mysql_conv_type(enum enum_field_types e){
    switch (e) {
        case MYSQL_TYPE_DECIMAL:
        case MYSQL_TYPE_TINY:
        case MYSQL_TYPE_SHORT:
        case MYSQL_TYPE_LONG:
        case MYSQL_TYPE_FLOAT:
        case MYSQL_TYPE_DOUBLE:
        case MYSQL_TYPE_LONGLONG:
        case MYSQL_TYPE_INT24:
        case MYSQL_TYPE_ENUM:
            return (MYSQL_TYPE_LONGLONG);
        case MYSQL_TYPE_DATE:
        case MYSQL_TYPE_TIME:
#ifdef MYSQL_TYPE_TIME2
        case MYSQL_TYPE_TIME2:
        case MYSQL_TYPE_DATETIME2:
#endif
        case MYSQL_TYPE_DATETIME:
        case MYSQL_TYPE_YEAR:
        case MYSQL_TYPE_NEWDATE:
        case MYSQL_TYPE_TIMESTAMP:
#ifdef MYSQL_TYPE_TIME2
        case MYSQL_TYPE_TIMESTAMP2:
#endif
            return (MYSQL_TYPE_DATE);
        case MYSQL_TYPE_NEWDECIMAL:
            return (MYSQL_TYPE_NEWDECIMAL);
        default:
            return (MYSQL_TYPE_STRING);
    }
}

