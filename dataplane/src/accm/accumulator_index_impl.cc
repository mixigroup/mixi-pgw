/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       accumulator_index_impl.cc
    @brief      index implemented
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
   index implemented :  constructor \n
   *******************************************************************************
   constructor\n
   *******************************************************************************
   @param[in]   t   accumulator type(Server or Client)
   @param[in]   dir base directory
 */
MIXIPGW::AccumulatorIndexImpl::AccumulatorIndexImpl(ACCM t, DIR dir):
        type_(t),fd_(-1),mapped_((char*)-1), size_(0), index_header_(new AccumulatorIndexHeaderImpl(t, dir)){
    dir_ = (dir?dir:"");
}

/**
   index implemented : destructor\n
   *******************************************************************************
   destructor\n
   *******************************************************************************
 */
MIXIPGW::AccumulatorIndexImpl::~AccumulatorIndexImpl(){
    Detach();
    if (index_header_){
        delete index_header_;
    }
    index_header_ = NULL;

    for(auto it = index_record_.begin();it != index_record_.end();++it){
        delete ((*it));
    }
    index_record_.clear();
}

/**
   write empty index header structure\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]   path file path
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::AccumulatorIndexImpl::WriteEmpty(const char* path){
    if (Attach(path, 0) != 0){ return(-1); }

    auto idh = (idx_header_ptr)mapped_;
    // initialize all with 0.
    bzero(mapped_, ACCM_INDX_SIZE);
    // version
    idh->version  = ACCM_VERSION;

    printf("valid (WriteEmpty):%p/%s\n", idh, path);

    // detach
    Detach();
    return(0);
}
/**
   attach target index files\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]   path  path
   @param[in]   check initialize=0,typical!=0-> check
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::AccumulatorIndexImpl::Attach(const char* path, int check){
    if (AccumulatorImpl::IsExists(path, &size_) != 0){
        return(-1);
    }
    if (size_ != ACCM_INDX_SIZE){
        return(-1);
    }
    if ((fd_ = open(path, O_RDWR,S_IREAD | S_IWRITE)) == -1){
        return(-1);
    }
    if ((mapped_ = (char*)mmap(NULL, size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0)) == (char*)-1){
        return(-1);
    }
    // parse and, attach object
    if (index_header_->Attach(mapped_, check) != 0){
        return(-1);
    }
    // index header     head = read  start position
    //                  tail = write start position
    //
    // object between head -> tail  : active counter aggregated file
    // other than above             : free space(contiguous free space.)
    //
    auto head = index_header_->Head();
    auto tail = index_header_->Tail();
    printf("record: %u - %u\n", head , tail);
    for(;head < tail; head++){
        index_record_.push_back(new AccumulatorIndexRecordImpl(type_, dir_.c_str()));
    }
    //
    return(0);
}

/**
   detach index files.\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::AccumulatorIndexImpl::Detach(void){
    if (mapped_ != (char*)-1){
        msync(mapped_, size_, 0);
        munmap(mapped_, size_);
    }
    size_ = 0;
    mapped_ = (char*)-1;
    if (fd_ != -1){
        close(fd_);
    }
    fd_ = -1;
    //
    return(0);
}
/**
   refer to index file header\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]   pidx  header object pointer
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::AccumulatorIndexImpl::Header(AccumulatorIndexHeader** pidx){
    (*pidx) = index_header_;
    return(0);
}
/**
   refer to index records\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]   idxr   record object list
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::AccumulatorIndexImpl::Record(std::vector<AccumulatorIndexRecord*>& idxr){
    for(auto it = index_record_.begin();it != index_record_.end();++it){
        idxr.push_back((*it));
    }
    return(0);
}
/**
   add latest record to free space\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::AccumulatorIndexImpl::AutoAlloc(void){

    return(0);
}
/**
   release oldest used region\n
   *******************************************************************************
   \n
   *******************************************************************************
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::AccumulatorIndexImpl::AutoFree(void){
    return(0);
}

