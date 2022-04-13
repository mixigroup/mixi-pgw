/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       accumulatro_internal.hpp
    @brief      accumulator library  : header files internal use
*******************************************************************************
   private header\n
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
#ifndef MIXI_PGW_DPDK_ACCUMULATOR_INTERNAL_HPP
#define MIXI_PGW_DPDK_ACCUMULATOR_INTERNAL_HPP

#include "accumulator.hpp"

#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <pthread.h>

namespace MIXIPGW{
  /*! @struct idx_header
      @brief
      index file header\n
      8 octets\n
  */
  typedef struct idx_header {
      uint32_t  version:8;
      uint32_t  type:8;       /*!< kind code :  */
      uint32_t  reserved:16;  /*!< reserved:not use */
      uint16_t  head;         /*!< valid index offset */
      uint16_t  tail;         /*!< valid index offset tail position */
  } __attribute__ ((packed)) idx_header_t,*idx_header_ptr;
  static_assert(sizeof(idx_header_t)==8, "idx_header = 8 octet only.");

  /*! @struct idx_record
      @brief
      record of index file(reuse like ring buffer)\n
      16 octets\n
  */
  typedef struct idx_record{
      uint32_t  version:8;
      uint32_t  type:8;
      uint32_t  reserved:16;
      uint32_t  time;         /*!< time : YYMMDDHHMM(ex: 1710102359) */
      uint64_t  sid;          /*!< sequence id */
  } __attribute__ ((packed)) idx_record_t,*idx_record_ptr;
  static_assert(sizeof(idx_record_t)==16, "idx_record = 16 octet only.");


  /*! @name accumulatro local defined. */
  /* @{ */
  static const uint32_t ACCM_VERSION = 0x01;            /*!< accumulator version */
  static const uint32_t ACCM_INVALIDVERSION = 0x02;     /*!< invlid version */
  static const char*    ACCM_INDX_FILE  = "accm.idx";   /*!< index file path */
  static const uint32_t ACCM_INDX_REC_CNT = 1024;       /*!< index count of records */
  static const uint32_t ACCM_INDX_SIZE  = (sizeof(idx_header_t) + (ACCM_INDX_REC_CNT * sizeof(idx_record_t))); /*!< index file size */
  /* @} */

  class AccumulatorIndexImpl;

  /** *****************************************************
   * @brief
   * accumulator main implemented\n
   * \n
   */
  class AccumulatorImpl: public Accumulator{
  public:
      AccumulatorImpl(ACCM, DIR);
      virtual ~AccumulatorImpl();
  public:
      static RETCD IsExists(const char*,uint64_t*);   /*!< check file existence */
      static RETCD CreateFile(const char*,uint32_t);  /*!< create file with size */
  public:
      virtual RETCD Serialize(DIR);                   /*!< save(flush) entire files(index file, multiple record file) */
      virtual RETCD UnSerialize(DIR);                 /*!< read entire files(...) */
      virtual RETCD AttachIndex(AccumulatorIndex**);  /*!< attach to index */
      virtual RETCD DetachIndex(AccumulatorIndex**);  /*!< detach from index */
  private:
      ACCM                  type_;  /*!< type: server or client */
      std::string           dir_;   /*!< base directory */
      AccumulatorIndexImpl* idx_;   /*!< main object of index implemented */
  };

  /** *****************************************************
   * @brief
   * accumulator index implemented\n
   * \n
   */
  class AccumulatorIndexHeaderImpl;
  class AccumulatorIndexImpl: public AccumulatorIndex{
      friend class AccumulatorImpl;
  public:
      AccumulatorIndexImpl(ACCM, DIR);
      virtual ~AccumulatorIndexImpl();
  public:
      virtual RETCD AutoAlloc(void);                               /*!< add latest record to free space */
      virtual RETCD AutoFree(void);                                /*!< release oldest used region */
      virtual RETCD Header(AccumulatorIndexHeader**);              /*!< refer to index file header */
      virtual RETCD Record(std::vector<AccumulatorIndexRecord*>&); /*!< refer to index record */
  private:
      RETCD Attach(const char*,int); /*!< open file, and mapping to memory */
      RETCD Detach(void);            /*!< detach memory/mapping */
      RETCD WriteEmpty(const char*); /*!< write empty index header structure */
  private:
      ACCM          type_;           /*!< type: server or client */
      std::string   dir_;            /*!< base directory */
      int           fd_;             /*!< index file descriptor */
      uint64_t      size_;           /*!< index file size */
      char*         mapped_;         /*!< index file memory mapped */
      AccumulatorIndexHeaderImpl*   index_header_;  /*!< index file header */
      std::vector<AccumulatorIndexRecord*>  index_record_; /*!< index record */
  };

  /** *****************************************************
  * @brief
  * accumulator index header interface\n
  * \n
  */
  class AccumulatorIndexHeaderImpl: public AccumulatorIndexHeader{
      friend class AccumulatorIndexImpl;
  public:
      AccumulatorIndexHeaderImpl(ACCM, DIR);
      virtual ~AccumulatorIndexHeaderImpl();
  public:
      virtual VALUE Version(void);  /*!< version */
      virtual VALUE Type(void);     /*!< type code */
      virtual VALUE Head(void);     /*!< first position index */
      virtual VALUE Tail(void);     /*!< tail position index */
  private:
      RETCD Attach(char*,int);      /*!< attach index header */
  private:
      char*         addr_;          /*!< index header : first address */
      uint32_t      len_;           /*!< index header : header length */
      ACCM          type_;          /*!< type: server or client */
      std::string   dir_;           /*!< base directory */
      VALUE         head_;          /*!< first position */
      VALUE         tail_;          /*!< tail position */
  };

  /** *****************************************************
  * @brief
  * accumulator index record interface\n
  * \n
  */
  class AccumulatorIndexRecordImpl: public AccumulatorIndexRecord{
      friend class AccumulatorIndexImpl;
  public:
      AccumulatorIndexRecordImpl(ACCM, DIR);
      virtual ~AccumulatorIndexRecordImpl();
  public:
      virtual VALUE Version(void);  /*!< version */
      virtual VALUE Type(void);     /*!< type */
      virtual VALUE Time(void);     /*!< time(yyMMddHHmm) : ex. 1710112359 */
      virtual SEQID Seqid(void);    /*!< sequence id */
  private:
      RETCD Attach(char*,int);      /*!< index head : attach */
  private:
      char*         addr_;          /*!< index head : first address */
      uint32_t      len_;           /*!< index head : header length */
      ACCM          type_;          /*!< type: server or client */
      std::string   dir_;           /*!< base directory */
      uint32_t      time_;          /*!< time */
      uint64_t      seqid_;         /*!< sequence id */
  };

};

#endif //MIXI_PGW_DPDK_ACCUMULATOR_INTERNAL_HPP
