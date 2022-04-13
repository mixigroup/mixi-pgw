/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       static_conf.cc
    @brief      static config  , parameters container
*******************************************************************************
   \n
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
#include "../../inc/static_conf.hpp"
#include "mixi_pgw_data_plane_def.hpp"

/**
   static config  :  constructor \n
   *******************************************************************************
   read default values\n
   *******************************************************************************
 */
MIXIPGW::StaticConf::StaticConf():
    nic_rx_ring_size_(NIC_RXRING),
    nic_tx_ring_size_(NIC_TXRING),
    rx_ring_size_(WORKER_RXRING),
    tx_ring_size_(WORKER_TXRING),
    burst_size_rx_read_(NIC_RXBURST),
    burst_size_rx_enq_(WORKER_RX_ENQ_BURST),
    burst_size_rx_deq_(WORKER_RX_DEQ_BURST),
    burst_size_tx_enq_(WORKER_TX_ENQ_BURST),
    burst_size_tx_deq_(WORKER_TX_DEQ_BURST),
    burst_size_tx_write_(NIC_TXBURST){
}