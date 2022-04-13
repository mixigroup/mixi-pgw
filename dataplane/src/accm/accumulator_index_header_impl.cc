/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       accumulator_index_header_impl.cc
    @brief      implemented index header
*******************************************************************************
   \n
   \n
   \n
   \n
*******************************************************************************
    @date       created(11/oct/2017)

    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 11/oct/2017 
      -# Initial Version
******************************************************************************/
#include "mixi_pgw_data_plane_def.hpp"
#include "accumulator.hpp"
#include "accumulator_internal.hpp"

using namespace MIXIPGW;

/**
   index header implemented :  constructor \n
   *******************************************************************************
   constructor\n
   *******************************************************************************
   @param[in]   t   accumulator type(Server or Client)
   @param[in]   dir base directory
 */
MIXIPGW::AccumulatorIndexHeaderImpl::AccumulatorIndexHeaderImpl(ACCM t, DIR dir):
        type_(t),addr_(NULL),len_(0){
    dir_ = (dir?dir:"");
}

/**
   index implemented : destructor\n
   *******************************************************************************
   destructor\n
   *******************************************************************************
 */
MIXIPGW::AccumulatorIndexHeaderImpl::~AccumulatorIndexHeaderImpl(){
}

/**
   version\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return VALUE version : ereror = 0xffffffff
 */
VALUE MIXIPGW::AccumulatorIndexHeaderImpl::Version(void){ return(ACCM_VERSION); }
/**
   type value\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return VALUE type : ereror = 0xffffffff
 */
VALUE MIXIPGW::AccumulatorIndexHeaderImpl::Type(void){ return(type_); }
/**
   attach index header\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]   addr index header : first address
   @param[in]   check check on/off
   @return VALUE type : ereror = 0xffffffff
 */
RETCD MIXIPGW::AccumulatorIndexHeaderImpl::Attach(char* addr,int check){
    addr_ = addr;
    len_  = sizeof(idx_header_t);
    head_ = (VALUE)-1;
    tail_ = (VALUE)-1;
    //
    auto idh = (idx_header_ptr)addr_;
    if (check && idh->version != ACCM_VERSION){
        printf("invalid version(index file):%p/%d\n", idh, check);
        return(-1);
    }
    if (check && idh->head > idh->tail){
        printf("invalid head > tail(index file):%p/%d\n", idh, check);
        return(-1);
    }
    head_ = idh->head;
    tail_ = idh->tail;
    //
    return(0);
}
/**
   index first position\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return VALUE index first position: ereror = 0xffffffff
 */
VALUE MIXIPGW::AccumulatorIndexHeaderImpl::Head(void){
    return(head_);
}
/**
   index tail position\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return VALUE index tail position: ereror = 0xffffffff
 */
VALUE MIXIPGW::AccumulatorIndexHeaderImpl::Tail(void){
    return(tail_);
}
