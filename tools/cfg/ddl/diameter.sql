DROP TABLE IF EXISTS `diameter`;

CREATE TABLE diameter (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `imsi` bigint(20) NOT NULL DEFAULT 0,
  `ueipv4` varchar(64) NOT NULL DEFAULT '' COMMENT 'User Element ipv4',
  `ueipv6` varchar(64) NOT NULL DEFAULT '' COMMENT 'User Element ipv6',
  `nasipv` varchar(64) NOT NULL DEFAULT '' COMMENT 'Nas Ip Address',
  `policy` varchar(64) NOT NULL DEFAULT '' COMMENT 'policy name',
  `ingress` bigint(20) NOT NULL DEFAULT 0 COMMENT 'ingress granted bitrate',
  `egress` bigint(20)  NOT NULL DEFAULT 0 COMMENT 'egress granted bitrate',
  `threshold` bigint(20) NOT NULL DEFAULT 0 COMMENT 'contains a threshold value in octets',
  `active` bigint(20) NOT NULL DEFAULT 0 COMMENT 'active = 0/inactive != 0',
  `updated_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `imsi_nasipv` (`imsi`, `nasipv`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

INSERT INTO diameter(`imsi`,`ueipv4`,`ueipv6`,`nasipv`,`policy`,ingress,egress,`active`)VALUES
 ('817000000001','10.3.155.1','','1.1.2.1','default',012,567,1)
,('817000000002','10.3.155.2','2:2:2:2:2:2:2:2','1.2.2.2','default',123,234,1)
,('817000000003','10.3.155.3','3:3:3:3:3:3:3:3','1.3.2.3','default',234,345,1)
;

DROP TABLE IF EXISTS `log_diameter_gy`;

CREATE TABLE `log_diameter_gy` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `imsi` bigint(20) NOT NULL,
  `ueipv4` bigint(20) NOT NULL DEFAULT 0,
  `reporter` varchar(64) NOT NULL DEFAULT '',
  `used_s5_bytes` bigint(20) NOT NULL DEFAULT 0,
  `used_sgi_bytes` bigint(20) NOT NULL DEFAULT 0,
  `created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`, `created_at`),
  KEY `imsi` (`imsi`)
) ENGINE=InnoDB
PARTITION BY RANGE COLUMNS(created_at) PARTITIONS 4(
PARTITION p20170823 VALUES LESS THAN ('2017-08-24'),
PARTITION p20170824 VALUES LESS THAN ('2017-08-25'),
PARTITION p20170825 VALUES LESS THAN ('2017-08-26'),
PARTITION pMAX VALUES LESS THAN (MAXVALUE));

