/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       arch/count_a05534_direct_db.cc
    @brief      architecture couting : for a05534
*******************************************************************************
      construct architecture , used cores, cores connections , direction , rings , multiple links\n
   exclude from implemetation code that is only for configure.\n
*******************************************************************************
    @date       created(14/feb/2018)

    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 14/feb/2018 
      -# Initial Version
******************************************************************************/

/**
 *
 * A05534          |          A05533
 +------+          |        +-------+
 | SFP+ | << - + -+| - - >> | SFP+  |
 |Nic(X)|\         |        |Nic(N) |
 |      | \        |        |       |
 +------+  \       |        +-------+
 *          \mirror|                             +----+                +--------+                +--------+  +----+
 *           \     |                  + nic Rx-->|q(0)|-+-> worker --> |counting| --> worker --> |countlog|->| DB |
 *            \    |                  | queue(0) |core|   rxring(0)    +--------+  txring(0)     +--------+  +----+
 *             \   |                  |          | Rx |
 *              \  |        +-------+ |          +----+                +--------+                +--------+  +----+
 *               +-|< -+ >> | SFP+  +-+ nic Rx-->|q(1)|-+-> worker --> |counting| --> worker --> |countlog|->| DB |
 *                 |        |Nic(Y) |   ring     |core|   rxring(1)    +--------+  txring(1)     +--------+  +----+
 *                 |        |       |   queue(1) | Rx |
 *                 |        +-------+            +----+
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
app->Add(new CoreCounterLogWorker(18, uri.c_str(), 20));
app->Add(new CoreCounterLogWorker(19, uri.c_str(), 21));

// connect in placed cores.
app->Connect(14, 16, 4096);
app->Connect(15, 17, 4096);
app->Connect(16, 18, 4096);
app->Connect(17, 19, 4096);

app->conf_->burst_size_rx_enq_ = 128;
app->conf_->burst_size_rx_deq_ = 128;
app->conf_->burst_size_tx_enq_ = 128;
app->conf_->burst_size_tx_deq_ = 128;
//
app->Commit(app->conf_);
//
COREID cores[3] = {14, 15,(COREID)-1};
app->BindNic((COREID)-1, 2, 1, app->conf_->nic_rx_ring_size_, app->conf_->nic_tx_ring_size_, cores);
