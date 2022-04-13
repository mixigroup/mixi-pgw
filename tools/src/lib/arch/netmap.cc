//
// Created by mixi on 2017/04/24.
//

#include <poll.h>
#include <assert.h>
#include <string>
#include <stdexcept>

#define NETMAP_WITH_LIBS
#include <net/netmap_user.h>

#include "mixipgw_tools_def.h"
#include "lib/logger.hpp"
#include "lib/module.hpp"
#include "lib/misc.hpp"
#include "lib/pktheader.hpp"
#include "lib/arch/netmap.hpp"
#include "lib/process.hpp"
#include "lib/interface/filter_interface.hpp"
//
#define  NMRING(r)  ((struct netmap_ring*)r)
#define  NMSLOT(s)  ((struct netmap_slot*)s)
#define  NMDESC(n)  ((struct nm_desc*)n)
#define  NMRING_SPACE(r)   nm_ring_space(NMRING(r))
#define  NMRING_EMPTY(r)   nm_ring_empty(NMRING(r))
#define  NMRING_NEXT(r,c)  nm_ring_next(NMRING(r), c);

namespace MIXIPGW_TOOLS{
  // ---------------------------
  // netmap wrapper
  class NetmapHandle{
  public:
      NetmapHandle():desc_(NULL),rxslot_(NULL),txslot_(NULL),pkt_count_(0),evt_count_(0),persistent_flag_(0){
          bzero(&pollfd_,sizeof(pollfd_));
      }
      ~NetmapHandle(){ }
  public:
      struct pollfd     pollfd_;
      struct nm_desc   *desc_;
      std::string       ifnm_;
      void*             rxslot_;
      void*             txslot_;
      uint64_t          pkt_count_,evt_count_;
      int               persistent_flag_;
  };


  // ---------------------------
  // NetmapInterface Implemented.
  Netmap::Netmap(ProcessParameter* param){
      handle_ = new NetmapHandle();
      param_  = param;
      filter_level_ = 0;
  }
  Netmap::~Netmap(){
      NetmapHandle* ph = (NetmapHandle*)handle_;
      assert(ph);
      delete ((NetmapHandle*)ph);
      handle_ = NULL;
  }
  int Netmap::Open(const char* ifnm,int cpuid){
      NetmapHandle* ph = (NetmapHandle*)handle_;
      struct nmreq  nmd;
      assert(ph);

      // example: eth0 => netnmap:eth0
      ph->ifnm_ = "netmap:";
      ph->ifnm_ += (ifnm?ifnm:"");
      // ringid == cpuid
      if (cpuid >= 0){
          char nbf[32] = {0};
          snprintf(nbf, sizeof(nbf)-1,"-%d", cpuid);
          ph->ifnm_ += nbf;
      }
      bzero(&nmd, sizeof(nmd));
      // magic from pkt-gen.c
      nmd.nr_tx_rings = nmd.nr_rx_rings = 0;
      nmd.nr_tx_slots = nmd.nr_rx_slots = 0;
      //
      ph->desc_ = nm_open(ph->ifnm_.c_str(), &nmd, 0, NULL);
      if (ph->desc_ == NULL) {
          throw std::runtime_error("failed. netmap open.");
      }
      if (cpuid!=(uint32_t)-1){
          Misc::SetAffinity(cpuid);
      }
      Logger::LOGINF("<PA>%d tx /%d rx rings. cpu[%u] %s",
                      ph->desc_->req.nr_tx_rings, ph->desc_->req.nr_rx_rings, cpuid, ifnm);
      /* setup poll(2) variables. */
      ph->pollfd_.fd = ph->desc_->fd;
      Logger::LOGINF("Wait %d secs for link to come up...(%s . %s) : %s", WAIT_LINKSEC,
                     ph->ifnm_.c_str(), MF_IS_INGRESS(param_->Get(ProcessParameter::USG_MODULE_FLAG))?"ingress":"egress");
      sleep(WAIT_LINKSEC);
      Logger::LOGINF("Ready to go, %s 0x%x/%d ", ph->desc_->req.nr_name, ph->desc_->first_rx_ring, ph->desc_->req.nr_rx_rings);

      return(RETOK);
  }
  int Netmap::Poll(int flag, PktInterface* pkt){
      NetmapHandle* ph = (NetmapHandle*)handle_;
      int ret,pktcnt = 0;
      assert(ph);
      // initialize event flag.
      ph->pollfd_.revents = 0;
      ph->pollfd_.events  = POLLIN;
      ioctl(ph->pollfd_.fd, NIOCRXSYNC, NULL);
      ioctl(ph->pollfd_.fd, NIOCTXSYNC, NULL);
      //
      ret = poll(&ph->pollfd_, 1, 200);
      if (ret <= 0) {
          if (ret < 0 && errno != EAGAIN && errno != EINTR){
              Logger::LOGERR("poll error %s", strerror(errno));
              return(RETERR);
          }
          return(RETWRN);
      }
      //
      if (ph->pollfd_.revents & POLLIN) {
          if ((pktcnt = SearchValidRing()) > 0){
              ph->pkt_count_ += pktcnt;
              ph->evt_count_ ++;
          }
      }
      return(RETOK);
  }
  int Netmap::Send(int flag,PktInterface* pkt){
      NetmapHandle* ph = (NetmapHandle*)handle_;
      if (!ph){
          return(RETERR);
      }
      // transfer = swap index (ts -> rs for zero copy.)
      uint32_t tidx   = NMSLOT(ph->txslot_)->buf_idx;
      NMSLOT(ph->txslot_)->buf_idx = NMSLOT(ph->rxslot_)->buf_idx;
      NMSLOT(ph->rxslot_)->buf_idx = tidx;
      //
      NMSLOT(ph->rxslot_)->len = pkt->Length();
      NMSLOT(ph->txslot_)->len = NMSLOT(ph->rxslot_)->len;
      NMSLOT(ph->txslot_)->flags |= NS_BUF_CHANGED;
      NMSLOT(ph->rxslot_)->flags |= NS_BUF_CHANGED;
      if (Module::VERBOSE() > PCAPLEVEL_DBG){
          Logger::APPEND_PCAP(0, (const char*)EH(pkt), pkt->Length());
      }
      return(RETOK);
  }
  int Netmap::SearchValidRing(void){
      NetmapHandle* ph = (NetmapHandle*)handle_;
      if (!ph){
          return(RETERR);
      }
      // netmap batch processing at NIC slot.
      u_int m = 0;
      uint16_t rxringidx = ph->desc_->first_rx_ring;
      uint16_t txringidx = ph->desc_->first_tx_ring;
      //
      while (rxringidx <= ph->desc_->last_rx_ring && txringidx <= ph->desc_->last_tx_ring) {
          void* rxr = NETMAP_RXRING(ph->desc_->nifp, rxringidx);
          void* txr = NETMAP_TXRING(ph->desc_->nifp, txringidx);
          if (NMRING_EMPTY(rxr)) {
              rxringidx++;
              continue;
          }
          if (NMRING_EMPTY(txr)) {
              txringidx++;
              continue;
          }
          m += ProcessRing(rxr, txr);
      }
      return (m);
  }
  int Netmap::ProcessRing(void* rxr, void* txr){
      u_int ret = 0;
      int      persistent_flag;
      uint32_t txring_space_work;
      uint32_t rxring_cur_work = NMRING(rxr)->cur;                  // RX
      uint32_t txring_cur_work = NMRING(txr)->cur;                  // TX
      uint32_t limit = NMRING(rxr)->num_slots;
      NetmapHandle* ph = (NetmapHandle*)handle_;
      if (!ph){
          return(RETERR);
      }
      persistent_flag = ph->persistent_flag_;
      persistent_flag &= ~PROC_NEED_SEND;

      /* print a warning if any of the ring flags is set (e.g. NM_REINIT) */
      if (NMRING(rxr)->flags || NMRING(txr)->flags){
          MIXIPGW_TOOLS::Logger::LOGWRN("rxflags %x txflags %x", NMRING(rxr)->flags, NMRING(txr)->flags);
          NMRING(rxr)->flags = 0;
          NMRING(txr)->flags = 0;
      }
      //
      txring_space_work = NMRING_SPACE(rxr);
      if (txring_space_work < limit){
          limit = txring_space_work;
      }
      txring_space_work = NMRING_SPACE(txr);
      if (txring_space_work < limit){
          limit = txring_space_work;
      }
      while (limit-- > 0) {
          // init processed flag
          void *rxslot = &NMRING(rxr)->slot[rxring_cur_work];
          void *txslot = &NMRING(txr)->slot[txring_cur_work];
          char* req = NETMAP_BUF(NMRING(rxr), NMSLOT(rxslot)->buf_idx);
          char* res = NETMAP_BUF(NMRING(txr), NMSLOT(txslot)->buf_idx);
          uint16_t reslen = NMSLOT(rxslot)->len;
          uint16_t reqlen = NMSLOT(rxslot)->len;
          // prefetch the buffer for next loop.
          __builtin_prefetch(req);

          /* swap packets */
          if (NMSLOT(txslot)->buf_idx < 2 || NMSLOT(rxslot)->buf_idx < 2) {
              MIXIPGW_TOOLS::Logger::LOGERR("wrong index rx[%d] = %d  -> tx[%d] = %d",
                                     rxring_cur_work, NMSLOT(rxslot)->buf_idx, txring_cur_work, NMSLOT(txslot)->buf_idx);
              sleep(2);
          }
          /* copy the packet length. */
          if (NMSLOT(rxslot)->len > 2048) {
              MIXIPGW_TOOLS::Logger::LOGERR("wrong len %d rx[%d] -> tx[%d]", NMSLOT(rxslot)->len, rxring_cur_work, txring_cur_work);
              NMSLOT(rxslot)->len = 0;
          }
          // callback to filters.
          if (param_ != NULL){
              PktHeader pkth(req, NMSLOT(rxslot)->len);
              pkth.Type(event_->OnPacketType(&pkth, module_flag_, udp_ctrl_port_));
              //
              FilterContainer* fc = param_->Get();
              if (!fc){
                  throw "failed. filter containers.";
              }
              // run filter(by packet type)
              fc->Exec(&pkth, &persistent_flag);
              // zero copy send.
              if (persistent_flag & PROC_NEED_SEND){
                  ph->rxslot_ = rxslot;
                  ph->txslot_ = txslot;
                  // Reflect on Same NIC.(swap index.)
                  Send(persistent_flag, &pkth);
              }
          }
          rxring_cur_work = NMRING_NEXT(rxr, rxring_cur_work);
          if (persistent_flag & PROC_NEED_SEND){
              txring_cur_work = NMRING_NEXT(txr, txring_cur_work);
          }
          persistent_flag &= ~PROC_NEED_SEND;
          ret ++;
      }
      NMRING(rxr)->head = NMRING(rxr)->cur = rxring_cur_work;
      NMRING(txr)->head = NMRING(txr)->cur = txring_cur_work;
      if (MIXIPGW_TOOLS::Module::VERBOSE() && ret > 0){
          MIXIPGW_TOOLS::Logger::LOGINF("sent %d packets to %p", ret, txr);
      }
      ph->persistent_flag_ = persistent_flag;
      return (ret);
  }

};// namespace MIXIPGW_TOOLS

