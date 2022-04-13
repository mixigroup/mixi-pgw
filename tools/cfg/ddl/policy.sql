
CREATE TABLE `policy` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `ueipv4` varchar(64) NOT NULL DEFAULT '' COMMENT 'User Element ipv4',
  `vlanid` bigint(20) NOT NULL DEFAULT 0 COMMENT 'Vlan Identifier',
  `active` bigint(20) NOT NULL DEFAULT 0 COMMENT 'active = 0/inactive != 0',
  `updated_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `ueipv4_vlanid` (`ueipv4`, `vlanid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;