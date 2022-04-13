/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       static_conf.hpp
    @brief      container of static configuration
*******************************************************************************
*******************************************************************************
    @date       created(27/sep/2017)

    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 27/sep/2017 
      -# Initial Version
******************************************************************************/
#ifndef __STATIC_CONF_HPP
#define __STATIC_CONF_HPP

#include "mixi_pgw_data_plane_def.hpp"

namespace MIXIPGW{
  /** *****************************************************
   * @brief
   * static config.\n
   * parameters container\n
   */
  class StaticConf{
  public:
      StaticConf();
  public:
      SIZE  nic_rx_ring_size_;   /*!< receive-side, count of NIC ring */
      SIZE  nic_tx_ring_size_;   /*!< transfer-side, count of NIC ring */
      SIZE  rx_ring_size_;       /*!< receive-side, count of dpdk@ring */
      SIZE  tx_ring_size_;       /*!< transfer-side, count of dpdk@ring */
      SIZE  burst_size_rx_read_; /*!< Rx : burst size for receiving NIC */
      SIZE  burst_size_rx_enq_;  /*!< Rx : Rx -> burst size for worker enqueue */
      SIZE  burst_size_rx_deq_;  /*!< Wk : Rx -> burst size for worker dequeue */
      SIZE  burst_size_tx_enq_;  /*!< Wk : worker -> burst size for Tx enqueue */
      SIZE  burst_size_tx_deq_;  /*!< Tx : worker -> burst size for Tx dequeue */
      SIZE  burst_size_tx_write_;/*!< Tx : burst size for transfering NIC */
  };
};

#endif //__STATIC_CONF_HPP
