/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       arch/count_a05411_direct_db.cc
    @brief      architecture counting :  for a05411
*******************************************************************************
   construct architecture , used cores, cores connections , direction , rings , multiple links\n
   exclude from implemetation code that is only for configure.\n
*******************************************************************************
    @date       created(23/feb/2018)

    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 23/feb/2018 
      -# Initial Version
******************************************************************************/
/**
 *
 * SGW             |          PGW
 +------+          |        +-------+
 | SFP+ | << - + -+| - - >> | SFP+  |
 |Nic(X)|\         |        |Nic(N) |
 |      | \        |        |       |
 +------+\ \       |        +-------+
 *       \  \mirror/ingress                      +----+                +--------+                +--------+
 *       \   \     |                  + nic Rx-->|q(0)|-+-> worker --> |counting| --> worker --> |countlog|-+
 *        \   \    |                  | queue(0) |core|   rxring(0)    +--------+  txring(0)     +--------+ |
 *         \   \   |        a05411    |          | Rx |                                                     |
 *          \   \  |        +-------+ |          +----+                +--------+                +--------+ |     +----+
 *          |    +-|< -+ >> | SFP+  +-+ nic Rx-->|q(1)|-+-> worker --> |counting| --> worker --> |countlog|-+---> | DB |
 *          |      |        |Nic(Y) |   ring     |core|   rxring(1)    +--------+  txring(1)     +--------+ |     |    |
 *          |      |        |       |   queue(1) | Rx |                                                     |     |    |
 *          |      |        +-------+            +----+                                                     |     |    |
 *          |                                                                                               |     |    |
 *          \mirror/egress                       +----+                +--------+                +--------+ |     |----+
 *           \     |                  + nic Rx-->|q(0)|-+-> worker --> |counting| --> worker --> |countlog|-+
 *            \    |                  | queue(0) |core|   rxring(0)    +--------+  txring(0)     +--------+ |
 *             \   |        a05411    |          | Rx |                                                     |
 *              \  |        +-------+ |          +----+                +--------+                +--------+ |
 *               +-|< -+ >> | SFP+  +-+ nic Rx-->|q(1)|-+-> worker --> |counting| --> worker --> |countlog|-+
 *                 |        |Nic(Z) |   ring     |core|   rxring(1)    +--------+  txring(1)     +--------+
 *                 |        |       |   queue(1) | Rx |
 *                 |        +-------+            +----+
 *
 *
 */

std::string uri = "mysql://root:develop@127.0.0.1:3306";
app->dburi_ = uri;

PGW_LOG(RTE_LOG_INFO, "architecture [%s]\n", __FILE__);

// core placement : ingress direction
app->Add(new CoreRx(0, 0, 25));
app->Add(new CoreRx(0, 1, 33));
app->Add(new CoreCounterWorker(27,COUNTER_INGRESS));
app->Add(new CoreCounterWorker(35,COUNTER_INGRESS));
app->Add(new CoreCounterLogWorker(29, uri.c_str(), 31));
app->Add(new CoreCounterLogWorker(37, uri.c_str(), 39));

// core placement : Egress direction
app->Add(new CoreRx(1, 0,  1));
app->Add(new CoreRx(1, 1,  9));
app->Add(new CoreCounterWorker( 3,COUNTER_EGRESS));
app->Add(new CoreCounterWorker(11,COUNTER_EGRESS));
app->Add(new CoreCounterLogWorker( 5, uri.c_str(),  7));
app->Add(new CoreCounterLogWorker(13, uri.c_str(), 15));

// connect in placed cores.
app->Connect(25, 27, 4096);
app->Connect(33, 35, 4096);
app->Connect(27, 29, 4096);
app->Connect(35, 37, 4096);

// connect in placed cores.
app->Connect( 1,  3, 4096);
app->Connect( 9, 11, 4096);
app->Connect( 3,  5, 4096);
app->Connect(11, 13, 4096);

app->conf_->burst_size_rx_enq_ = 128;
app->conf_->burst_size_rx_deq_ = 128;
app->conf_->burst_size_tx_enq_ = 128;
app->conf_->burst_size_tx_deq_ = 128;
//
app->Commit(app->conf_);
//
COREID cores[3] = {25, 33,(COREID)-1};
app->BindNic((COREID)-1, 2, 1, app->conf_->nic_rx_ring_size_, app->conf_->nic_tx_ring_size_, cores);

cores[0] = 1;
cores[1] = 9;
app->BindNic((COREID)-1, 2, 1, app->conf_->nic_rx_ring_size_, app->conf_->nic_tx_ring_size_, cores);

