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
unsigned int dummy_coreid = 1;

// core placement
cores.push_back(new CoreRx(0, 0, 1, TUNNEL_INGRESS));
cores.push_back(new CorePgwIngressTunnelWorker(2,uri.c_str(),1801,48,PGW_TUNNEL_IP,PGW_TUNNEL_IP_S,0,0));
cores.push_back(new CoreCounterIngressWorker(3));
cores.push_back(new CoreTx(1, 0, 4));
cores.push_back(new CoreCounterLogWorker(5, uri.c_str(), 49));

AddCores(COREID(4),cores);

cores2.push_back(new CoreRx(1, 0, 6, TUNNEL_EGRESS));
cores2.push_back(new CorePgwEgressTunnelWorker(7,uri.c_str(),1802,50,PGW_TUNNEL_IP,PGW_TUNNEL_IP_S,0,0));
cores2.push_back(new CoreCounterEgressWorker(8));
cores2.push_back(new CoreTx(0, 9));
cores2.push_back(new CoreCounterLogWorker(10, uri.c_str(), 51));

AddCores(COREID(6),cores2);

// connect in placed cores.
// nic:rx -> ingress
Connect(1, 2, conf_->rx_ring_size_);
// ingress -> counter-i
Connect(2, 3, conf_->tx_ring_size_);
// counter-i -> nic:tx
Connect(3, 4, conf_->tx_ring_size_);
// counter-i -> counter log(db)
Connect(3, 5, conf_->tx_ring_size_);

// nic:rx -> egress
Connect(6, 7, conf_->rx_ring_size_);
// egress -> counter-e
Connect(7, 8, conf_->tx_ring_size_);
// counter-e -> nic:tx
Connect(8, 9, conf_->tx_ring_size_);
// counter-e -> counter log(db)
Connect(8,10, conf_->tx_ring_size_);

Commit(conf_);
//
BindNic(1, 1, 1, conf_->nic_rx_ring_size_, conf_->nic_tx_ring_size_, NULL);
BindNic(6, 1, 1, conf_->nic_rx_ring_size_, conf_->nic_tx_ring_size_, NULL);
