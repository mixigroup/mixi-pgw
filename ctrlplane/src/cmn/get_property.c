/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       get_property.c
    @brief      get dynamic property of PGW system
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
 get property.\n
 *******************************************************************************
 get dynamic property of PGW system\n
 *******************************************************************************
 @param[in]     pinst instance
 @param[in]     id    property type
 @param[out]    value  data first address
 @param[out]    length data length
 @return RETCD  0==success,0!=error
*/
RETCD pgw_get_property(handle_ptr pinst, INT id, PTR* value, INT* length){
    property_ptr it,next;
    pthread_mutex_lock(&pinst->property_mtx);
    // loop property list
    if (!TAILQ_EMPTY(&pinst->properties)){
        TAILQ_FOREACH_SAFE(it, &pinst->properties, link, next) {
            if (it->id == id) {
                (*value) = it->data;
                if (length){
                    (*length) = it->length;
                }
                pthread_mutex_unlock(&pinst->property_mtx);
                return(OK);
            }
        }
    }
    pthread_mutex_unlock(&pinst->property_mtx);
    // not found.
    return(ERR);
}