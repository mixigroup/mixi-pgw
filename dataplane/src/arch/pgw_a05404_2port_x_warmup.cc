/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       arch/pgw_a05404_2port_x_warmup.cc
    @brief      PGW 2port: for a05404
*******************************************************************************
   construct architecture , used cores, cores connections , direction , rings , multiple links\n
   exclude from implemetation code that is only for configure.\n
*******************************************************************************
    @date       created(19/jun/2018)

    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 19/jun/2018 
      -# Initial Version
******************************************************************************/

// ./img/00_summary.png
// ./img/01_detail.png

#define OFFSET(c)   (c+24)

std::string uri = "mysql://root:develop@127.0.0.1:3306";
app->dburi_ = uri;
CoreTapRx* taprx;

PGW_LOG(RTE_LOG_INFO, "architecture [%s]\n", __FILENAME__);

// core placement
app->Add(new CoreRxDistributor(0, 0, 1));
taprx = new CoreTapRx("tap_00", 11, 0);
app->Add(taprx);
app->Add(new CoreTapTx("tap_00", 9, 0, (int)taprx->GetN(KEY::OPT)));


app->Add(new CorePgwIngressDistributorWorker(5,uri.c_str(),1801,13,PGW_TUNNEL_IP,PGW_TUNNEL_IP_S,0,0));
app->Add(new CorePgwEgressDistributorWorker(7,uri.c_str(),1805,15,PGW_TUNNEL_IP,PGW_TUNNEL_IP_S,0,0));
app->Add(new CoreTx(0, 0, 3));

// connect in placed cores.

// nic:rx -> exept gtpu : tapwrite
app->Connect(1, 9, app->conf_->rx_ring_size_);
// nic:rx -> ingress 
app->Connect(1, 5, app->conf_->rx_ring_size_);
// nic:rx -> egress 
app->Connect(1, 7, app->conf_->rx_ring_size_);
// ingress -> nic:tx
app->Connect(5, 3, app->conf_->tx_ring_size_);
// ingress -> error notify : error indicate
app->Connect(5, 9, app->conf_->tx_ring_size_);
// tapread -> ingress tunnel-table : warmup
app->Connect(11, 5, app->conf_->rx_ring_size_);
// tapread -> nic:tx
app->Connect(11, 3, app->conf_->tx_ring_size_);
// tapread -> egress tunnel-table : warmup
app->Connect(11, 7, app->conf_->rx_ring_size_);
// egress -> nic:tx
app->Connect(7, 3, app->conf_->tx_ring_size_);
// egress -> error notify : error indicate
app->Connect(7, 9, app->conf_->tx_ring_size_);
//

// ------------------------
// core placement
app->Add(new CoreRxDistributor(1, 0, OFFSET(1)));
taprx = new CoreTapRx("tap_01", OFFSET(11), 0);
app->Add(taprx);
app->Add(new CoreTapTx("tap_01", OFFSET(9), 0, (int)taprx->GetN(KEY::OPT)));

app->Add(new CorePgwIngressDistributorWorker(OFFSET(5),uri.c_str(),OFFSET(1801),OFFSET(13),PGW_TUNNEL_IP,PGW_TUNNEL_IP_S,0,0));
app->Add(new CorePgwEgressDistributorWorker(OFFSET(7),uri.c_str(),OFFSET(1805),OFFSET(15),PGW_TUNNEL_IP,PGW_TUNNEL_IP_S,0,0));
app->Add(new CoreTx(1, 0, OFFSET(3)));

// connect in placed cores.

// nic:rx -> except gtpu : tapwrite
app->Connect(OFFSET(1), OFFSET(9), app->conf_->rx_ring_size_);
// nic:rx -> ingress
app->Connect(OFFSET(1), OFFSET(5), app->conf_->rx_ring_size_);
// nic:rx -> egress
app->Connect(OFFSET(1), OFFSET(7), app->conf_->rx_ring_size_);
// ingress -> nic:tx
app->Connect(OFFSET(5), OFFSET(3), app->conf_->tx_ring_size_);
// ingress -> error notify : error indicate
app->Connect(OFFSET(5), OFFSET(9), app->conf_->tx_ring_size_);
// tapread -> ingress tunnel-table : warmup
app->Connect(OFFSET(11), OFFSET(5), app->conf_->rx_ring_size_);
// tapread -> nic:tx
app->Connect(OFFSET(11), OFFSET(3), app->conf_->tx_ring_size_);
// tapread -> egress tunnel-table : warmup
app->Connect(OFFSET(11), OFFSET(7), app->conf_->rx_ring_size_);
// egress -> nic:tx
app->Connect(OFFSET(7), OFFSET(3), app->conf_->tx_ring_size_);
// egress -> error notify  : error indicate
app->Connect(OFFSET(7), OFFSET(9), app->conf_->tx_ring_size_);

// cross warmup
// tapread -> ingress tunnel-table : warmup
app->Connect(11, OFFSET(5), app->conf_->rx_ring_size_);
// tapread -> egress tunnel-table : warmup
app->Connect(11, OFFSET(7), app->conf_->rx_ring_size_);

// cross warmup
// tapread -> ingress tunnel-table : warmup
app->Connect(OFFSET(11), 5, app->conf_->rx_ring_size_);
// tapread -> egress tunnel-table : warmup
app->Connect(OFFSET(11), 7, app->conf_->rx_ring_size_);

//
app->Commit(app->conf_);
//
app->BindNic(1, 1, 1, app->conf_->nic_rx_ring_size_, app->conf_->nic_tx_ring_size_, NULL);
app->BindNic(OFFSET(1), 1, 1, app->conf_->nic_rx_ring_size_, app->conf_->nic_tx_ring_size_, NULL);


