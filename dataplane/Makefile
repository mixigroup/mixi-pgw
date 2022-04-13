SRCDIR = src
TOOLDIR = tools
TSTDIR = test
INCDIR = inc
MNTDIR = sf_mixi_pgw_data_plane
UNAME_S := $(shell uname -s)

DPDKPATH=$(RTE_SDK)/$(RTE_TARGET)
CXXFLAGS = -I$(DPDKPATH) -include $(DPDKPATH)/rte_config.h -mssse3
CXXFLAGS += -D__DEBUG__ -I../inc/
CXXFLAGS += -DDEBUG=true -DNDEBUG=1 -I../inc/
CXXFLAGS += -I./$(INCDIR)/
CXXFLAGS += -std=c++17
CXXFLAGS += -Wno-register

ifdef DEBUG
CXXFLAGS += -O0 -g -D__DEBUG__ 
else
CXXFLAGS += -O2 
endif
##CXXFLAGS += -fPIE

ifdef PCAP_DEBUG
CXXFLAGS += -D__PCAP_DEBUG__
endif
ifdef PCAP_DEBUG_ENCAP
CXXFLAGS += -D__PCAP_DEBUG_ENCAP__
endif

ifdef USE_SPINLOCK
CXXFLAGS += -D__USE_SPINLOCK__
endif

ifdef CANNOT_USE_GRE
CXXFLAGS += -D__CANNOT_USE_GRE__
endif

ifdef NODAEMON
CXXFLAGS += -D__NODAEMON__
endif


PDIR=$(shell pwd)
MYSQL_SRC_DIR=$(PDIR)/../deps/mysql-8.0.28/
ifeq ($(ARCH),1)
else ifeq ($(ARCH),9)
else
	CXXFLAGS += -I$(PDIR)/../deps/mysql-8.0.28/_build/include/
	CXXFLAGS += -I$(PDIR)/../deps/mysql-8.0.28/include
	CXXFLAGS += -I$(PDIR)/../deps/mysql-8.0.28/libbinlogevents/include
	CXXFLAGS += -I$(PDIR)/../deps/mysql-8.0.28/sql
	CXXFLAGS += -I$(PDIR)/../deps/mysql-8.0.28/
	STATICLIBS += $(PDIR)/../deps/mysql-8.0.28/_build/libbinlogevents/lib/libbinlogevents.a
	STATICLIBS += $(PDIR)/../deps/mysql-8.0.28/_build/archive_output_directory/libjson_binlog_static.a
	STATICLIBS += $(PDIR)/../deps/mysql-8.0.28/_build/archive_output_directory/libmysqlclient.a
	STATICLIBS += $(PDIR)/../deps/mysql-8.0.28/_build/archive_output_directory/libmysys.a
	STATICLIBS += $(PDIR)/../deps/mysql-8.0.28/_build/archive_output_directory/libkeyring_common.a
	BINLOG_SRCS += ${MYSQL_SRC_DIR}/sql/log_event.cc
	BINLOG_SRCS += ${MYSQL_SRC_DIR}/sql/rpl_utility.cc
	BINLOG_SRCS += ${MYSQL_SRC_DIR}/sql/rpl_gtid_sid_map.cc
	BINLOG_SRCS += ${MYSQL_SRC_DIR}/sql/rpl_gtid_misc.cc
	BINLOG_SRCS += ${MYSQL_SRC_DIR}/sql/rpl_gtid_set.cc
	BINLOG_SRCS += ${MYSQL_SRC_DIR}/sql/rpl_gtid_specification.cc
	BINLOG_SRCS += ${MYSQL_SRC_DIR}/sql/rpl_tblmap.cc
	BINLOG_SRCS += ${MYSQL_SRC_DIR}/sql/basic_istream.cc
	BINLOG_SRCS += ${MYSQL_SRC_DIR}/sql/binlog_istream.cc
	BINLOG_SRCS += ${MYSQL_SRC_DIR}/sql/binlog_reader.cc
	BINLOG_SRCS += ${MYSQL_SRC_DIR}/sql/stream_cipher.cc
	BINLOG_SRCS += ${MYSQL_SRC_DIR}/sql/rpl_log_encryption.cc
endif

MOD = mixi-pgw-data-plane
ifeq ($(ARCH),0)
	MOD = mixi-pgw-data-plane_encap_vbox
else ifeq ($(ARCH),1)
	MOD = mixi-pgw-data-plane_encap_a05533
else ifeq ($(ARCH),2)
	MOD = mixi-pgw-data-plane_counting_a05533
else ifeq ($(ARCH),6)
	MOD = mixi-pgw-data-plane_dist_ingress_a05536_1port
else ifeq ($(ARCH),7)
	MOD = mixi-pgw-data-plane_a05404_1port
else ifeq ($(ARCH),8)
	MOD = mixi-pgw-data-plane_a05404_2port
else ifeq ($(ARCH),9)
	MOD = mixi-pgw-data-plane_tunnel_a05406
else ifeq ($(ARCH),10)
	MOD = mixi-pgw-data-plane_a05404_2port_x_warm
else ifeq ($(ARCH),12)
	MOD = mixi-pgw-data-plane_counting_a05534_direct_db
else ifeq ($(ARCH),13)
	MOD = mixi-pgw-data-plane_counting_a05411_direct_db
else ifeq ($(ARCH),14)
	MOD = mixi-pgw-data-plane_counting_a05411_multi_worker
else ifeq ($(ARCH),15)
	MOD = mixi-pgw-data-plane_a05532_1gx_tunnel
else ifeq ($(ARCH),16)
	MOD = mixi-pgw-data-plane_a05832_tunnel_00_01
else ifeq ($(ARCH),17)
	MOD = mixi-pgw-data-plane_a05832_tunnel_02_03
else ifeq ($(ARCH),18)
	MOD = mixi-pgw-data-plane_a05832_tunnel_04_05 
else ifeq ($(ARCH),19)
	MOD = mixi-pgw-data-plane_a05832_tunnel_06_07
else ifeq ($(ARCH),20)
	MOD = mixi-pgw-data-plane_a05832_tunnel_08_09
else ifeq ($(ARCH),21)
	MOD = mixi-pgw-data-plane_a05832_tunnel_10_11
endif
CXXFLAGS += -DARCH=$(ARCH)

LIBS = \
    -static-libgcc -m64 -pthread -march=native \
    -Wl,--no-as-needed \
    -L$(DPDKPATH)/lib \
    -lpthread -ldl -lrt -lm -lpcap -lnuma -lssl -lcrypto \
    $(STATICLIBS) \
    -Wl,--whole-archive \
    -Wl,--start-group \
    -ldpdk \
    -Wl,--end-group \
    -Wl,--no-whole-archive

SRCS =  $(SRCDIR)/entry.cc
ifeq ($(ARCH),1)
else ifeq ($(ARCH),9)
else ifeq ($(ARCH),12)
	SRCS += $(SRCDIR)/core/core_counter_log_worker.cc
else ifeq ($(ARCH),13)
	SRCS += $(SRCDIR)/core/core_counter_log_worker.cc
else ifeq ($(ARCH),14)
	SRCS += $(SRCDIR)/core/core_counter_log_worker.cc
else ifeq ($(ARCH),15)
	SRCS += $(SRCDIR)/core/core_counter_ingress_worker.cc
	SRCS += $(SRCDIR)/core/core_counter_egress_worker.cc
	SRCS += $(SRCDIR)/core/core_counter_log_worker.cc
	SRCS += $(SRCDIR)/app/app_v2.cc
	SRCS += $(SRCDIR)/core/core_pgw_base_worker.cc
	SRCS += $(SRCDIR)/core/core_pgw_ingress_tunnel_worker.cc
	SRCS += $(SRCDIR)/core/core_pgw_egress_tunnel_worker.cc
	SRCS += $(BINLOG_SRCS)

else ifeq ($(ARCH),16)
	SRCS += $(SRCDIR)/core/core_counter_ingress_worker.cc
	SRCS += $(SRCDIR)/core/core_counter_egress_worker.cc
	SRCS += $(SRCDIR)/core/core_counter_log_worker.cc
	SRCS += $(SRCDIR)/app/app_v2.cc
	SRCS += $(SRCDIR)/core/core_pgw_base_worker.cc
	SRCS += $(SRCDIR)/core/core_pgw_ingress_tunnel_worker.cc
	SRCS += $(SRCDIR)/core/core_pgw_egress_tunnel_worker.cc
else ifeq ($(ARCH),17)
	SRCS += $(SRCDIR)/core/core_counter_ingress_worker.cc
	SRCS += $(SRCDIR)/core/core_counter_egress_worker.cc
	SRCS += $(SRCDIR)/core/core_counter_log_worker.cc
	SRCS += $(SRCDIR)/app/app_v2.cc
	SRCS += $(SRCDIR)/core/core_pgw_base_worker.cc
	SRCS += $(SRCDIR)/core/core_pgw_ingress_tunnel_worker.cc
	SRCS += $(SRCDIR)/core/core_pgw_egress_tunnel_worker.cc
else ifeq ($(ARCH),18)
	SRCS += $(SRCDIR)/core/core_counter_ingress_worker.cc
	SRCS += $(SRCDIR)/core/core_counter_egress_worker.cc
	SRCS += $(SRCDIR)/core/core_counter_log_worker.cc
	SRCS += $(SRCDIR)/app/app_v2.cc
	SRCS += $(SRCDIR)/core/core_pgw_base_worker.cc
	SRCS += $(SRCDIR)/core/core_pgw_ingress_tunnel_worker.cc
	SRCS += $(SRCDIR)/core/core_pgw_egress_tunnel_worker.cc
else ifeq ($(ARCH),19)
	SRCS += $(SRCDIR)/core/core_counter_ingress_worker.cc
	SRCS += $(SRCDIR)/core/core_counter_egress_worker.cc
	SRCS += $(SRCDIR)/core/core_counter_log_worker.cc
	SRCS += $(SRCDIR)/app/app_v2.cc
	SRCS += $(SRCDIR)/core/core_pgw_base_worker.cc
	SRCS += $(SRCDIR)/core/core_pgw_ingress_tunnel_worker.cc
	SRCS += $(SRCDIR)/core/core_pgw_egress_tunnel_worker.cc
else ifeq ($(ARCH),20)
	SRCS += $(SRCDIR)/core/core_counter_ingress_worker.cc
	SRCS += $(SRCDIR)/core/core_counter_egress_worker.cc
	SRCS += $(SRCDIR)/core/core_counter_log_worker.cc
	SRCS += $(SRCDIR)/app/app_v2.cc
	SRCS += $(SRCDIR)/core/core_pgw_base_worker.cc
	SRCS += $(SRCDIR)/core/core_pgw_ingress_tunnel_worker.cc
	SRCS += $(SRCDIR)/core/core_pgw_egress_tunnel_worker.cc
else ifeq ($(ARCH),21)
	SRCS += $(SRCDIR)/core/core_counter_ingress_worker.cc
	SRCS += $(SRCDIR)/core/core_counter_egress_worker.cc
	SRCS += $(SRCDIR)/core/core_counter_log_worker.cc
	SRCS += $(SRCDIR)/app/app_v2.cc
	SRCS += $(SRCDIR)/core/core_pgw_base_worker.cc
	SRCS += $(SRCDIR)/core/core_pgw_ingress_tunnel_worker.cc
	SRCS += $(SRCDIR)/core/core_pgw_egress_tunnel_worker.cc
else
	SRCS += $(SRCDIR)/core/core_pgw_base_worker.cc
	SRCS += $(SRCDIR)/core/core_pgw_ingress_worker.cc
	SRCS += $(SRCDIR)/core/core_pgw_egress_worker.cc
	SRCS += $(SRCDIR)/core/core_pgw_ingress_distributor_worker.cc
	SRCS += $(SRCDIR)/core/core_pgw_egress_distributor_worker.cc
endif


SRCS += $(SRCDIR)/conf/static_conf.cc
SRCS += $(SRCDIR)/app/app.cc
SRCS += $(SRCDIR)/core/cores.cc $(SRCDIR)/core/core_rx.cc $(SRCDIR)/core/core_tx.cc
SRCS += $(SRCDIR)/core/core_encap_worker.cc
SRCS += $(SRCDIR)/core/core_gtpu2gre_worker.cc
SRCS += $(SRCDIR)/core/core_greterm_worker.cc
SRCS += $(SRCDIR)/core/core_counter_worker.cc
SRCS += $(SRCDIR)/core/core_transfer_worker.cc
SRCS += $(SRCDIR)/core/core_rx_distributor.cc
SRCS += $(SRCDIR)/core/core_tapbase.cc
SRCS += $(SRCDIR)/core/core_taprx.cc
SRCS += $(SRCDIR)/core/core_taptx.cc

ifeq ($(LIB),1)
	MOD = libaccm.so
	SRCS  =  $(SRCDIR)/accm/accumulator.cc
	SRCS +=  $(SRCDIR)/accm/accumulator_impl.cc
	SRCS +=  $(SRCDIR)/accm/accumulator_index_impl.cc
	SRCS +=  $(SRCDIR)/accm/accumulator_index_header_impl.cc
	SRCS +=  $(SRCDIR)/accm/accumulator_index_record_impl.cc

	CXXFLAGS += -shared -fPIC -I$(SRCDIR)/accm/
endif

ifeq ($(ARCH),3)
  CXXFLAGS += -DPGW_RX=1
else ifeq ($(ARCH),4)
  CXXFLAGS += -DPGW_RX=2
else ifeq ($(ARCH),9)
  CXXFLAGS += -DTUNNEL_RX
else ifeq ($(ARCH),14)
  CXXFLAGS += -DCOUNTER_RX
else ifeq ($(ARCH),15)
  CXXFLAGS += -DTUNNEL_RX
else ifeq ($(ARCH),16)
  CXXFLAGS += -DTUNNEL_RX
else ifeq ($(ARCH),17)
  CXXFLAGS += -DTUNNEL_RX
else ifeq ($(ARCH),18)
  CXXFLAGS += -DTUNNEL_RX
else ifeq ($(ARCH),19)
  CXXFLAGS += -DTUNNEL_RX
else ifeq ($(ARCH),20)
  CXXFLAGS += -DTUNNEL_RX
else ifeq ($(ARCH),21)
  CXXFLAGS += -DTUNNEL_RX
endif

CXX = g++
OBJS = $(SRCS:.cc=.o)
all: $(MOD)

$(MOD):$(OBJS)
	$(CXX) -o $(MOD) $(CXXFLAGS) $(SRCS) $(LIBS) 
	chmod u+x ./$(MOD)
clean:
	rm -f ./*.o
	rm -f ./$(TOOLDIR)/*.o
	rm -f ./$(MOD)
	rm -f ./*.o
	rm -f ./$(SRCDIR)/*.o
	rm -f ./$(SRCDIR)/accm/*.o
	rm -f ./$(SRCDIR)/app/*.o
	rm -f ./$(SRCDIR)/arch/*.o
	rm -f ./$(SRCDIR)/conf/*.o
	rm -f ./$(SRCDIR)/core/*.o
	rm -f ./$(TSTDIR)/*.o
	rm -f ./$(TSTDIR)/t_*
compile_test:
#	cd ./test;make TEST=1 $(MANYCORE)
	cd ./test;make TEST=2 $(MANYCORE)
	cd ./test;make TEST=3 $(MANYCORE)
	cd ./test;make TEST=4 $(MANYCORE)
#	cd ./test;make TEST=5 $(MANYCORE)
	cd ./test;make TEST=6 $(MANYCORE)
	cd ./test;make TEST=7 $(MANYCORE)
	cd ./test;make TEST=8 $(MANYCORE)
	cd ./test;make TEST=9 $(MANYCORE)
	cd ./test;make TEST=10 $(MANYCORE)
	cd ./test;make TEST=11 $(MANYCORE)
	cd ./test;make TEST=12 $(MANYCORE)
	cd ./test;make TEST=13 $(MANYCORE)
	cd ./test;make TEST=14 $(MANYCORE)
	cd ./test;make TEST=15 $(MANYCORE)
	cd ./test;make TEST=16 $(MANYCORE)
	cd ./test;make TEST=99 $(MANYCORE)
	cd ./test;make TEST=1002 $(MANYCORE)
	cd ./test;make TEST=1004 $(MANYCORE)
	cd ./test;make TEST=1013 $(MANYCORE)
	cd ./test;make TEST=1014 $(MANYCORE)
	cd ./test;make TEST=1015 $(MANYCORE)
	cd ./test;make TEST=1020 $(MANYCORE)
	cd ./test;make TEST=1021 $(MANYCORE)
#	cd ./test;make TEST=1099 $(MANYCORE)

compile_counter_test:
	cd ./test;make TEST=6 $(MANYCORE)
	cd ./test;make TEST=7 $(MANYCORE)
	cd ./test;make TEST=8 $(MANYCORE)
	cd ./test;make TEST=9 $(MANYCORE)
	cd ./test;make TEST=10 $(MANYCORE)
	cd ./test;make TEST=11 $(MANYCORE)

run_counter_test:
	./test/t_counter
	./test/t_counter_full_1ring
	./test/t_counter_enough_map
	./test/t_counter_rotate
	./test/t_transfer
	./test/t_counter_to_transfer

compile_system_test:
	cd ./test;make TEST=150 $(MANYCORE)
	cd ./test;make TEST=151 $(MANYCORE)
	cd ./test;make TEST=152 $(MANYCORE)
	cd ./test;make TEST=153 $(MANYCORE)
run_system_test:
	./test/t_pgw_ingress_g
	./test/t_tunnel_ingress_g
	./test/t_tunnel_egress_g
	./test/t_pgw_egress_g
test_current:
	cd ./test;make -f Makefile TEST=99 $(MANYCORE)
test_template:
	cd ./test;make -f Makefile.nodb TEST=19 $(MANYCORE)
test_num:
	cd ./test;make TEST=$(NO) $(MANYCORE)
test_num_nodb:
	cd ./test;make -f Makefile.nodb TEST=$(NO) $(MANYCORE)
run_all_test:
	./test/t_encap
	./test/t_gtpu2gre
	./test/t_greterm
	./test/t_counter
	./test/t_counter_full_1ring
	./test/t_counter_enough_map
	./test/t_counter_rotate
	./test/t_transfer
	./test/t_counter_to_transfer
	./test/t_pgw_ingress
	./test/t_rx_distributor
	./test/t_pgw_ingress_dist
	./test/t_pgw_egress_dist
	make run_nodb_test_g
	./test/t_perf
	./test/t_warmup
	make run_throughput_test
	make run_system_test
run_throughput_test:
	./test/t_encap_t
	./test/t_greterm_t
	./test/t_rx_distributor_t
	./test/t_pgw_ingress_distributor_t
	./test/t_pgw_egress_distributor_t
	./test/t_rx_t
	./test/t_tx_t
#	./test/t_throughput_t
print:
	@echo "CC='$(CC)'"
	@echo "CXXFLAGS='$(CXXFLAGS)'"
	@echo "ARCH='$(ARCH)'"
compile_nodb:
	cd ./test;make -f Makefile.nodb TEST=2 $(MANYCORE)
	cd ./test;make -f Makefile.nodb TEST=3 $(MANYCORE)
	cd ./test;make -f Makefile.nodb TEST=4 CANNOT_USE_GRE=0 $(MANYCORE)
	cd ./test;make -f Makefile.nodb TEST=17 $(MANYCORE)
	cd ./test;make -f Makefile.nodb TEST=18 $(MANYCORE)
	cd ./test;make -f Makefile.nodb TEST=102 $(MANYCORE)
	cd ./test;make -f Makefile.nodb TEST=103 $(MANYCORE)
	cd ./test;make -f Makefile.nodb TEST=104 CANNOT_USE_GRE=0 $(MANYCORE)
run_nodb_test:
	./test/t_encap
	./test/t_gtpu2gre
	./test/t_greterm
##	./test/t_limitpps
	./test/t_greterm_v6
run_nodb_test_g:
	./test/t_encap_g
	./test/t_gtpu2gre_g
	./test/t_greterm_g
compile_tunnel_pgw:
	cd ./test;make -f Makefile TEST=18 $(MANYCORE)
	cd ./test;make -f Makefile TEST=19 $(MANYCORE)
run_tunnel_pgw:
	./test/t_pgw_ingress_tunnel
	./test/t_pgw_egress_tunnel
