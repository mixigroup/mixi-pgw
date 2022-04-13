/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       set_property.c
    @brief      set dynamic property in PGW system
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


/**
 set property.\n
 *******************************************************************************
 set dynamic property in PGW system\n
 *******************************************************************************
 @param[in]     pinst instance
 @param[in]     id    property type
 @param[in]     value first address
 @param[in]     length address data length
 @return RETCD  0==success,0!=error
*/
RETCD pgw_set_property(handle_ptr pinst, INT id, PTR value, INT length){
    property_ptr pfind = NULL;
    if (!pinst || !value || !length){
        return(ERR);
    }
    pthread_mutex_lock(&pinst->property_mtx);
    if (!TAILQ_EMPTY(&pinst->properties)){
        // when update, release previous item
        TAILQ_FOREACH(pfind, &pinst->properties, link) {
            if (pfind->id == id) {
                TAILQ_REMOVE(&pinst->properties, pfind, link);
                free(pfind->data);
                free(pfind);
                break;
            }
        }
    }
    pthread_mutex_unlock(&pinst->property_mtx);
    //
    pfind = (property_ptr)malloc(sizeof(property_t));
    if (!pfind){
        PGW_LOG(PGW_LOG_LEVEL_ERR, "not enough memory.(pgw_set_property:%s)\n", strerror(errno));
        return(ERR);
    }
    pfind->id = id;
    pfind->length = length;
    pfind->data = (PTR)malloc(length + 1);
    if (!pfind->data){
        free(pfind);
        PGW_LOG(PGW_LOG_LEVEL_ERR, "not enough memory.(pgw_set_property - internal:%s)\n", strerror(errno));
        return(ERR);
    }
    bzero(pfind->data, length + 1);
    memcpy(pfind->data, value, length);
    //
    pthread_mutex_lock(&pinst->property_mtx);
    TAILQ_INSERT_TAIL(&(pinst->properties), pfind, link);
    pthread_mutex_unlock(&pinst->property_mtx);

    return(OK);
}
