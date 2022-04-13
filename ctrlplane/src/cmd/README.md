# mixi_pgw_ctrl_plane_delete_bearer


+ [home](../../../README.md)
+ [binlog](../../../binlog/README.md)
+ [ctrl plane](../../../ctrlplane/README.md)
  + [delete bearer](../../../ctrlplane/src/cmd/README.md) <<
  + [proxy](../../../ctrlplane/src/proxy/README.md)
+ [data plane](../../../dataplane/README.md)
+ [tools](../../../tools/README.md)
  + [tools sources](../../../tools/src/README.md)
  + [radius](../../../tools/src/mod/mod_radius/README.md)
  + [diameter](../../../tools/src/mod/mod_diameter/README.md)
  + [sgw-tun](../../../tools/cfg/tools/sgw_tun/README.md)


+ Request DELETE BEARER

## Dependency

+ pthread
+ libevent
+ libmysqlclient

```
apt-get install libmysqlclient-dev
apt-get install libevent-dev
apt-get install libgtest-dev
```

## Compile/Link

```
mkdir ./build/
cd ./build
cmake ..
make
```

## Execute

```
mixi_pgw_ctrl_plane_delete_bearer -i <1234567890> -t 10
```
