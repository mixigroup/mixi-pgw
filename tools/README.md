# mixipgw-tools

Authentication module, external module, and tools used by mixi-pgw(or typical PGW)

+ [home](../README.md)
+ [binlog](../binlog/README.md)
+ [ctrl plane](../ctrlplane/README.md)
  + [delete bearer](../ctrlplane/src/cmd/README.md)
  + [proxy](../ctrlplane/src/proxy/README.md)
+ [data plane](../dataplane/README.md)
+ [tools](../tools/README.md) <<
  + [tools sources](../tools/src/README.md)
  + [radius](../tools/src/mod/mod_radius/README.md)
  + [diameter](../tools/src/mod/mod_diameter/README.md)
  + [sgw-tun](../tools/cfg/tools/sgw_tun/README.md)


## directory structure layout

```
├── cfg                     // config
│   ├── ddl                 //      database defined lunguage,*.sql
│   └── tools               //      test scripts
├── inc                     // include files
│   ├── lib                 //      main library headers
│   ├── lib_db              //      database library headers
│   ├── lib_lookup          //      lockfree table lookup library headers
│   └── lib_srv             //      udp server library headers
├── src                     // source files
│   ├── lib                 //      main library sources
│   ├── lib_db              //      database library sources
│   ├── lib_lookup          //      lockfree table lookup library sources
│   ├── lib_srv             //      udp server library sources
│   └── mod                 //      modules/process
│       ├── mod_sgw         //          sgw simulater
│       ├── mod_diameter    //          diameter server
│       └── mod_radius      //          radius server
└── test                    // test sources
    └── strs
```

## compile/link

### ubuntu

```
mkdir -p ./_build
cd ./_build
cmake ..
make
```

```
$ uname -r
4.15.0-175-generic
```

```
$ cat /etc/lsb-release 
DISTRIB_ID=Ubuntu
DISTRIB_RELEASE=18.04
DISTRIB_CODENAME=bionic
DISTRIB_DESCRIPTION="Ubuntu 18.04.6 LTS"
```


## dependencies

```
apt update
apt install -y pkg-config
apt install -y cmake 
apt install -y g++
apt install -y libboost-all-dev
apt install -y liblog4cpp5-dev
apt install -y libgtest-dev
apt install -y ncurses-dev
apt install -y libevent-dev
apt install -y libssl-dev
```

### setup netmap

```
cd ./mixi-pgw/deps
git clone git@github.com:luigirizzo/netmap.git
cd netmap
./configure --no-drivers
```

```
**********************************  NOTE   **********************************
*** Running some preliminary tests to customize the build environment.
*****************************************************************************
**********************************  NOTE   **********************************
*** Now running compile tests to adapt the code to your
*** kernel version. Please wait.
*****************************************************************************
kernel directory            /lib/modules/4.15.0-175-generic/build
                            [/usr/src/linux-headers-4.15.0-175-generic]
kernel sources              /lib/modules/4.15.0-175-generic/build
                            [/usr/src/linux-headers-4.15.0-175-generic]
linux version               40f12  [4.15.18]
module file                 netmap.ko

subsystems                  null ptnetmap generic monitor pipe vale
apps                        dedup vale-ctl nmreplay tlem lb bridge pkt-gen
native drivers  
```

```
make
make install
```

# setup google test

```
git clone https://github.com/google/googletest.git
cd googletest
mkdir ./build
cd build
cmake ../
make
sudo make install
```
