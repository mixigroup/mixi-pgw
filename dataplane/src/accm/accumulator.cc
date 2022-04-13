/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       accumulatro_file.cc
    @brief      IPC file interface 
*******************************************************************************
   accumulator inserted into database with logging data via file interface\n
   common implementation of accumulator file interface\n
   \n
   \n
*******************************************************************************
    @date       created(10/oct/2017)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 10/oct/2017 
      -# Initial Version
******************************************************************************/
#include "mixi_pgw_data_plane_def.hpp"
#include "accumulator.hpp"
#include "accumulator_internal.hpp"

#include <ftw.h>

using namespace MIXIPGW;
/**
   initialize accumulator\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in]       t     accumulator type
   @param[in]       dir   base directory
   @param[in/out]   pinst accumulator object handle
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::Accumulator::Init(ACCM t, DIR dir, Accumulator** ppinst){
    (*ppinst) = new AccumulatorImpl(t, dir);
    // server-side, initialize entire index and record files
    if (t == ACCM::SERVER){
        nftw(dir, [](const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf){
            printf("%s : %u - %s\n", fpath, (unsigned)sb->st_size, (tflag==FTW_F)?"file":"directory");
            if (tflag == FTW_F){
                unlink(fpath);
            }
            return(0);
        }, 16, FTW_DEPTH | FTW_PHYS);
    }
    return(0);
}
/**
   cleanup accumulator\n
   *******************************************************************************
   \n
   *******************************************************************************
   @param[in/out]   pinst  accumulator object handle
   @return RETCD  0==success,0!=error
 */
RETCD MIXIPGW::Accumulator::UnInit(Accumulator** ppinst){
    if ((*ppinst)){
        delete (AccumulatorImpl*)(*ppinst);
    }
    (*ppinst) = NULL;
    return(0);
}

