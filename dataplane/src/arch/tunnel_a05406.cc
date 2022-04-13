/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       arch/tunnel_a05406.cc
    @brief      tunnel architecture : for a05406
*******************************************************************************
      construct architecture , used cores, cores connections , direction , rings , multiple links\n
   exclude from implemetation code that is only for configure.\n
*******************************************************************************
    @date       created(21/dec/2017)

    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 21/dec/2017 
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
 *            |           +-------+                +------+                                 |
 *            |           |       <-+-- worker --- |wkr(4)|<--- worker -+   +-----+         |
 *            +--nic <--- |Tx(0)  | | txring(4)    +------+  rxring(4)  |<--|Rx(0)|<-- nic -+
 *           txring(0)    +-------+ |                                   |   +-----+ rxring(0)
 *                                  |              +------+             |
 *                                  +-- worker --- |wkr(5)|<--- worker -+
 *                                  | txring(5)    +------+  rxring(5)  |
 *                                  |                                   |
 *                                  |              +------+             |
 *                                  +-- worker --- |wkr(6)|<--- worker -+
 *                                  | txring(6)    +------+  rxring(6)  |
 *                                  |                                   |
 *                                  |              +------+             |
 *                                  +-- worker --- |wkr(7)|<--- worker -+
 *                                    txring(7)    +------+  rxring(7)
 */

std::string uri = "mysql://root:develop@127.0.0.1:3306";
app->dburi_ = uri;

PGW_LOG(RTE_LOG_INFO, "architecture [%s]\n", __FILENAME__);

// core placement : ingress direction
app->Add(new CoreRx(1, 0, 1, TUNNEL_INGRESS));
app->Add(new CoreEncapWorker(3,PGW_TUNNEL_IP));
app->Add(new CoreEncapWorker(5,PGW_TUNNEL_IP));
app->Add(new CoreEncapWorker(7,PGW_TUNNEL_IP));
app->Add(new CoreEncapWorker(9,PGW_TUNNEL_IP));
app->Add(new CoreTx(0, 11));

// core placement : egress direction
app->Add(new CoreRx(0, 0, 25, TUNNEL_EGRESS));
app->Add(new CoreGretermWorker(27,0));
app->Add(new CoreGretermWorker(29,0));
app->Add(new CoreGretermWorker(31,0));
app->Add(new CoreGretermWorker(33,0));
app->Add(new CoreTx(1, 35));

// connect in placed cores.
// ingress
app->Connect(1, 3, app->conf_->rx_ring_size_);
app->Connect(1, 5, app->conf_->rx_ring_size_);
app->Connect(1, 7, app->conf_->rx_ring_size_);
app->Connect(1, 9, app->conf_->rx_ring_size_);

app->Connect(3, 11, app->conf_->rx_ring_size_);
app->Connect(5, 11, app->conf_->rx_ring_size_);
app->Connect(7, 11, app->conf_->rx_ring_size_);
app->Connect(9, 11, app->conf_->rx_ring_size_);

// egress
app->Connect(25, 27, app->conf_->rx_ring_size_);
app->Connect(25, 29, app->conf_->rx_ring_size_);
app->Connect(25, 31, app->conf_->rx_ring_size_);
app->Connect(25, 33, app->conf_->rx_ring_size_);

app->Connect(27, 35, app->conf_->rx_ring_size_);
app->Connect(29, 35, app->conf_->rx_ring_size_);
app->Connect(31, 35, app->conf_->rx_ring_size_);
app->Connect(33, 35, app->conf_->rx_ring_size_);

//
app->Commit(app->conf_);
//
app->BindNic( 1, 1, 1, app->conf_->nic_rx_ring_size_, app->conf_->nic_tx_ring_size_, NULL);
app->BindNic(25, 1, 1, app->conf_->nic_rx_ring_size_, app->conf_->nic_tx_ring_size_, NULL);
