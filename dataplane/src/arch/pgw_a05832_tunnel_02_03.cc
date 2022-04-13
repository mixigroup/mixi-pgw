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
cores.push_back(new CoreRx(3, 0, 11, TUNNEL_INGRESS));
cores.push_back(new CorePgwIngressTunnelWorker(12,uri.c_str(),15834,28,PGW_TUNNEL_IP,PGW_TUNNEL_IP_S,0,0));
cores.push_back(new CoreCounterIngressWorker(13));
cores.push_back(new CoreTx(2, 0, 14));
cores.push_back(new CoreCounterLogWorker(15, uri.c_str(), 60));

AddCores(COREID(12),cores);

cores2.push_back(new CoreRx(2, 0, 16, TUNNEL_EGRESS));
cores2.push_back(new CorePgwEgressTunnelWorker(17,uri.c_str(),15835,30,PGW_TUNNEL_IP,PGW_TUNNEL_IP_S,0,0));
cores2.push_back(new CoreCounterEgressWorker(18));
cores2.push_back(new CoreTx(3, 19));
cores2.push_back(new CoreCounterLogWorker(20, uri.c_str(), 62));

AddCores(COREID(14),cores2);

// connect in placed cores.
// nic:rx -> ingress
Connect(11, 12, conf_->rx_ring_size_);
// ingress -> counter-i
Connect(12, 13, conf_->tx_ring_size_);
// counter-i -> nic:tx
Connect(13, 14, conf_->tx_ring_size_);
// counter-i -> counter log(db)
Connect(13, 15, conf_->tx_ring_size_);

// nic:rx -> egress
Connect(16, 17, conf_->rx_ring_size_);
// egress -> counter-e
Connect(17, 18, conf_->tx_ring_size_);
// counter-e -> nic:tx
Connect(18, 19, conf_->tx_ring_size_);
// counter-e -> counter log(db)
Connect(18, 20, conf_->tx_ring_size_);

Commit(conf_);
//
BindNic(11, 1, 1, conf_->nic_rx_ring_size_, conf_->nic_tx_ring_size_, NULL);
BindNic(16, 1, 1, conf_->nic_rx_ring_size_, conf_->nic_tx_ring_size_, NULL);
