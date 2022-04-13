/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       arch/pgw_egress_a05533.cc
    @brief      PGW  - egress : for a05533
*******************************************************************************
      construct architecture , used cores, cores connections , direction , rings , multiple links\n
   exclude from implemetation code that is only for configure.\n
*******************************************************************************
    @date       created(31/oct/2017)

    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 31/oct/2017 
      -# Initial Version
******************************************************************************/

/**                                                 +------+
 *  --- many core  ---                              |      |
 *                                                  |  +---+--+
 *                                  +-> worker ------->|wkr(0)| --> worker -+
 *                                  | rxring(0)     |  +------+  txring(0)  |
 *                                  |               +------+                |
 *                                  |               |      |                |
 *                        +-------+ |               |  +---+--+             |  +-----+
 *            +-- nic --> |Rx(0)  |-+-> worker ------->|wkr(1)| --> worker -+> |Tx(0)|--> nic --+
 *            | rxring(0) +-------+ | rxring(1)     |  +------+  txring(1)  |  +-----+ txring(0)|
 *            |                     |               +------+                |                   |
 *            |                     |               |      |                |                   |
 *            |                     |               |  +---+--+             |                   |
 *            |                     +-> worker ------->|wkr(2)| --> worker -+                   |
 *            |                     | rxring(2)     |  +------+  txring(2)  |                   |    +---------+
 +------+     |                     |               +------+                |                   +-->>|         |
 |      |     |                     |               |      |                |                        |         |
 |      | -->>+                     |               |  +---+--+             |                        | Nic(Y)  |
 |Nic(X)|                           +-> worker ------->|wkr(3)| --> worker -+                        |         |
 +------+                             rxring(3)     |  +------+  txring(3)                           +---------+
 *                                                  |
 *                                                  |
 *                                                 /|
 *                                                  |
 *                                                  |
 *                                                  |
 *                                                  |
 *                                              +---+-------+
 *                                              | binlog    |
 *                                              +-----------+
 *
 *
 */

std::string uri = "mysql://root:develop@127.0.0.1:3306";
app->dburi_ = uri;

PGW_LOG(RTE_LOG_INFO, "architecture [%s]\n", __FILENAME__);

// core placement : egress direction
app->Add(new CoreRx(0, 0, 14));
app->Add(new CorePgwEgressWorker(15,uri.c_str(),1801,10,0x0a04c001,0x0a190001,0,0));
app->Add(new CorePgwEgressWorker(16,uri.c_str(),1802,10,0x0a04c002,0x0a190002,1,0));
app->Add(new CorePgwEgressWorker(17,uri.c_str(),1803,10,0x0a04c003,0x0a190003,2,0));
app->Add(new CorePgwEgressWorker(18,uri.c_str(),1804,10,0x0a04c004,0x0a190004,3,0));
app->Add(new CoreTx(1, 19));

// connect in placed cores.
app->Connect(14, 15, app->conf_->rx_ring_size_);
app->Connect(14, 16, app->conf_->rx_ring_size_);
app->Connect(14, 17, app->conf_->tx_ring_size_);
app->Connect(14, 18, app->conf_->tx_ring_size_);
app->Connect(15, 19, app->conf_->tx_ring_size_);
app->Connect(16, 19, app->conf_->tx_ring_size_);
app->Connect(17, 19, app->conf_->tx_ring_size_);
app->Connect(18, 19, app->conf_->tx_ring_size_);

//
app->Commit(app->conf_);
//
app->BindNic(14, 1, 1, app->conf_->nic_rx_ring_size_, app->conf_->nic_tx_ring_size_, NULL);
app->BindNic(19, 1, 1, app->conf_->nic_rx_ring_size_, app->conf_->nic_tx_ring_size_, NULL);
