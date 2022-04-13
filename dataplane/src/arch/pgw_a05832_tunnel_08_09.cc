/******************************************************************************/
/*! @addtogroup mixi_pgw_data_plane
    @file       arch/pgw_a05532_1gx_tunnel.cc
    @brief      PGW  - 1Gnic x tunenl 2port: for a05532
*******************************************************************************
   construct architecture , used cores, cores connections , direction , rings , multiple links\n
   exclude from implemetation code that is only for configure.\n
*******************************************************************************
    @date       created(26/apr/2018)

    @par        Revision
    0.0.0.1
    @par        Copyright
    2017 mixi. Released Under the MIT license.
*******************************************************************************
    @par        History
    - 26/apr/2018 
      -# Initial Version
******************************************************************************/

std::string uri = "mysql://root:develop@10.4.100.200:3306";
dburi_ = uri;

PGW_LOG(RTE_LOG_INFO, "architecture [%s]\n", __FILENAME__);

VCores cores,cores2;

// core placement
cores.push_back(new CoreRx(8, 0, 41, TUNNEL_INGRESS));
cores.push_back(new CorePgwIngressTunnelWorker(42,uri.c_str(),15840,29,PGW_TUNNEL_IP,PGW_TUNNEL_IP_S,0,0));
cores.push_back(new CoreCounterIngressWorker(43));
cores.push_back(new CoreTx(9, 0, 44));
cores.push_back(new CoreCounterLogWorker(45, uri.c_str(), 61));

AddCores(COREID(17),cores);

cores2.push_back(new CoreRx(9, 0, 46, TUNNEL_EGRESS));
cores2.push_back(new CorePgwEgressTunnelWorker(47,uri.c_str(),15841,31,PGW_TUNNEL_IP,PGW_TUNNEL_IP_S,0,0));
cores2.push_back(new CoreCounterEgressWorker(48));
cores2.push_back(new CoreTx(8, 49));
cores2.push_back(new CoreCounterLogWorker(50, uri.c_str(), 63));

AddCores(COREID(19),cores2);

// connect in placed cores.
// nic:rx -> ingress
Connect(41, 42, conf_->rx_ring_size_);
// ingress -> counter-i
Connect(42, 43, conf_->tx_ring_size_);
// counter-i -> nic:tx
Connect(43, 44, conf_->tx_ring_size_);
// counter-i -> counter log(db)
Connect(43, 45, conf_->tx_ring_size_);

// nic:rx -> egress
Connect(46, 47, conf_->rx_ring_size_);
// egress -> counter-e
Connect(47, 48, conf_->tx_ring_size_);
// counter-e -> nic:tx
Connect(48, 49, conf_->tx_ring_size_);
// counter-e -> counter log(db)
Connect(48, 50, conf_->tx_ring_size_);

Commit(conf_);
//
BindNic(41, 1, 1, conf_->nic_rx_ring_size_, conf_->nic_tx_ring_size_, NULL);
BindNic(46, 1, 1, conf_->nic_rx_ring_size_, conf_->nic_tx_ring_size_, NULL);
