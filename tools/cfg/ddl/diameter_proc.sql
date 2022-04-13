DROP PROCEDURE mixipgw.policy_debug_record;

DELIMITER $$

CREATE PROCEDURE mixipgw.policy_debug_record(IN __start BIGINT, IN __end BIGINT)
BEGIN
  DECLARE _counter BIGINT DEFAULT __start;
  -- cleanup first
  TRUNCATE TABLE policy;

  __INSERT: LOOP
    IF _counter >= __end THEN
      LEAVE __INSERT;
    END IF;
    -- regist data to policy table
    INSERT INTO `policy`(
      ueipv4,vlanid, active
    )VALUES(
      CONCAT('10.0.',ROUND(_counter/255),'.',(_counter%255)),
      _counter,
      1
    );
    SET _counter = _counter + 1;
  END LOOP;
END;
$$
DELIMITER ;




DELIMITER $$

DROP PROCEDURE IF EXISTS `rebuild_partition`;
$$

CREATE PROCEDURE `rebuild_partition` (_trgt_schema VARCHAR(64),_trgt_tbl VARCHAR(64))
BEGIN
    DECLARE __drop_target_pname   VARCHAR(32) DEFAULT '';
    DECLARE __insert_target_pname VARCHAR(32) DEFAULT '';
    DECLARE __insert_target_pname_fmt VARCHAR(32) DEFAULT '';
    DECLARE __current_first_pname VARCHAR(32) DEFAULT '';
    DECLARE __current_last_pname VARCHAR(32) DEFAULT '';
    DECLARE __sql VARCHAR(1024) DEFAULT '';

    /* preparing result table */
    DROP TEMPORARY TABLE IF EXISTS `logt`;
    CREATE TEMPORARY TABLE `logt` (
        `message`            VARCHAR(128),
        `created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
    );
    /* remove: > 3 days ago. partition name */
    SELECT DATE_FORMAT(DATE_ADD(NOW(),INTERVAL -2 DAY),'p%Y%m%d') INTO __drop_target_pname;
    /* remove: > 2 days ago. partition name */
    SELECT DATE_FORMAT(DATE_ADD(NOW(),INTERVAL  1 DAY),'p%Y%m%d')  INTO __insert_target_pname;
    SELECT DATE_FORMAT(DATE_ADD(NOW(),INTERVAL  2 DAY),'%Y-%m-%d') INTO __insert_target_pname_fmt;

    /* partition is exists. */
    IF EXISTS(SELECT (1) FROM INFORMATION_SCHEMA.PARTITIONS WHERE TABLE_SCHEMA = _trgt_schema AND TABLE_NAME = _trgt_tbl) THEN
        /* remove when oldest partition name equal remove target partition name */
        SELECT PARTITION_NAME INTO __current_first_pname
        FROM INFORMATION_SCHEMA.PARTITIONS
        WHERE TABLE_SCHEMA = _trgt_schema AND TABLE_NAME = _trgt_tbl
        ORDER BY PARTITION_NAME ASC
        LIMIT 1;
        /*  */
        INSERT INTO logt(message)VALUES(CONCAT('target: ',__drop_target_pname, '/current: ', __current_first_pname));
        /*  */
        IF __current_first_pname = __drop_target_pname THEN
            SET __sql = CONCAT('ALTER TABLE ',_trgt_tbl,' DROP PARTITION ',__drop_target_pname);
            /* */
            SET @drop_partition_sql = __sql;
            /* */
            PREPARE stmt_drop FROM @drop_partition_sql;
            EXECUTE stmt_drop;
            DEALLOCATE PREPARE stmt_drop;
            /* */
            INSERT INTO logt(message)VALUES(CONCAT('drop: ',__drop_target_pname));
        END IF;
        /* insert when next month of lastest partition name except MAXVALUE equal insert target partition name */
        SELECT PARTITION_NAME INTO __current_last_pname
        FROM INFORMATION_SCHEMA.PARTITIONS
        WHERE TABLE_SCHEMA = _trgt_schema AND TABLE_NAME = _trgt_tbl AND PARTITION_NAME <> 'pMAX'
        ORDER BY PARTITION_NAME DESC
        LIMIT 1;
        /* */
        SELECT DATE_FORMAT(DATE_ADD(CAST(CONCAT(SUBSTR(__current_last_pname,2),'000000') AS DATETIME),INTERVAL 1 DAY),'p%Y%m%d')
        INTO __current_last_pname;
        /* */
        IF __current_last_pname = __insert_target_pname THEN
            SET __sql = CONCAT('ALTER TABLE ',_trgt_tbl,' REORGANIZE PARTITION pmax INTO(',
                               'PARTITION ',__insert_target_pname,' ',
                               'VALUES LESS THAN (\'',__insert_target_pname_fmt,'\'),',
                               'PARTITION pmax VALUES LESS THAN MAXVALUE)');
            SET @insert_partition_sql = __sql;
            /* */
            PREPARE stmt_insert FROM @insert_partition_sql;
            EXECUTE stmt_insert;
            DEALLOCATE PREPARE stmt_insert;
            /* */
            INSERT INTO logt(message)VALUES(CONCAT('make: ',__insert_target_pname));
        END IF;
    END IF;
    SELECT * FROM `logt`;
END
$$
DELIMITER ;

call rebuild_partition('mixipgw','log_diameter_gy');


/*
## crontab -e
### run partition carry-over  every day AM03:00
0 3 * * * mysql -u root -ppassword mixipgw -e "CALL rebuild_partition('mixipgw','log_diameter_gy')"
*/