/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       accumulator_index_record_impl.cc
    @brief      index record implemented
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
   index record implement :  constructor \n
   *******************************************************************************
   constructor\n
   *******************************************************************************
   @param[in]   t   accumulator type(Server or Client)
   @param[in]   dir base directory
 */
MIXIPGW::AccumulatorIndexRecordImpl::AccumulatorIndexRecordImpl(ACCM t, DIR dir):
        type_(t),addr_(NULL),len_(0){
    dir_ = (dir?dir:"");
}

/**
   index record implemented : destructor\n
   *******************************************************************************
   destructor\n
   *******************************************************************************
 */
MIXIPGW::AccumulatorIndexRecordImpl::~AccumulatorIndexRecordImpl(){}

/**
   record : attach\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]   addr record : first address
   @param[in]   check check on/off
   @return VALUE type : ereror = 0xffffffff
 */
RETCD MIXIPGW::AccumulatorIndexRecordImpl::Attach(char* addr,int check){
    addr_ = addr;
    len_  = sizeof(idx_record_t);
    time_ = (uint32_t)-1;
    seqid_= (uint64_t)-1;
    //
    auto rch = (idx_record_ptr)addr_;
    if (check && rch->version != ACCM_VERSION){
        printf("invalid version(record file):%p/%d\n", rch, check);
        return(-1);
    }
    if (check && (rch->time == 0 || rch->sid == 0)){
        printf("invalid time/sid(record file):%p/%d\n", rch, check);
        return(-1);
    }
    time_ = rch->time;
    seqid_= rch->sid;
    //
    return(0);
}

/**
   version\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return VALUE version : ereror = 0xffffffff
 */
VALUE MIXIPGW::AccumulatorIndexRecordImpl::Version(void){ return(ACCM_VERSION); }
/**
   type value\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return VALUE type : ereror = 0xffffffff
 */
VALUE MIXIPGW::AccumulatorIndexRecordImpl::Type(void){ return(type_); }
/**
   time(yyMMddHHmm) : ex. 1710112359\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return VALUE time: ereror = 0xffffffff
 */
VALUE MIXIPGW::AccumulatorIndexRecordImpl::Time(void){ return(time_); }
/**
   sequence id\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return SEQID sequence id: ereror = 0xffffffff
 */
SEQID MIXIPGW::AccumulatorIndexRecordImpl::Seqid(void){ return(seqid_); }

