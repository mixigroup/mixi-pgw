-- mysql --

DROP DATABASE IF EXISTS mixipgw;
CREATE DATABASE mixipgw;
use mixipgw;

/**
  [pgw] interconversion of ipv4 and teid

  + teid -> ipv4 translate -> counter(ingress)
  + ipv4 -> teid translate -> counter(egress)
  + teid -> bitrate(ul/dl) -> enq+qos/ dec

 **/

CREATE TABLE `tunnel` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `imsi` bigint(20) NOT NULL DEFAULT 0,
  `msisdn` bigint(20) NOT NULL DEFAULT 0,
  `ueipv4` varchar(64) NOT NULL DEFAULT '',
  `pgw_teid` bigint(20) NOT NULL DEFAULT 0 COMMENT 'Send Create Session Response:F-TEID',
  `pgw_gtpc_ipv` varchar(64) NOT NULL DEFAULT '' COMMENT 'Create Session Request Recieved Control plane Ipaddress',
  `pgw_gtpu_ipv` varchar(64) NOT NULL DEFAULT '' COMMENT 'P-GW gtpu ipv4',
  `dns` varchar(64) NOT NULL DEFAULT '',
  `ebi` bigint(20) NOT NULL DEFAULT 0,
  `sgw_gtpc_teid` bigint(20) NOT NULL DEFAULT 0 COMMENT 'Recieved by Create Session Request:F-TEID.',
  `sgw_gtpc_ipv` varchar(64) NOT NULL DEFAULT '' COMMENT 'Recieved by Create Session Request:F-TEID.',
  `sgw_gtpu_teid` bigint(20) NOT NULL DEFAULT 0 COMMENT 'Recieved by Create Session Request:BEARERCTX:F-TEID.',
  `sgw_gtpu_ipv` varchar(64) NOT NULL DEFAULT '' COMMENT 'Recieved by Create Session Request:BEARERCTX:F-TEID.',
  `policy` varchar(64) NOT NULL DEFAULT '',
  `active` bigint(20) NOT NULL DEFAULT 0 COMMENT 'active = 0/inactive != 0',
  `bitrate_s5` bigint(20) NOT NULL DEFAULT 0,
  `bitrate_sgi` bigint(20) NOT NULL DEFAULT 0,
  `qci` bigint(20) NOT NULL DEFAULT 0 COMMENT 'Recieved by Create Session Request: Qci.',
  `qos` bigint(20) NOT NULL DEFAULT 0,
  `teid_mask` bigint(20) NOT NULL DEFAULT 0,
  `priority` bigint(20) NOT NULL DEFAULT 0,
  `updated_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `pgw_teid` (`pgw_teid`),
  UNIQUE KEY `msisdn` (`msisdn`),
  UNIQUE KEY `imsi` (`imsi`),
  KEY `teid_mask` (`teid_mask`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE `log_gy` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `teid` bigint(20) NOT NULL,
  `ueipv4` bigint(20) NOT NULL DEFAULT 0,
  `reporter` varchar(64) NOT NULL DEFAULT '',
  `used_s5_bytes` bigint(20) NOT NULL DEFAULT 0,
  `used_sgi_bytes` bigint(20) NOT NULL DEFAULT 0,
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `teid` (`teid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE `contract` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `teid` bigint(20) NOT NULL,
  `max_traffic_month` bigint(20) NOT NULL DEFAULT 0,
  `base_traffic_s5` bigint(20) NOT NULL DEFAULT 0,
  `base_traffic_sgi` bigint(20) NOT NULL DEFAULT 0,
  `guaranteed_min_traffic_s5` bigint(20) NOT NULL DEFAULT 0,
  `guaranteed_min_traffic_sgi` bigint(20) NOT NULL DEFAULT 0,
  `updated_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `teid` (`teid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- in case update traffic(CCR(update) from gy)

TRUNCATE TABLE `tunnel`;
TRUNCATE TABLE `contract`;
TRUNCATE TABLE `log_gy`;


