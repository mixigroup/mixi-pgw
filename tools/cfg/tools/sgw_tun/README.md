sgw_tun
====

+ [home](../../../../README.md)
+ [binlog](../../../../binlog/README.md)
+ [ctrl plane](../../../../ctrlplane/README.md)
  + [delete bearer](../../../../ctrlplane/src/cmd/README.md)
  + [proxy](../../../../ctrlplane/src/proxy/README.md)
+ [data plane](../../../../dataplane/README.md)
+ [tools](../../../../tools/README.md)
  + [tools sources](../../../../tools/src/README.md)
  + [radius](../../../../tools/src/mod/mod_radius/README.md)
  + [diameter](../../../../tools/src/mod/mod_diameter/README.md)
  + [sgw-tun](../../../../tools/cfg/tools/sgw_tun/README.md) <<

sgw simulator on tun interface


## Description

Simulate sgw+mme functions.

Tun interface is created as "sim0" at startup.
Implement tun -> gtpu : encap, gtpu -> tun: decap process between sim0 and NIC.


## Features

### Control Plane

+ Call CreateSession at startup
  + Save teid and peer data plane information from CreateSession Response
  
### Data Plane

+ Encapsulates IP packets arrived at sim0:tun interface into GTPU and forwards those to PGW data plane.
+ Decapsulates IP packets arrived from PGW data plane to GTPU and forwards those to sim0:tun interface.

## Dependencies

Use libmixipgw_tools_misc.so(gtpc-v2 parse) function
  of parent project.

## Install

### ubuntu

```
mkdir ./build
cd ./build
cmake ..
make

```

## exsample

### start program, tun <-> udp with gtpu
```
sudo ./sgw_tun sim0 192.168.15.202 2152  16 110.44.181.16 310141234567890 818012345678
```

### check sim0 interface
```
sim0      Link encap:UNSPEC  HWaddr 00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00  
          inet addr:127.0.0.3  P-t-P:127.0.0.3  Mask:255.255.255.0
          UP POINTOPOINT RUNNING MULTICAST  MTU:1500  Metric:1
          RX packets:0 errors:0 dropped:0 overruns:0 frame:0
          TX packets:0 errors:0 dropped:0 overruns:0 carrier:0
          collisions:0 txqueuelen:500 
          RX bytes:0 (0.0 B)  TX bytes:0 (0.0 B)
```
### run in namespace [ue]
```
### generate namaspace
$ sudo ip netns add ue

### setup tun device to generated ns
sudo ip link set netns ue dev sim0
sudo ip netns exec ue bash

### config on ue context.
ip link set up lo
ip link set up dev sim0
ip addr add 127.0.0.3/24 dev sim0
ip route add 0.0.0.0/0 dev sim0

### communicate with internet.
ping 8.8.8.8 

```
