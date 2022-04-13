
CREATE TABLE radius (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `imsi` bigint(20) NOT NULL DEFAULT 0,
  `ueipv4` varchar(64) NOT NULL DEFAULT '' COMMENT 'User Element ipv4',
  `ueipv6` varchar(64) NOT NULL DEFAULT '' COMMENT 'User Element ipv6',
  `nasipv` varchar(64) NOT NULL DEFAULT '' COMMENT 'Nas Ip Address',
  `active` bigint(20) NOT NULL DEFAULT 0 COMMENT 'active = 0/inactive != 0',
  `updated_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `imsi_nasipv` (`imsi`, `nasipv`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

INSERT INTO radius(`imsi`,`ueipv4`,`ueipv6`,`nasipv`,`active`)VALUES
 ('817000000001','10.3.155.1','','1.1.2.1',1)
,('817000000002','10.3.155.2','2:2:2:2:2:2:2:2','1.2.2.2',1)
,('817000000003','10.3.155.3','3:3:3:3:3:3:3:3','1.3.2.3',1)

;