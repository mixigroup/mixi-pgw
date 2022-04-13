/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       arch/pgw_ingress_distributor_a05536_1port.cc
    @brief      PGW  - ingress - distributor : 1port: for a05536
*******************************************************************************
   construct architecture , used cores, cores connections , direction , rings , multiple links\n
   exclude from implemetation code that is only for configure.\n
*******************************************************************************
    @date       created(20/nov/2017)

    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 20/nov/2017 
      -# Initial Version
******************************************************************************/

// ./img/00_summary.png
// ./img/01_detail.png

std::string uri = "mysql://root:develop@127.0.0.1:3306";
app->dburi_ = uri;
CoreTapRx* taprx;
PGW_LOG(RTE_LOG_INFO, "architecture [%s]\n", __FILENAME__);

// core placement
app->Add(new CoreRxDistributor(0, 0, 14));
taprx = new CoreTapRx("tap_00", 19, 0);
app->Add(taprx);
app->Add(new CoreTapTx("tap_00", 18, 0, taprx));

app->Add(new CorePgwIngressDistributorWorker(16,uri.c_str(),1801,20,0xac190202,0x0a190001,0,0));
app->Add(new CorePgwEgressDistributorWorker(17,uri.c_str(),1805,21,0xac190202,0x0a190001,0,0));
app->Add(new CoreTx(0, 0, 15));

// connect in placed cores.

// nic:rx -> except gtpu : tapwrite
app->Connect(14, 18, app->conf_->rx_ring_size_);
// nic:rx -> ingress 
app->Connect(14, 16, app->conf_->rx_ring_size_);
// nic:rx -> egress 
app->Connect(14, 17, app->conf_->rx_ring_size_);
// ingress -> nic:tx
app->Connect(16, 15, app->conf_->tx_ring_size_);
// ingress -> error notify  : error indicate
app->Connect(16, 18, app->conf_->tx_ring_size_);
// tabread -> ingress tunnel-table : warmup
app->Connect(19, 16, app->conf_->rx_ring_size_);
// tapread -> nic:tx
app->Connect(19, 15, app->conf_->tx_ring_size_);
// tapread -> egress tunnel-table : warmup
app->Connect(19, 17, app->conf_->rx_ring_size_);
// egress -> nic:tx
app->Connect(17, 15, app->conf_->tx_ring_size_);
// egress -> error notify  : error indicate
app->Connect(17, 18, app->conf_->tx_ring_size_);
//
app->Commit(app->conf_);
//
app->BindNic(14, 1, 1, app->conf_->nic_rx_ring_size_, app->conf_->nic_tx_ring_size_, NULL);
