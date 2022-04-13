/******************************************************************************/
/*! @addtogroup mixi_pgw_ctrl_plane
    @file       create_instance.c
    @brief      instanciate
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
 deallocate instanciate \n
 *******************************************************************************
 cleanup client library\n
 *******************************************************************************
 @param[out]    ppinst  instance address
 @return RETCD  0==success,0!=error
*/
RETCD pgw_release_instance(handle_ptr *ppinst){

    if ((*ppinst)->server_gtpc){
        close((*ppinst)->server_gtpc->sock);
        (*ppinst)->server_gtpc->sock = -1;
    }
    if ((*ppinst)->server_gtpu){
        close((*ppinst)->server_gtpu->sock);
        (*ppinst)->server_gtpu->sock = -1;
    }


    // propertyrelease
    {   property_ptr p1 = TAILQ_FIRST(&(*ppinst)->properties);
        while(p1 != NULL){
            property_ptr p2 = TAILQ_NEXT(p1, link);
            free(p1);
            p1 = p2;
        }
    }

    free((*ppinst));
    (*ppinst) = NULL;

   return(OK);
}
