/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       accumulator_impl.cc
    @brief      accumulator implement
*******************************************************************************
    accumulator inserted into database with logging data via file interface\n
    common implementation of accumulator file interface\n
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

/*! @name accumulatro local defined. */
/* @{ */
static pthread_mutex_t*     _g_mtx_ = NULL;  /*!< IPC sync mutex */
static int                  _g_shmid_ = -1;  /*!< shared memory */
/* @} */


/**
   accumulator main object :  constructor \n
   *******************************************************************************
   constructor\n
   *******************************************************************************
   @param[in]     t   accumulator type(Server or Client)
 */
MIXIPGW::AccumulatorImpl::AccumulatorImpl(ACCM t, DIR dir):type_(t), idx_(new AccumulatorIndexImpl(t,dir)){
    dir_ = (dir?dir:"");
    pthread_mutexattr_t mat;
    // initialized at once
    if (_g_shmid_ == -1){
        // share mutex between processes
        if ((_g_shmid_ = shmget(IPC_PRIVATE, sizeof(pthread_mutex_t), 0600)) < 0){
            rte_panic("shmget (%d: %s)\n", errno, strerror(errno));
        }
        // attach shared memory
        if ((_g_mtx_ = (pthread_mutex_t*)shmat(_g_shmid_, NULL, 0)) == NULL){
            rte_panic("shmat(%d: %s)\n", errno, strerror(errno));
        }
        pthread_mutexattr_init(&mat);
        if (pthread_mutexattr_setpshared(&mat, PTHREAD_PROCESS_SHARED) != 0) {
            rte_panic("pthread_mutexattr_setpshared(%d: %s)\n", errno, strerror(errno));
        }
        pthread_mutex_init(_g_mtx_, &mat);
    }
}

/**
   accumulator main object : destructor\n
   *******************************************************************************
   destructor\n
   *******************************************************************************
 */
MIXIPGW::AccumulatorImpl::~AccumulatorImpl(){
    if (_g_shmid_ != -1){
        if (shmctl(_g_shmid_, IPC_RMID, NULL) != 0){
            perror("shmctl");
        }
    }
    _g_shmid_ = -1;
    if (idx_){
        delete idx_;
    }
    idx_ = NULL;
}

/**
   serialize\n
   *******************************************************************************
   save to file\n
   *******************************************************************************
   @param[in/out]   dir  base directory
   @return RETCD  0==success,0!=error
 */
int MIXIPGW::AccumulatorImpl::Serialize(DIR dir){
    if (dir_.empty() && dir != NULL){
        dir_ = dir;
    }
    if (dir_.empty()){
        return(-1);
    }
    // FIXME:

    return(0);
}
/**
   de-serialize\n
   *******************************************************************************
   read from file\n
   *******************************************************************************
   @param[in/out]   dir base directory
   @return RETCD  0==success,0!=error
 */
int MIXIPGW::AccumulatorImpl::UnSerialize(DIR dir){
    if (dir_.empty() && dir != NULL){
        dir_ = dir;
    }
    if (dir_.empty()){
        return(-1);
    }
    uint64_t    size = 0;
    std::string path = dir_;
    path += "/";
    path += ACCM_INDX_FILE;
    // is exists file
    auto ret = IsExists(path.c_str(), &size);
    // genereate index file when not exists.(server mode)
    if (ret != 0){
        if (type_ == ACCM::SERVER){
            if (CreateFile(path.c_str(), ACCM_INDX_SIZE) < 0){
                rte_panic("CreateIdx - server mode(%s : %d: %s)\n", path.c_str(), errno, strerror(errno));
                return(-1);
            }
            // create new file with empty parameters.
            idx_->WriteEmpty(path.c_str());
        }else{
            rte_panic("missing index file - client mode(%s : %d: %s)\n", path.c_str(), errno, strerror(errno));
            return(-1);
        }
    }
    // attach index objects
    if (idx_->Attach(path.c_str(), 1) != 0){
        rte_panic("index attach (%s : %d: %s)\n", path.c_str(), errno, strerror(errno));
        return(-1);
    }
    return(0);
}

/**
   attach index files\n
   *******************************************************************************
   implemented\n
   *******************************************************************************
   @param[in/out] ppindex  index objects
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::AccumulatorImpl::AttachIndex(AccumulatorIndex** ppindex){
    (*ppindex) = idx_;
    return(0);
}
/**
   detach from index files\n
   *******************************************************************************
   implemented\n
   *******************************************************************************
   @param[in/out] ppindex  index objects
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::AccumulatorImpl::DetachIndex(AccumulatorIndex** ppindex){
    (*ppindex) = NULL;
    return(0);
}

/**
   files is exists + get size\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in] path path
   @param[in/out] size size
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::AccumulatorImpl::IsExists(const char* path,uint64_t* size){
    struct stat		st;
    if (!path || !size)	return(-1);
    if (stat(path,&st) == -1){
        return(-1);
    }
    (*size) = (uint64_t)st.st_size;
    return(0);
}
/**
   create file with specified size\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in] path file path
   @param[in] size file size
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::AccumulatorImpl::CreateFile(const char* path ,uint32_t size){
    char        zero = 0;
    int         fd = -1;

    // file create with Create Mode flags
    if ((fd = open(path, O_CREAT | O_WRONLY | O_TRUNC,S_IREAD | S_IWRITE)) == -1){
        rte_panic("cannot create file - server mode(%s : %d: %s)\n", path, errno, strerror(errno));
        return(-1);
    }
    // extend created file to specified size
    if (lseek(fd, (size - 1), SEEK_SET) == -1){
        rte_panic("extract file (%s[%u] : %d: %s)\n", path, (unsigned)size, errno, strerror(errno));
        return(-1);
    }
    if (write(fd, &zero, sizeof(zero)) == -1){
        rte_panic("write file (%s[%u] : %d: %s)\n", path, (unsigned)size, errno, strerror(errno));
        return(-1);
    }
    // write initialized header
    if (lseek(fd, 0, SEEK_SET) == -1){
        rte_panic("seek back(%s[%u] : %d: %s)\n", path, (unsigned)size, errno, strerror(errno));
        return(-1);
    }
    close(fd);
    return(0);
}

