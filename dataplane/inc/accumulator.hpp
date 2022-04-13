/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       accumulatro.hpp
    @brief      IPC file interface
*******************************************************************************
    accumulator inserted into database with logging data via file interface\n
    common implementation of accumulator file interface\n
    this header file and libaccm.so must be comppile/link independently\n
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
#ifndef __MIXI_PGW_DPDK_ACCUMULATOR_FILE_HPP
#define __MIXI_PGW_DPDK_ACCUMULATOR_FILE_HPP

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <vector>
#include <string>

namespace MIXIPGW{
#ifndef RETCD
  typedef int       RETCD;
#endif
#ifndef SIZE
  typedef unsigned  SIZE;
#endif
#ifndef DIR
  typedef const char* DIR;
#endif
#ifndef VALUE
  typedef unsigned  VALUE;
#endif
#ifndef SEQID
  typedef uint64_t  SEQID;
#endif

  /*! @enum ACCM
    @brief
    direction[FROM, TO]
  */
  enum ACCM{
      SERVER = 0, CLIENT
  };
  class AccumulatorIndex;
  class AccumulatorIndexHeader;
  class AccumulatorIndexRecord;

  /** *****************************************************
  * @brief
  * accumulator main interface\n
  * \n
  */
  class Accumulator{
  public:
      static RETCD Init(ACCM, DIR, Accumulator**);    /*!< initialize with base directory */
      static RETCD UnInit(Accumulator**);             /*!< cleanup */
  public:
      virtual RETCD Serialize(DIR)=0;                 /*!< save(flush) entire files ( index file, multiple record files ) */
      virtual RETCD UnSerialize(DIR)=0;               /*!< read entire files (...) */
      virtual RETCD AttachIndex(AccumulatorIndex**)=0;/*!< attach to index */
      virtual RETCD DetachIndex(AccumulatorIndex**)=0;/*!< detach from index */
  };
  /** *****************************************************
  * @brief
  * accumulator index interface\n
  * \n
  */
  class AccumulatorIndex{
  public:
      virtual RETCD AutoAlloc(void)=0;                              /*!< add latest record to free space */
      virtual RETCD AutoFree(void)=0;                               /*!< release oldest used region */
      virtual RETCD Header(AccumulatorIndexHeader**)=0;             /*!< refer to index file header */
      virtual RETCD Record(std::vector<AccumulatorIndexRecord*>&)=0;/*!< refer to index records */
  };
  /** *****************************************************
  * @brief
  * accumulator index header interface\n
  * \n
  */
  class AccumulatorIndexHeader{
  public:
      virtual VALUE Version(void)=0;  /*!< version */
      virtual VALUE Type(void)=0;     /*!< type code */
      virtual VALUE Head(void)=0;     /*!< first position */
      virtual VALUE Tail(void)=0;     /*!< last position */
  };

  /** *****************************************************
  * @brief
  * accumulator index record header interface\n
  * \n
  */
  class AccumulatorIndexRecord{
  private:
      virtual VALUE Version(void)=0; /*!< version */
      virtual VALUE Type(void)=0;    /*!< type */
      virtual VALUE Time(void)=0;    /*!< time(yyMMddHHmm) : ex. 1710112359 */
      virtual SEQID Seqid(void)=0;   /*!< sequence id */
  };

};

#endif //__MIXI_PGW_DPDK_ACCUMULATOR_FILE_HPP
