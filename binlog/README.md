# binlog slave application

+ [home](../README.md)
+ [binlog](../binlog/README.md) <<
+ [ctrl plane](../ctrlplane/README.md)
  + [delete bearer](../ctrlplane/src/cmd/README.md)
  + [proxy](../ctrlplane/src/proxy/README.md)
+ [data plane](../dataplane/README.md)
+ [tools](../tools/README.md)
  + [tools sources](../tools/src/README.md)
  + [radius](../tools/src/mod/mod_radius/README.md)
  + [diameter](../tools/src/mod/mod_diameter/README.md)
  + [sgw-tun](../tools/cfg/tools/sgw_tun/README.md)

## introduction

binlog-read is sample implementation about binlog slave application(mysql)

+ Use mysql 8.x libraries

> Please use at your own risk.


## database setup

Prepare replication master database as follows.

```
docker pull mysql
docker run --name mysql -e MYSQL_ROOT_PASSWORD=develop -d -p 3306:3306 mysql
docker exec -it mysql bash

mysql -u root -p

mysql> CREATE DATABASE mixi_pgw;
mysql> CREATE USER 'root'@'127.0.0.1' IDENTIFIED BY 'develop'
mysql> use mixi_pgw;
```

### mixi_pgw database connection

```
$ ./mysql -u root -p -h 127.0.0.1 mixi_pgw
```

### setup replication

```
mysql> GRANT ALL ON *.* TO 'root'@'%';
mysql> GRANT ALL ON *.* TO 'root'@'127.0.0.1';
mysql> GRANT REPLICATION SLAVE ON *.* TO root@'%';
mysql> GRANT REPLICATION SLAVE ON *.* TO root@'127.0.0.1';
mysql> GRANT REPLICATION CLIENT ON *.* TO root@'%';
mysql> GRANT REPLICATION CLIENT ON *.* TO root@'127.0.0.1';
```

```
mysql> SHOW MASTER STATUS;
+---------------+----------+--------------+------------------+-------------------+
| File          | Position | Binlog_Do_DB | Binlog_Ignore_DB | Executed_Gtid_Set |
+---------------+----------+--------------+------------------+-------------------+
| binlog.000002 |     2290 |              |                  |                   |
+---------------+----------+--------------+------------------+-------------------+
```

```
mysql> SHOW VARIABLES LIKE '%SERVER_ID%';
+----------------+-------+
| Variable_name  | Value |
+----------------+-------+
| server_id      | 1     |
| server_id_bits | 32    |
+----------------+-------+
```

### Simple check with Replication Client

```
$./mysqlbinlog --read-from-remote-server --host=127.0.0.1 --user=root --password --stop-never -vv --base64-output=AUTO binlog.000002
```

## compile/link


### library

```
mkdir -p ./_build
cd ./_build
cmake ../cmake/lib/
make
sudo make install
```

### sample module

```
mkdir -p ./_build
cd ./_build
cmake ../cmake/mod/
make
```
