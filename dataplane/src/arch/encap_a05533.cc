/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       arch/encap_a05533.cc
    @brief      Encap : for a05533
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
 *  --- many core  ---
 *                                                 +------+
 *                                  +-> worker --> |wkr(0)| --> worker -+
 *                                  | rxring(0)    +------+  txring(0)  |
 *                                  |                                   |
 *                        +-------+ |              +------+             |  +-----+
 *            +-- nic --> |Rx(0)  |-+-> worker --> |wkr(1)| --> worker -+> |Tx(0)|--> nic --+
 *            | rxring(0) +-------+ | rxring(1)    +------+  txring(1)  |  +-----+ txring(0)|
 *            |                     |                                   |                   |
 *            |                     |              +------+             |                   |
 *            |                     +-> worker --> |wkr(2)| --> worker -+                   |
 *            |                     | rxring(2)    +------+  txring(2)  |                   |    +---------+
 +------+     |                     |                                   |                   +-->>|         |
 |      | -->>+                     |              +------+             |                   +--<<| Nic(Y)  |
 |Nic(X)| <<--+                     +-> worker --> |wkr(3)| --> worker -+                   |    |         |
 +------+     |                       rxring(3)    +------+  txring(3)                      |    +---------+
 *            |                                                                             |
 *            |                                                                             |
 *            |           +-------+                +------+                +-----+          |
 *            +--nic <--- |Tx(0)  |<--- worker <-- |wkr(r)| <-- worker <-- |Rx(0)|<-- nic <-+
 *           rxring(0)    +-------+   txring(r)    +------+  rxring(r)     +-----+ rxring(0)
 *
 *
 */

std::string uri = "mysql://root:develop@127.0.0.1:3306";
app->dburi_ = uri;

PGW_LOG(RTE_LOG_INFO, "architecture [%s]\n", __FILENAME__);

// core placement : ingress direction
app->Add(new CoreRx(0, 0, 14));
app->Add(new CoreEncapWorker(15,0));
app->Add(new CoreEncapWorker(16,0));
app->Add(new CoreEncapWorker(17,0));
app->Add(new CoreEncapWorker(18,0));
app->Add(new CoreTx(1, 19));

// core placement : egress direction
app->Add(new CoreRx(1, 0, 20));
app->Add(new CoreEncapWorker(21,0));
app->Add(new CoreTx(0, 22));

// connect in placed cores.
app->Connect(14, 15, app->conf_->rx_ring_size_);
app->Connect(14, 16, app->conf_->rx_ring_size_);
app->Connect(14, 17, app->conf_->tx_ring_size_);
app->Connect(14, 18, app->conf_->tx_ring_size_);
app->Connect(15, 19, app->conf_->tx_ring_size_);
app->Connect(16, 19, app->conf_->tx_ring_size_);
app->Connect(17, 19, app->conf_->tx_ring_size_);
app->Connect(18, 19, app->conf_->tx_ring_size_);
// egress
app->Connect(20, 21, app->conf_->rx_ring_size_);
app->Connect(21, 22, app->conf_->rx_ring_size_);

//
app->Commit(app->conf_);
//
app->BindNic(14, 1, 1, app->conf_->nic_rx_ring_size_, app->conf_->nic_tx_ring_size_, NULL);
app->BindNic(19, 1, 1, app->conf_->nic_rx_ring_size_, app->conf_->nic_tx_ring_size_, NULL);
