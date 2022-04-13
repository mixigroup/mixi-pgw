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
cores.push_back(new CoreRx(6, 0, 31, TUNNEL_INGRESS));
cores.push_back(new CorePgwIngressTunnelWorker(32,uri.c_str(),15838,29,PGW_TUNNEL_IP,PGW_TUNNEL_IP_S,0,0));
cores.push_back(new CoreCounterIngressWorker(33));
cores.push_back(new CoreTx(7, 0, 34));
cores.push_back(new CoreCounterLogWorker(35, uri.c_str(), 61));

AddCores(COREID(13),cores);

cores2.push_back(new CoreRx(7, 0, 36, TUNNEL_EGRESS));
cores2.push_back(new CorePgwEgressTunnelWorker(37,uri.c_str(),15839,31,PGW_TUNNEL_IP,PGW_TUNNEL_IP_S,0,0));
cores2.push_back(new CoreCounterEgressWorker(38));
cores2.push_back(new CoreTx(6, 39));
cores2.push_back(new CoreCounterLogWorker(40, uri.c_str(), 63));

AddCores(COREID(15),cores2);

// connect in placed cores.
// nic:rx -> ingress
Connect(31, 32, conf_->rx_ring_size_);
// ingress -> counter-i
Connect(32, 33, conf_->tx_ring_size_);
// counter-i -> nic:tx
Connect(33, 34, conf_->tx_ring_size_);
// counter-i -> counter log(db)
Connect(33, 35, conf_->tx_ring_size_);

// nic:rx -> egress
Connect(36, 37, conf_->rx_ring_size_);
// egress -> counter-e
Connect(37, 38, conf_->tx_ring_size_);
// counter-e -> nic:tx
Connect(38, 39, conf_->tx_ring_size_);
// counter-e -> counter log(db)
Connect(38, 40, conf_->tx_ring_size_);

Commit(conf_);
//
BindNic(31, 1, 1, conf_->nic_rx_ring_size_, conf_->nic_tx_ring_size_, NULL);
BindNic(36, 1, 1, conf_->nic_rx_ring_size_, conf_->nic_tx_ring_size_, NULL);
