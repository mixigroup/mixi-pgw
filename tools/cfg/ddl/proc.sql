DROP PROCEDURE mixipgw.log_gy_to_tunnel;

DELIMITER $$

CREATE PROCEDURE mixipgw.log_gy_to_tunnel(IN __teid_s BIGINT, IN __teid_e BIGINT)
BEGIN
  DECLARE _teid BIGINT;
  DECLARE _used_s5_bytes BIGINT;
  DECLARE _used_sgi_bytes BIGINT;
  DECLARE _max_traffic_month BIGINT;
  DECLARE _guaranteed_min_traffic_s5 BIGINT;
  DECLARE _guaranteed_min_traffic_sgi BIGINT;
  DECLARE _base_traffic_s5 BIGINT;
  DECLARE _base_traffic_sgi BIGINT;

  DECLARE _done INT DEFAULT 0;
  DECLARE _cur_log CURSOR FOR
    SELECT teid,MAX(used_s5_bytes),MAX(used_sgi_bytes)
    FROM log_gy_work
    WHERE teid BETWEEN __teid_s AND __teid_e
    GROUP BY teid;
  DECLARE CONTINUE HANDLER FOR NOT FOUND SET _done = 1;
  -- preparing work regions, and move data to those
  DROP TABLE IF EXISTS log_gy_work;
  CREATE TABLE log_gy_work LIKE log_gy;
  INSERT INTO log_gy_work SELECT * FROM log_gy WHERE teid BETWEEN __teid_s AND __teid_e;
  DELETE FROM log_gy WHERE id IN (SELECT id FROM log_gy_work);

  -- start cursor loop on work table
  OPEN _cur_log;

  __FETCH: LOOP
    FETCH _cur_log INTO _teid,_used_s5_bytes, _used_sgi_bytes;
    IF _done = 1 THEN
      LEAVE __FETCH;
    END IF;
    -- exists contract
    IF EXISTS(SELECT 1 FROM contract WHERE teid = _teid) THEN
      -- contract information to variables.
      SELECT guaranteed_min_traffic_s5,guaranteed_min_traffic_sgi,base_traffic_s5,base_traffic_sgi,max_traffic_month
      INTO _guaranteed_min_traffic_s5,_guaranteed_min_traffic_sgi,_base_traffic_s5,_base_traffic_sgi,_max_traffic_month
      FROM contract
      WHERE teid = _teid;
      --
      SELECT _teid, _used_s5_bytes,  _used_sgi_bytes;
      --
      -- setup min guaranteed communication traffic as contract of communication traffic.  最小保証通信量/契約通信量 を設定
      IF (_max_traffic_month < _used_s5_bytes OR _max_traffic_month < _used_sgi_bytes) THEN
        -- exceeded limitance communication traffic in month
        UPDATE tunnel SET bitrate_s5 = _guaranteed_min_traffic_s5,bitrate_sgi = _guaranteed_min_traffic_sgi WHERE pgw_teid = _teid;
      ELSE
        -- TODO: ex. could be add logic like an over [n] GB in [m] days.
        UPDATE tunnel SET bitrate_s5 = _base_traffic_s5,bitrate_sgi = _base_traffic_sgi WHERE pgw_teid = _teid;
      END IF;
    END IF;
  END LOOP;
  CLOSE _cur_log;
END;
$$
DELIMITER ;
