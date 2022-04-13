
# Source File Directory

+ [home](../../README.md)
+ [binlog](../../binlog/README.md)
+ [ctrl plane](../../ctrlplane/README.md)
  + [delete bearer](../../ctrlplane/src/cmd/README.md)
  + [proxy](../../ctrlplane/src/proxy/README.md)
+ [data plane](../../dataplane/README.md)
+ [tools](../../tools/README.md)
  + [tools sources](../../tools/src/README.md) <<
  + [radius](../../tools/src/mod/mod_radius/README.md)
  + [diameter](../../tools/src/mod/mod_diameter/README.md)
  + [sgw-tun](../../tools/cfg/tools/sgw_tun/README.md)

## libraries


| |file| note||
|:--|:--|:--|:--|
|1|arch_interface.cc| interface of architecture| - |
|2|buffer.cc|heap buffer| - |
|3|filter_container.cc|gtp-u En/De.capsultion.| - |
|4|logger.cc|wrapper for log4cpp||
|5|misc.cc|utitility function group.|-|
|6|module.cc|instanciate per module||
|7|pktheader.cc| container of packet parser|  |
|8|process.cc| parameters for process| |

### architecture


| |file| note||
|:--|:--|:--|:--|
|1|netmap.cc|use netmap (determined at compile time)| - |
|2|osxsim.cc|used for debug(osx)(determined at compile time)| - |

### filters

| |file| note||
|:--|:--|:--|:--|
|1|arp.cc|arp protocol | - |
|2|bfd.cc|bfd protocol | - |
|3|gtpu_counter_ingress.cc|gtpu counter(ingress)| - |
|4|gtpu_counter_egress.cc|gtpu counter(egress)| - |
|5|gtpu_decap.cc|gtpu decapsulate| - |
|6|gtpu_encap.cc|gtpu encapsulate| - |
|7|gtpu_echo.cc|gtpu echo| - |
|8|icmp.cc|icmp echo| - |
|9|xepc_ctrl.cc| system internal protocol| - |

### packets

GTPC−V２ packet generator, parsers

| |file| note||
|:--|:--|:--|:--|
|1|gtpc.cc|GTPC-V2 packet base | - |
|x|v2_xxxx.cc|GTPC-V2 items| - |


### test

test for libmixipgw_tools_misc.so

## libmixipgw_tools_db

database access wrapper(mysql)

| |file| note||
|:--|:--|:--|:--|
|1|bind.cc| bind variables | - |
|2|cfg.cc|config| - |
|3|con.cc|database connection| - |
|4|rec.cc|result recordset||
|5|stmt.cc|statement(prepared statement for mysql)|-|


## libmixipgw_tools_lookup

fast dual link buffer

| |file| note||
|:--|:--|:--|:--|
|1|dual_bufferd_lookup_table.cc| dummy instance | - |
|2|dual_bufferd_lookup_table.hh| template implemented | - |


## libmixipgw_tools_srv

udp server implement(via host protocol stack: no-netmap)

| |file| note||
|:--|:--|:--|:--|
|1|bfd.cc| bfd server | - |
|2|con.cc| 1 udp connection | - |
|3|http_client.cc| http client| - |
|4|server.cc| udp server | - |


## mod

### mixi-pgw-diameter

Diameter

### mixi-pgw-radius

Radius authenticate

### mixi-pgw-sgw

SGW Simulator
