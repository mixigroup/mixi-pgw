CREATE TABLE `proxy` (
  `id` BIGINT NOT NULL AUTO_INCREMENT,
  `pgw_group_id` BIGINT NOT NULL COMMENT 'Group enum,0:echo/1:dev/2:qa/3:company/4:user(MNO[0])/5:user(MNO[1])/6:user(MNO[2])',
  `dst_ip` VARCHAR(64) NOT NULL COMMENT 'route(ip addr)',
  `updated_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'last updated',
  PRIMARY KEY (`id`)
) ENGINE=INNODB AUTO_INCREMENT=1 DEFAULT CHARSET=latin1;
