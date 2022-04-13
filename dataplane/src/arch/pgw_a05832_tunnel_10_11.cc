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
cores.push_back(new CoreRx(10, 0, 51, TUNNEL_INGRESS));
cores.push_back(new CorePgwIngressTunnelWorker(52,uri.c_str(),15842,29,PGW_TUNNEL_IP,PGW_TUNNEL_IP_S,0,0));
cores.push_back(new CoreCounterIngressWorker(53));
cores.push_back(new CoreTx(11, 0, 54));
cores.push_back(new CoreCounterLogWorker(55, uri.c_str(), 61));

AddCores(COREID(21),cores);

cores2.push_back(new CoreRx(10, 0, 56, TUNNEL_EGRESS));
cores2.push_back(new CorePgwEgressTunnelWorker(57,uri.c_str(),15843,31,PGW_TUNNEL_IP,PGW_TUNNEL_IP_S,0,0));
cores2.push_back(new CoreCounterEgressWorker(58));
cores2.push_back(new CoreTx(10, 59));
cores2.push_back(new CoreCounterLogWorker(60, uri.c_str(), 63));

AddCores(COREID(23),cores2);

// connect in placed cores.
// nic:rx -> ingress
Connect(51, 52, conf_->rx_ring_size_);
// ingress -> counter-i
Connect(52, 53, conf_->tx_ring_size_);
// counter-i -> nic:tx
Connect(53, 54, conf_->tx_ring_size_);
// counter-i -> counter log(db)
Connect(53, 55, conf_->tx_ring_size_);

// nic:rx -> egress
Connect(56, 57, conf_->rx_ring_size_);
// egress -> counter-e
Connect(57, 58, conf_->tx_ring_size_);
// counter-e -> nic:tx
Connect(58, 59, conf_->tx_ring_size_);
// counter-e -> counter log(db)
Connect(58, 60, conf_->tx_ring_size_);

Commit(conf_);
//
BindNic(51, 1, 1, conf_->nic_rx_ring_size_, conf_->nic_tx_ring_size_, NULL);
BindNic(56, 1, 1, conf_->nic_rx_ring_size_, conf_->nic_tx_ring_size_, NULL);
