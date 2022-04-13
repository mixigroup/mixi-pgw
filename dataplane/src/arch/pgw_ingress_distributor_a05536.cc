/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       arch/pgw_ingress_distributor_a05536.cc
    @brief      PGW  - ingress - distributor : for a05536
*******************************************************************************
   construct architecture , used cores, cores connections , direction , rings , multiple links\n
   exclude from implemetation code that is only for configure.\n
*******************************************************************************
    @date       created(16/nov/2017)

    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 16/nov/2017 
      -# Initial Version
******************************************************************************/

// ./img/00_summary.png
// ./img/01_detail.png

std::string uri = "mysql://root:develop@127.0.0.1:3306";
app->dburi_ = uri;
CoreTapRx* taprx;
PGW_LOG(RTE_LOG_INFO, "architecture [%s]\n", __FILENAME__);

// core placement : ingress direction
app->Add(new CoreRxDistributor(0, 0, 14));
taprx = new CoreTapRx("tap_00", 16, 0);
app->Add(taprx);
app->Add(new CoreTapTx("tap_00", 15, 0, taprx));

app->Add(new CorePgwIngressDistributorWorker(17,uri.c_str(),1801,10,0x0a04c001,0x0a190001,0,0));
app->Add(new CorePgwIngressDistributorWorker(18,uri.c_str(),1802,10,0x0a04c002,0x0a190002,1,0));
app->Add(new CorePgwIngressDistributorWorker(19,uri.c_str(),1803,10,0x0a04c003,0x0a190003,2,0));
app->Add(new CorePgwIngressDistributorWorker(20,uri.c_str(),1804,10,0x0a04c004,0x0a190004,3,0));
app->Add(new CoreTx(0, 21));

// core placement : Egress direction
app->Add(new CoreTx(1, 22));
app->Add(new CoreRxDistributor(1, 0, 23));
taprx = new CoreTapRx("tap_01", 28, 0);
app->Add(taprx);
app->Add(new CoreTapTx("tap_01", 29, 0, taprx));

app->Add(new CorePgwEgressDistributorWorker(24,uri.c_str(),1805,10,0x0a04c001,0x0a190001,0,0));
app->Add(new CorePgwEgressDistributorWorker(25,uri.c_str(),1806,10,0x0a04c002,0x0a190002,1,0));
app->Add(new CorePgwEgressDistributorWorker(26,uri.c_str(),1807,10,0x0a04c003,0x0a190003,2,0));
app->Add(new CorePgwEgressDistributorWorker(27,uri.c_str(),1808,10,0x0a04c004,0x0a190004,3,0));

// connect in placed cores.

// nic:rx -> except gtpu : tapwrite
app->Connect(14, 15, app->conf_->rx_ring_size_);
// nic:rx -> ingress scale-out by teid
app->Connect(14, 17, app->conf_->rx_ring_size_);
app->Connect(14, 18, app->conf_->tx_ring_size_);
app->Connect(14, 19, app->conf_->tx_ring_size_);
app->Connect(14, 20, app->conf_->tx_ring_size_);
// ingress[n] -> nic:tx
app->Connect(17, 21, app->conf_->tx_ring_size_);
app->Connect(18, 21, app->conf_->tx_ring_size_);
app->Connect(19, 21, app->conf_->tx_ring_size_);
app->Connect(20, 21, app->conf_->tx_ring_size_);
// ingress[n] -> error notify  : error indicate
app->Connect(17, 15, app->conf_->tx_ring_size_);
app->Connect(18, 15, app->conf_->tx_ring_size_);
app->Connect(19, 15, app->conf_->tx_ring_size_);
app->Connect(20, 15, app->conf_->tx_ring_size_);

// ingress tabread -> ingress tunnel-table : warmup
app->Connect(16, 17, app->conf_->rx_ring_size_);
app->Connect(16, 18, app->conf_->rx_ring_size_);
app->Connect(16, 19, app->conf_->rx_ring_size_);
app->Connect(16, 20, app->conf_->rx_ring_size_);
// ingress tapread -> nic:tx
app->Connect(16, 21, app->conf_->tx_ring_size_);

// ingress tapread -> egress tunnel-table : warmup
app->Connect(16, 24, app->conf_->rx_ring_size_);
app->Connect(16, 25, app->conf_->rx_ring_size_);
app->Connect(16, 26, app->conf_->rx_ring_size_);
app->Connect(16, 27, app->conf_->rx_ring_size_);

// egress tapread -> nic:tx
app->Connect(28, 22, app->conf_->rx_ring_size_);
// egress nic:rx -> tapwrite
app->Connect(23, 29, app->conf_->rx_ring_size_);

// egress nic:rx -> egress[n]
app->Connect(23, 24, app->conf_->rx_ring_size_);
app->Connect(23, 25, app->conf_->rx_ring_size_);
app->Connect(23, 26, app->conf_->rx_ring_size_);
app->Connect(23, 27, app->conf_->rx_ring_size_);
// egress[n] -> nic:tx
app->Connect(24, 22, app->conf_->rx_ring_size_);
app->Connect(25, 22, app->conf_->rx_ring_size_);
app->Connect(26, 22, app->conf_->rx_ring_size_);
app->Connect(27, 22, app->conf_->rx_ring_size_);
//
app->Commit(app->conf_);
//
app->BindNic(14, 1, 1, app->conf_->nic_rx_ring_size_, app->conf_->nic_tx_ring_size_, NULL);
app->BindNic(23, 1, 1, app->conf_->nic_rx_ring_size_, app->conf_->nic_tx_ring_size_, NULL);
