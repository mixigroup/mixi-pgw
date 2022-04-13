MOD_ENCAP = t_encap
MOD_PERF  = t_perf

LTEEPCLIBDIR=/usr/local/lib/
DPDKPATH=$(RTE_SDK)/$(RTE_TARGET)
CXXFLAGS = -I$(DPDKPATH) -include $(DPDKPATH)/rte_config.h -mssse3 -I./ 
CXXFLAGS += -D__DEBUG__ -I../inc/
CXXFLAGS += -DDEBUG=true -DNDEBUG=1 -I../inc/
CXXFLAGS += -std=c++17
CXXFLAGS += -Wno-register

CXXFLAGS += -fPIC
PDIR=$(shell pwd)
CXXFLAGS += -I$(PDIR)/../../deps/mysql-8.0.28/_build/include/
CXXFLAGS += -I$(PDIR)/../../deps/mysql-8.0.28/include
CXXFLAGS += -I$(PDIR)/../../deps/mysql-8.0.28/libbinlogevents/include
CXXFLAGS += -I$(PDIR)/../../deps/mysql-8.0.28/sql
CXXFLAGS += -I$(PDIR)/../../deps/mysql-8.0.28/
CXXFLAGS += -I$(PDIR)/../../deps/mysql-8.0.28/extra/rapidjson/include


STATICLIBS += $(PDIR)/../../deps/mysql-8.0.28/_build/libbinlogevents/lib/libbinlogevents.a
STATICLIBS += $(PDIR)/../../deps/mysql-8.0.28/_build/archive_output_directory/libjson_binlog_static.a
STATICLIBS += $(PDIR)/../../deps/mysql-8.0.28/_build/archive_output_directory/libmysqlclient.a
STATICLIBS += $(PDIR)/../../deps/mysql-8.0.28/_build/archive_output_directory/libmysys.a
STATICLIBS += $(PDIR)/../../deps/mysql-8.0.28/_build/archive_output_directory/libkeyring_common.a
MYSQL_SRC_DIR=$(PDIR)/../../deps/mysql-8.0.28/

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
BINLOG_SRCS += ${MYSQL_SRC_DIR}/libbinlogevents/src/trx_boundary_parser.cpp




ifdef CANNOT_USE_GRE
CXXFLAGS += -D__CANNOT_USE_GRE__
endif

ifeq ($(MANYCORE),1)
	__MANYCORE__ = 1
endif
CXXFLAGS += -D__TEST_MODE__

LIBS = \
    -static-libgcc -m64 -pthread -march=native \
    -Wl,--no-as-needed \
    -L$(DPDKPATH)/lib \
    $(STATICLIBS) \
    -L$(LTEEPCLIBDIR) \
    -lpthread -ldl -lrt -lm -lpcap -lnuma -lssl -lcrypto \
    -Wl,--whole-archive \
    -Wl,--start-group \
    -ldpdk \
    -Wl,--end-group \
    -Wl,--no-whole-archive

MOD = basic_test

ifeq ($(TEST),1)
	SRCS +=  ./kni_bk/drv.cc
	SRCS +=  ./kni_bk/init_kni.cc
	SRCS +=  ./kni_bk/init_port.cc
	SRCS +=  ./kni_bk/kni_alloc.cc
	SRCS +=  ./kni_bk/kni_eth.cc
	SRCS +=  ./kni_bk/kni_free.cc
	SRCS +=  ./kni_bk/kni_util.cc
	SRCS +=  ./kni_bk/worker.cc

	MOD = t_kni
else ifeq ($(TEST),2)
	SRCS +=  ./ut_encap.cc
	MOD = t_encap
else ifeq ($(TEST),3)
	SRCS +=  ./ut_gtpu_2_gre.cc
	MOD = t_gtpu2gre
else ifeq ($(TEST),4)
	SRCS +=  ./ut_greterm.cc
	MOD = t_greterm
else ifeq ($(TEST),5)
	SRCS +=  ./ut_accm.cc
	MOD = t_accm
	LIBS += -lgtest -lgtest_main -laccm -L../
else ifeq ($(TEST),6)
	SRCS +=  ./ut_counter_simple.cc
	CXXFLAGS += -DTHRESHOLD_TRANSFER=1
	MOD = t_counter
	LIBS += -lgtest -lgtest_main
else ifeq ($(TEST),7)
	SRCS +=  ./ut_counter_full_1ring.cc
	MOD = t_counter_full_1ring
	CXXFLAGS += -DTHRESHOLD_TRANSFER=1
	LIBS += -lgtest -lgtest_main
else ifeq ($(TEST),8)
	SRCS +=  ./ut_counter_enough_map.cc
	MOD = t_counter_enough_map
	LIBS += -lgtest -lgtest_main
else ifeq ($(TEST),9)
	SRCS +=  ./ut_counter_rotate.cc
	MOD = t_counter_rotate
	CXXFLAGS += -DTHRESHOLD_TRANSFER=1
	LIBS += -lgtest -lgtest_main
else ifeq ($(TEST),10)
	SRCS +=  ./ut_transfer.cc
	MOD = t_transfer
	CXXFLAGS += -DTHRESHOLD_TRANSFER=1
	LIBS += -lgtest -lgtest_main
else ifeq ($(TEST),11)
	SRCS +=  ./ut_counter_to_transfer.cc
	MOD = t_counter_to_transfer
	CXXFLAGS += -DTHRESHOLD_TRANSFER=1
	LIBS += -lgtest -lgtest_main
else ifeq ($(TEST),12)
	SRCS +=  ./ut_pgw_ingress.cc
	MOD = t_pgw_ingress
	CXXFLAGS += -DTHRESHOLD_TRANSFER=1
	LIBS += -lgtest -lgtest_main
else ifeq ($(TEST),13)
	SRCS +=  ./ut_rx_distributor.cc
	MOD = t_rx_distributor
	CXXFLAGS += -D__TEST_MODE__=1
	LIBS += -lgtest -lgtest_main
else ifeq ($(TEST),14)
	SRCS +=  ./ut_pgw_ingress_distributor.cc
	MOD = t_pgw_ingress_dist
	CXXFLAGS += -D__TEST_MODE__=1
	LIBS += -lgtest -lgtest_main
else ifeq ($(TEST),15)
	SRCS +=  ./ut_pgw_egress_distributor.cc
	MOD = t_pgw_egress_dist
	CXXFLAGS += -D__TEST_MODE__=1
	LIBS += -lgtest -lgtest_main
else ifeq ($(TEST),16)
	SRCS +=  ./ut_warmup.cc
	MOD = t_warmup
	CXXFLAGS += -D__TEST_MODE__=1
	LIBS += -lgtest -lgtest_main
else ifeq ($(TEST),17)
	SRCS +=  ../src/core/core_counter_log_worker.cc
	SRCS +=  ./ut_counter_log.cc
	MOD = t_counter_log
	CXXFLAGS += -DTHRESHOLD_TRANSFER=1
	LIBS += -lgtest -lgtest_main
else ifeq ($(TEST),18)
	SRCS +=  ../src/core/core_pgw_ingress_tunnel_worker.cc
	SRCS +=  ./ut_pgw_ingress_tunnel.cc
	MOD = t_pgw_ingress_tunnel
	CXXFLAGS += -D__TEST_MODE__=1
	LIBS += -lgtest -lgtest_main
else ifeq ($(TEST),19)
	SRCS +=  ../src/core/core_pgw_egress_tunnel_worker.cc
	SRCS +=  ./ut_pgw_egress_tunnel.cc
	MOD = t_pgw_egress_tunnel
	CXXFLAGS += -D__TEST_MODE__=1
	LIBS += -lgtest -lgtest_main
else ifeq ($(TEST),20)
	SRCS +=  ../src/core/core_counter_egress_worker.cc
	SRCS +=  ../src/core/core_counter_ingress_worker.cc
	SRCS +=  ../src/core/core_pgw_ingress_tunnel_worker.cc
	SRCS +=  ../src/core/core_pgw_egress_tunnel_worker.cc
	SRCS +=  ./ut_pgw_ingress_policer.cc
	MOD = t_pgw_ingress_policer
	CXXFLAGS += -D__TEST_MODE__=1 -DARCH=15
	LIBS += -lgtest -lgtest_main
else ifeq ($(TEST),21)
	SRCS +=  ../src/core/core_counter_egress_worker.cc
	SRCS +=  ../src/core/core_counter_ingress_worker.cc
	SRCS +=  ../src/core/core_pgw_ingress_tunnel_worker.cc
	SRCS +=  ../src/core/core_pgw_egress_tunnel_worker.cc
	SRCS +=  ./ut_pgw_egress_policer.cc
	MOD = t_pgw_egress_policer
	CXXFLAGS += -D__TEST_MODE__=1 -DARCH=15
	LIBS += -lgtest -lgtest_main
else ifeq ($(TEST),30)
	SRCS +=  ./ut_boot_seq.cc
	MOD = t_boot_seq
	CXXFLAGS += -D__TEST_MODE__=1
	LIBS += -lgtest -lgtest_main
else ifeq ($(TEST),99)
	SRCS +=  ./ut_perf.cc
	MOD = t_perf
	CXXFLAGS += -D__TEST_MODE__=1 -DARCH=99
	LIBS += -lgtest -lgtest_main
else ifeq ($(TEST),150) #pgw ingress
    SRCS +=  ./ut_pgw_ingress_g.cc
    MOD = t_pgw_ingress_g
    LIBS += -lgtest -lgtest_main
    CXXFLAGS += -DARCH=99 -D__TEST_MODE__
else ifeq ($(TEST),151) #tunnel ingress
    SRCS +=  ./ut_tunnel_ingress_g.cc
    MOD = t_tunnel_ingress_g
    LIBS += -lgtest -lgtest_main
    CXXFLAGS += -DARCH=99 -D__TEST_MODE__ -D__CANNOT_USE_GRE__ -DTUNNEL_RX
else ifeq ($(TEST),152) #tunnel egress
    SRCS +=  ./ut_tunnel_egress_g.cc
    MOD = t_tunnel_egress_g
    LIBS += -lgtest -lgtest_main
    CXXFLAGS += -DARCH=99 -D__TEST_MODE__ -D__CANNOT_USE_GRE__ -DTUNNEL_RX
else ifeq ($(TEST),153) #pgw egress
    SRCS +=  ./ut_pgw_egress_g.cc
    MOD = t_pgw_egress_g
    LIBS += -lgtest -lgtest_main
    CXXFLAGS += -DARCH=99 -D__TEST_MODE__
else ifeq ($(TEST),1002)
    SRCS +=  ./ut_encap_t.cc
    MOD = t_encap_t
    CXXFLAGS += -D__TEST_MODE__=1 -DARCH=99
    LIBS += -lgtest -lgtest_main
else ifeq ($(TEST),1004)
    SRCS +=  ./ut_greterm_t.cc
    MOD = t_greterm_t
    CXXFLAGS += -D__TEST_MODE__=1 -DARCH=99
    LIBS += -lgtest -lgtest_main
else ifeq ($(TEST),1013)
    SRCS +=  ./ut_rx_distributor_t.cc
    MOD = t_rx_distributor_t
    CXXFLAGS += -D__TEST_MODE__=1 -DARCH=99
    LIBS += -lgtest -lgtest_main
else ifeq ($(TEST),1014)
    SRCS +=  ./ut_pgw_ingress_distributor_t.cc
    MOD = t_pgw_ingress_distributor_t
    CXXFLAGS += -D__TEST_MODE__=1 -DARCH=99
    LIBS += -lgtest -lgtest_main
else ifeq ($(TEST),1015)
    SRCS +=  ./ut_pgw_egress_distributor_t.cc
    MOD = t_pgw_egress_distributor_t
    CXXFLAGS += -D__TEST_MODE__=1 -DARCH=99
    LIBS += -lgtest -lgtest_main
else ifeq ($(TEST),1020)
    SRCS +=  ./ut_rx_t.cc
    MOD = t_rx_t
    CXXFLAGS += -D__TEST_MODE__=1 -DARCH=99
    LIBS += -lgtest -lgtest_main
else ifeq ($(TEST),1021)
    SRCS +=  ./ut_tx_t.cc
    MOD = t_tx_t
    CXXFLAGS += -D__TEST_MODE__=1 -DARCH=99
    LIBS += -lgtest -lgtest_main
else ifeq ($(TEST),1099)
	SRCS +=  ./ut_throughput_t.cc
	MOD = t_throughput_t
	CXXFLAGS += -D__TEST_MODE__=1 -DARCH=99
	LIBS += -lgtest -lgtest_main
else
	$(error error is $(TEST))
endif
ifeq ($(TEST),5)
else
	## テスト対象
	SRCS +=  ../src/core/cores.cc
	SRCS +=  ../src/core/core_encap_worker.cc
	SRCS +=  ../src/core/core_gtpu2gre_worker.cc
	SRCS +=  ../src/core/core_greterm_worker.cc
	SRCS +=  ../src/core/core_counter_worker.cc
	SRCS +=  ../src/core/core_transfer_worker.cc
	SRCS +=  ../src/core/core_pgw_base_worker.cc
	SRCS +=  ../src/core/core_pgw_egress_worker.cc
	SRCS +=  ../src/core/core_pgw_ingress_worker.cc

	SRCS +=  ../src/core/core_rx_distributor.cc
	SRCS +=  ../src/core/core_pgw_egress_distributor_worker.cc
	SRCS +=  ../src/core/core_pgw_ingress_distributor_worker.cc

	SRCS +=  ../src/core/core_tapbase.cc
	SRCS +=  ../src/core/core_taprx.cc
	SRCS +=  ../src/core/core_taptx.cc
endif

ifeq ($(TEST),9)
  CXXFLAGS += -O2
else ifeq ($(TEST),99)
  CXXFLAGS += -O0 -g
	SRCS +=  ../src/app/app.cc
	SRCS +=  ../src/conf/static_conf.cc
	SRCS +=  ../src/core/core_rx.cc
	SRCS +=  ../src/core/core_tx.cc
else ifeq ($(TEST),150)
  CXXFLAGS += -O0 -g
    SRCS +=  ../src/app/app.cc
    SRCS +=  ../src/conf/static_conf.cc
    SRCS +=  ../src/core/core_rx.cc
    SRCS +=  ../src/core/core_tx.cc
else ifeq ($(TEST),151)
  CXXFLAGS += -O0 -g
    SRCS +=  ../src/app/app.cc
    SRCS +=  ../src/conf/static_conf.cc
    SRCS +=  ../src/core/core_rx.cc
    SRCS +=  ../src/core/core_tx.cc
else ifeq ($(TEST),152)
  CXXFLAGS += -O0 -g
    SRCS +=  ../src/app/app.cc
    SRCS +=  ../src/conf/static_conf.cc
    SRCS +=  ../src/core/core_rx.cc
    SRCS +=  ../src/core/core_tx.cc
else ifeq ($(TEST),153)
  CXXFLAGS += -O0 -g
    SRCS +=  ../src/app/app.cc
    SRCS +=  ../src/conf/static_conf.cc
    SRCS +=  ../src/core/core_rx.cc
    SRCS +=  ../src/core/core_tx.cc
else ifeq ($(TEST),1002)
  CXXFLAGS += -O0 -g
    SRCS +=  ../src/app/app.cc
    SRCS +=  ../src/conf/static_conf.cc
    SRCS +=  ../src/core/core_rx.cc
    SRCS +=  ../src/core/core_tx.cc
else ifeq ($(TEST),1004)
  CXXFLAGS += -O0 -g
    SRCS +=  ../src/app/app.cc
    SRCS +=  ../src/conf/static_conf.cc
    SRCS +=  ../src/core/core_rx.cc
    SRCS +=  ../src/core/core_tx.cc
else ifeq ($(TEST),1013)
  CXXFLAGS += -O0 -g
    SRCS +=  ../src/app/app.cc
    SRCS +=  ../src/conf/static_conf.cc
    SRCS +=  ../src/core/core_rx.cc
    SRCS +=  ../src/core/core_tx.cc
else ifeq ($(TEST),1014)
  CXXFLAGS += -O0 -g
    SRCS +=  ../src/app/app.cc
    SRCS +=  ../src/conf/static_conf.cc
    SRCS +=  ../src/core/core_rx.cc
    SRCS +=  ../src/core/core_tx.cc
else ifeq ($(TEST),1015)
  CXXFLAGS += -O0 -g
    SRCS +=  ../src/app/app.cc
    SRCS +=  ../src/conf/static_conf.cc
    SRCS +=  ../src/core/core_rx.cc
    SRCS +=  ../src/core/core_tx.cc
else ifeq ($(TEST),1020)
  CXXFLAGS += -O0 -g
    SRCS +=  ../src/app/app.cc
    SRCS +=  ../src/conf/static_conf.cc
    SRCS +=  ../src/core/core_rx.cc
    SRCS +=  ../src/core/core_tx.cc
else ifeq ($(TEST),1021)
  CXXFLAGS += -O0 -g
    SRCS +=  ../src/app/app.cc
    SRCS +=  ../src/conf/static_conf.cc
    SRCS +=  ../src/core/core_rx.cc
    SRCS +=  ../src/core/core_tx.cc
else ifeq ($(TEST),1099)
  CXXFLAGS += -O0 -g
    SRCS +=  ../src/app/app.cc
    SRCS +=  ../src/conf/static_conf.cc
    SRCS +=  ../src/core/core_rx.cc
    SRCS +=  ../src/core/core_tx.cc
else
  CXXFLAGS += -O0 -g
endif

SRCS += $(BINLOG_SRCS)


CXX = g++
OBJS = $(SRCS:.cc=.o)
all: $(MOD)

$(MOD):$(OBJS)
	$(CXX) -o $(MOD) $(CXXFLAGS) $(SRCS) $(LIBS) 
	chmod u+x ./$(MOD)
clean:
	rm -f ./$(MOD)
	rm -f ./*.o
