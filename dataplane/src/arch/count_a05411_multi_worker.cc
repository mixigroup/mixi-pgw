/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       arch/count_a05411_multi_worker.cc
    @brief      architecture counting : for a05411, multi worker
*******************************************************************************
   construct architecture , used cores, cores connections , direction , rings , multiple links\n
   exclude from implemetation code that is only for configure.\n
*******************************************************************************
    @date       created(01/mar/2018)
    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 01/mar/2018 
      -# Initial Version
******************************************************************************/

/**
 * SGW             |          PGW
 +------+          |        +-------+
 | SFP+ | << - + -+| - - >> | SFP+  |
 |Nic(X)|          |        |Nic(N) |
 |      |          |        |       |
 +-+----+\         |        +-------+
   |     \
   |     |
   |   mirror/ingress
 mirror  |                                         +------+              +-----------+
 egress  |                          +-> worker --> |wkr(0)| --> worker -+|CountLog(0)+--+
   |     |                          | rxring(0)    +------+  txring(0)   +-----------+  |
   |     |                          |                                                   |
   |     |                          |              +------+              +-----------+  |
   |     |    +---------------------+-> worker --> |wkr(1)| --> worker -+|CountLog(1)+--+----------------+
   |  +--+--+ | rxring(0)           | rxring(1)    +------+  txring(1)   +-----------+  |           mysql protocol
   |  |     | |                     |                                                   |                |
   |  |RX(0)+-+                     |              +------+              +-----------+  |                |
   |  |     |                       +-> worker --> |wkr(2)| --> worker -+|CountLog(2)+--+                |
   |  +-----+                       | rxring(2)    +------+  txring(2)   +-----------+  |                |    +---------+
   |                                |                                                   |                +-->>|         |
   |                                |              +------+              +-----------+  |                |    | DB      |
   |                                +-> worker --> |wkr(3)| --> worker -+|CountLog(3)+--+                |    |         |
   +----+                             rxring(3)    +------+  txring(3)   +-----------+                   |    +---------+
        |                                          +------+              +-----------+                   |
 *      |                           +-> worker --> |wkr(0)| --> worker -+|CountLog(0)+--+                |
 *      |                           | rxring(0)    +------+  txring(0)   +-----------+  |                |
 *      |                           |                                                   |          mysql protocol
 *    +-+---+                       |              +------+              +-----------+  |                |
 *    |     | +---------------------+-> worker --> |wkr(1)| --> worker -+|CountLog(1)+--+----------------+
 *    |RX(1)+-+ rxring(0)           | rxring(1)    +------+  txring(1)   +-----------+  |
 *    |     |                       |                                                   |
 *    +-----+                       |              +------+              +-----------+  |
 *                                  +-> worker --> |wkr(2)| --> worker -+|CountLog(2)+--+
 *                                  | rxring(2)    +------+  txring(2)   +-----------+  |
 *                                  |                                                   |
 *                                  |              +------+              +-----------+  |
 *                                  +-> worker --> |wkr(3)| --> worker -+|CountLog(3)+--+
 *                                    rxring(3)    +------+  txring(3)   +-----------+
 */

#define OFFSET(c)   (c+1) // NUMA node1

std::string uri = "mysql://root:develop@127.0.0.1:3306";
app->dburi_ = uri;

PGW_LOG(RTE_LOG_INFO, "architecture [%s]\n", __FILE__);

// core placement : ingress direction NUMA node0
app->Add(new CoreRx(0, 0, 1,COUNTER_INGRESS));
app->Add(new CoreCounterWorker(3,COUNTER_INGRESS));
app->Add(new CoreCounterWorker(5,COUNTER_INGRESS));
app->Add(new CoreCounterWorker(7,COUNTER_INGRESS));
app->Add(new CoreCounterWorker(9,COUNTER_INGRESS));
app->Add(new CoreCounterWorker(11,COUNTER_INGRESS));
app->Add(new CoreCounterWorker(13,COUNTER_INGRESS));
app->Add(new CoreCounterLogWorker(15, uri.c_str(), 17));
app->Add(new CoreCounterLogWorker(19, uri.c_str(), 21));
app->Add(new CoreCounterLogWorker(23, uri.c_str(), 25));
app->Add(new CoreCounterLogWorker(27, uri.c_str(), 29));
app->Add(new CoreCounterLogWorker(31, uri.c_str(), 33));
app->Add(new CoreCounterLogWorker(35, uri.c_str(), 37));

// core placement : Egress direction NUMA node1
app->Add(new CoreRx(1, 0, OFFSET(1),COUNTER_EGRESS));
app->Add(new CoreCounterWorker(OFFSET(3),COUNTER_EGRESS));
app->Add(new CoreCounterWorker(OFFSET(5),COUNTER_EGRESS));
app->Add(new CoreCounterWorker(OFFSET(7),COUNTER_EGRESS));
app->Add(new CoreCounterWorker(OFFSET(9),COUNTER_EGRESS));
app->Add(new CoreCounterWorker(OFFSET(11),COUNTER_EGRESS));
app->Add(new CoreCounterWorker(OFFSET(13),COUNTER_EGRESS));
app->Add(new CoreCounterLogWorker(OFFSET(15), uri.c_str(), OFFSET(17)));
app->Add(new CoreCounterLogWorker(OFFSET(19), uri.c_str(), OFFSET(21)));
app->Add(new CoreCounterLogWorker(OFFSET(23), uri.c_str(), OFFSET(25)));
app->Add(new CoreCounterLogWorker(OFFSET(27), uri.c_str(), OFFSET(29)));
app->Add(new CoreCounterLogWorker(OFFSET(31), uri.c_str(), OFFSET(33)));
app->Add(new CoreCounterLogWorker(OFFSET(35), uri.c_str(), OFFSET(37)));

// connect in placed cores.
app->Connect(1, 3, 4096);
app->Connect(1, 5, 4096);
app->Connect(1, 7, 4096);
app->Connect(1, 9, 4096);
app->Connect(1, 11, 4096);
app->Connect(1, 13, 4096);
app->Connect(3, 15, 1024);
app->Connect(5, 19, 1024);
app->Connect(7, 23, 1024);
app->Connect(9, 27, 1024);
app->Connect(11,31, 1024);
app->Connect(13,35, 1024);

// connect in placed cores.

app->Connect(OFFSET(1), OFFSET(3), 4096);
app->Connect(OFFSET(1), OFFSET(5), 4096);
app->Connect(OFFSET(1), OFFSET(7), 4096);
app->Connect(OFFSET(1), OFFSET(9), 4096);
app->Connect(OFFSET(1), OFFSET(11), 4096);
app->Connect(OFFSET(1), OFFSET(13), 4096);
app->Connect(OFFSET(3), OFFSET(15), 1024);
app->Connect(OFFSET(5), OFFSET(19), 1024);
app->Connect(OFFSET(7), OFFSET(23), 1024);
app->Connect(OFFSET(9), OFFSET(27), 1024);
app->Connect(OFFSET(11), OFFSET(31), 1024);
app->Connect(OFFSET(13), OFFSET(35), 1024);

app->conf_->burst_size_rx_enq_ = 128;
app->conf_->burst_size_rx_deq_ = 128;
app->conf_->burst_size_tx_enq_ = 128;
app->conf_->burst_size_tx_deq_ = 128;
//
app->Commit(app->conf_);
//
app->BindNic(1, 1, 1, app->conf_->nic_rx_ring_size_, app->conf_->nic_tx_ring_size_, NULL);
app->BindNic(OFFSET(1), 1, 1, app->conf_->nic_rx_ring_size_, app->conf_->nic_tx_ring_size_, NULL);
