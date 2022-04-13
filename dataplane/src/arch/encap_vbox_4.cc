/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       arch/encap_vbox_4.cc
    @brief      Encapsulate  : for virtual machine / debug
*******************************************************************************
      construct architecture , used cores, cores connections , direction , rings , multiple links\n
   exclude from implemetation code that is only for configure.\n
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
/**
 *  --- debug:4core  ---
 *                    +-------+                +------+                +-----+
 * Nic(X) -> nic -->  |Rx(0.0)| --> worker --> |wkr(0)| --> worker -+> |Tx(0)|--> nic --> Nic(Y)
 *       rxring(0)    +-------+ | rxring(0)    +------+  txring(0)  |  +-----+ txring(0)
 *                              |                                   |
 *                              |              +------+             |
 *                              +-> worker --> |wkr(1)| --> worker -+
 *                                rxring(1)    +------+  txring(1)
 *
 *
 */
std::string uri = "mysql://root:develop@127.0.0.1:3306";
app->dburi_ = uri;

PGW_LOG(RTE_LOG_INFO, "architecture [%s]\n", __FILENAME__);
// core placement
app->Add(new CoreRx(0, 0, 0));
app->Add(new CoreEncapWorker(1,0));
app->Add(new CoreEncapWorker(2,0));
app->Add(new CoreTx(1, 3));
// connect in placed cores.
app->Connect( 0,  1, app->conf_->rx_ring_size_);
app->Connect( 0,  2, app->conf_->rx_ring_size_);
app->Connect( 1,  3, app->conf_->tx_ring_size_);
app->Connect( 2,  3, app->conf_->tx_ring_size_);
//
app->Commit(app->conf_);
//
app->BindNic(0, 1, 1, app->conf_->nic_rx_ring_size_, app->conf_->nic_tx_ring_size_, NULL);
app->BindNic(3, 1, 1, app->conf_->nic_rx_ring_size_, app->conf_->nic_tx_ring_size_, NULL);
