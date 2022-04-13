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
cores.push_back(new CoreRx(4, 0, 21, TUNNEL_INGRESS));
cores.push_back(new CorePgwIngressTunnelWorker(22,uri.c_str(),15836,29,PGW_TUNNEL_IP,PGW_TUNNEL_IP_S,0,0));
cores.push_back(new CoreCounterIngressWorker(23));
cores.push_back(new CoreTx(5, 0, 24));
cores.push_back(new CoreCounterLogWorker(25, uri.c_str(), 61));

AddCores(COREID(9),cores);

cores2.push_back(new CoreRx(5, 0, 26, TUNNEL_EGRESS));
cores2.push_back(new CorePgwEgressTunnelWorker(27,uri.c_str(),15837,31,PGW_TUNNEL_IP,PGW_TUNNEL_IP_S,0,0));
cores2.push_back(new CoreCounterEgressWorker(28));
cores2.push_back(new CoreTx(4, 29));
cores2.push_back(new CoreCounterLogWorker(30, uri.c_str(), 63));

AddCores(COREID(11),cores2);

// connect in placed cores.
// nic:rx -> ingress
Connect(21, 22, conf_->rx_ring_size_);
// ingress -> counter-i
Connect(22, 23, conf_->tx_ring_size_);
// counter-i -> nic:tx
Connect(23, 24, conf_->tx_ring_size_);
// counter-i -> counter log(db)
Connect(23, 25, conf_->tx_ring_size_);

// nic:rx -> egress
Connect(26, 27, conf_->rx_ring_size_);
// egress -> counter-e
Connect(27, 28, conf_->tx_ring_size_);
// counter-e -> nic:tx
Connect(28, 29, conf_->tx_ring_size_);
// counter-e -> counter log(db)
Connect(28, 30, conf_->tx_ring_size_);

Commit(conf_);
//
BindNic(21, 1, 1, conf_->nic_rx_ring_size_, conf_->nic_tx_ring_size_, NULL);
BindNic(26, 1, 1, conf_->nic_rx_ring_size_, conf_->nic_tx_ring_size_, NULL);
