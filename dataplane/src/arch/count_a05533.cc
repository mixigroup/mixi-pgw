/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       arch/count_a05533.cc
    @brief      counting architecture : for a05533
*******************************************************************************
      construct architecture , used cores, cores connections , direction , rings , multiple links\n
   exclude from implemetation code that is only for configure.\n
*******************************************************************************
    @date       created(17/oct/2017)

    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 17/oct/2017 
      -# Initial Version
******************************************************************************/

/**
 *  --- many core  ---
 * [9 cores ]
 *
 * A05534          |          A05533
 +------+          |        +------+
 | SFP+ | << - + -+| - - >> | SFP+ |
 |Nic(X)|\         |        |Nic(Y)|
 |      | \        |        |      |
 +------+  \       |        +------+
 *          \mirror|                                                  +--------+                +--------+  +----+
 *           \     |                                   +-> worker --> |counting| --> worker --> |transfer|->|eth0|
 *            \    |                                   | rxring(0)    +--------+  txring(0)     +--------+  +----+
 *             \   |                            +----+ |
 *              \  |        +------+            |core| |              +--------+                +--------+  +----+
 *               +-|< -+ >> | SFP+ +-- nic Rx-->| Rx |-+-> worker --> |counting| --> worker --> |transfer|->|eth0|
 *                 |        |Nic(Y)|   ring     |    | | rxring(1)    +--------+  txring(1)     +--------+  +----+
 *                 |        |      |            +----+ |
 *                 |        +------+
 *
 *
 */

std::string uri = "mysql://root:develop@127.0.0.1:3306";
app->dburi_ = uri;

PGW_LOG(RTE_LOG_INFO, "architecture [%s]\n", __FILE__);

// core placement : ingress direction
app->Add(new CoreRx(0, 0, 14));
app->Add(new CoreRx(0, 1, 15));
app->Add(new CoreCounterWorker(16,0));
app->Add(new CoreCounterWorker(17,0));
app->Add(new CoreTransferWorker(18,"10.4.63.15",13868,"10.4.63.11",53868,42));
app->Add(new CoreTransferWorker(19,"10.4.63.15",13869,"10.4.63.11",53868,43));

// connect in placed cores.
app->Connect(14, 16, app->conf_->rx_ring_size_);
app->Connect(15, 17, app->conf_->rx_ring_size_);
app->Connect(16, 18, app->conf_->tx_ring_size_);
app->Connect(17, 19, app->conf_->tx_ring_size_);

//
app->Commit(app->conf_);
//
COREID cores[3] = {14, 15,(COREID)-1};
app->BindNic((COREID)-1, 2, 1, app->conf_->nic_rx_ring_size_, app->conf_->nic_tx_ring_size_, cores);
