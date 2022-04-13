//
// Created by mixi on 2017/04/27.
//
#include "mixipgw_tools_def.h"
#include "lib_srv/server.hpp"
#include "lib_srv/bfd.hpp"
#include "lib/logger.hpp"
#include "lib/module.hpp"
#include "lib/misc.hpp"
#include "lib/const/bfd.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/with_lock_guard.hpp>

namespace MIXIPGW_TOOLS{
  static uint32_t local_discriminator_incr = 0;
  //
  BfdSession::BfdSession(uint32_t ipaddr, uint16_t port):BfdSession(){
      memcpy(&peer_addr_.s_addr, &ipaddr, sizeof(uint32_t));
      peer_port_ = port;
  }
  BfdSession::BfdSession():session_state_(BFDSTATE_DOWN),remote_session_state_(BFDSTATE_DOWN),local_discr_(),remote_discr_(BFD_OFF),
                           local_diag_(BFD_OFF),desired_min_tx_interval_(BFDDFLT_DESIREDMINTX),required_min_rx_interval_(BFDDFLT_REQUIREDMINRX),remote_min_rx_interval_(BFD_OFF),
                           demand_mode_(BFD_OFF),remote_demand_mode_(BFD_OFF),detect_mult_(BFDDFLT_DETECTMULT),auth_type_(BFD_OFF),rcv_auth_seq_(BFD_OFF),xmit_auth_seq_(BFD_OFF),
                           auth_seq_known_(BFD_OFF),must_cease_tx_echo_(BFD_OFF),must_terminate_(BFD_OFF),peer_port_(BFD_OFF),peer_addr_(),detect_time_(0),pollbit_on_(BFD_OFF),
                           received_min_rx_interval_(BFD_OFF),conn_(NULL),freefunc_(NULL){}

  BfdSession::BfdSession(const BfdSession& cpy){
      session_state_          = cpy.session_state_;
      remote_session_state_   = cpy.remote_session_state_;
      local_discr_            = cpy.local_discr_;
      remote_discr_           = cpy.remote_discr_;
      local_diag_             = cpy.local_diag_;
      desired_min_tx_interval_= cpy.desired_min_tx_interval_;
      required_min_rx_interval_=cpy.required_min_rx_interval_;
      remote_min_rx_interval_ = cpy.remote_min_rx_interval_;
      demand_mode_            = cpy.demand_mode_;
      remote_demand_mode_     = cpy.remote_demand_mode_;
      detect_mult_            = cpy.detect_mult_;
      auth_type_              = cpy.auth_type_;
      rcv_auth_seq_           = cpy.rcv_auth_seq_;
      xmit_auth_seq_          = cpy.xmit_auth_seq_;
      auth_seq_known_         = cpy.auth_seq_known_;
      must_cease_tx_echo_     = cpy.must_cease_tx_echo_;
      must_terminate_         = cpy.must_terminate_;
      detect_time_            = cpy.detect_time_;
      peer_addr_              = cpy.peer_addr_;
      peer_port_              = cpy.peer_port_;
      pollbit_on_             = cpy.pollbit_on_;
      received_min_rx_interval_=cpy.received_min_rx_interval_;
      conn_                   = cpy.conn_;
      freefunc_               = cpy.freefunc_;
  }
  //
  BfdSessionManager::BfdSessionManager(uint64_t delay){
      threads_.create_thread(boost::bind(&BfdSessionManager::on_bfd_thread, this, delay));
  }
  BfdSessionManager::~BfdSessionManager(){
      threads_.join_all();
  }
  boost::mutex* BfdSessionManager::mutex(void){
      return(&mutex_);
  }
  void BfdSessionManager::subscribe(uint32_t ipaddr, uint16_t port){
      boost::unique_lock<boost::mutex> lock(mutex_);
      subscribe_unsafe(ipaddr, port);
  }
  void BfdSessionManager::subscribe_unsafe(uint32_t ipaddr, uint16_t port){
      bfd_sessions_[MAKE_ULL(ipaddr, port)] = BfdSession(ipaddr, port);
  }
  void BfdSessionManager::unsubscribe(uint32_t ipaddr, uint16_t port){
      boost::unique_lock<boost::mutex> lock(mutex_);
      SESSIONITR  itr = bfd_sessions_.find(MAKE_ULL(ipaddr, port));
      if (itr != bfd_sessions_.end()){
          if (((itr->second).conn_) != NULL && (itr->second).freefunc_ != NULL){
              (itr->second).freefunc_(&(itr->second).conn_);
          }
          bfd_sessions_.erase(itr);
      }
  }
  BfdSession* BfdSessionManager::find_unsafe(uint32_t ipaddr, uint16_t port){
      SESSIONITR  itr;
      if ((itr = bfd_sessions_.find(MAKE_ULL(ipaddr, port))) == bfd_sessions_.end()){ return(NULL); }
      return(&(itr->second));
  }
  void BfdSessionManager::transmit_unsafe(BfdSession* psess){
      if (Module::VERBOSE() > PCAPLEVEL_DBG){
          Logger::LOGINF("transmit_unsafe(%p/ %p)", pthread_self(), psess);
      }
      bfd_t   bfd;
      memset(&bfd, 0,sizeof(bfd));
      bfd.h.head.vers   = BFD_VERSION;
      bfd.h.head.diag   = psess->local_diag_;
      if (psess->local_discr_!=0){
          bfd.u.bit.state  = psess->session_state_;
      }else{
          bfd.u.bit.state  = BFDSTATE_DOWN;
      }

      if (psess->pollbit_on_ == BFD_ON){
          bfd.u.bit.poll   = BFD_OFF;
          bfd.u.bit.final  = BFD_ON;
      }else{
          bfd.u.bit.poll   = BFD_OFF;
          bfd.u.bit.final  = BFD_OFF;
      }
      bfd.u.bit.cpi = BFD_ON;
      bfd.u.bit.auth = BFD_OFF;
      bfd.u.bit.demand = BFD_OFF;
      bfd.u.bit.multipoint = BFD_OFF;
      bfd.detect_mult = psess->detect_mult_;
      bfd.length = sizeof(bfd);
      bfd.my_discr = psess->local_discr_;
      bfd.your_discr = psess->remote_discr_;
      bfd.min_tx_int = htonl(psess->desired_min_tx_interval_);
      bfd.min_rx_int = htonl(psess->required_min_rx_interval_);
      bfd.min_echo_rx_int = htonl(psess->received_min_rx_interval_);
      if ((bfd.your_discr == 0 || bfd.my_discr == 0) && bfd.u.bit.state != BFDSTATE_DOWN){
          Logger::LOGERR("do not sendudp.(%u/ %u)", bfd.your_discr, bfd.my_discr);
          return;
      }
      //
      sendudp(psess, &bfd, sizeof(bfd));
  }
  int BfdSessionManager::sendudp(BfdSession* psess,void* data,size_t len){
      char host[32] = {0};
      uint32_t addr = psess->peer_addr_.s_addr;
      snprintf(host,sizeof(host)-1,"%u.%u.%u.%u",
               ((uint8_t*)&addr)[3],
               ((uint8_t*)&addr)[2],
               ((uint8_t*)&addr)[1],
               ((uint8_t*)&addr)[0]);
      if (psess->conn_ != NULL){
          if (Module::VERBOSE() > PCAPLEVEL_DBG){
              Logger::LOGINF("setudp endpoint.(%s) %u", host, BFDDFLT_UDPPORT);
          }
          psess->conn_->setudpendp(host, BFDDFLT_UDPPORT);
          return(psess->conn_->write(data, len));
      }
      Logger::LOGERR("sendudp(%s: %u)",host, len);
      return(RETERR);
  }
  void BfdSessionManager::on_bfd_thread(uint64_t delay){
      Logger::LOGINF("BfdSessionManager::on_bfd_thread(" FMT_LLU ")", delay);
      //
      while (!Module::ABORT()) {
          usleep(delay);
          uint64_t curms = Misc::GetMicrosecondArround();
          {   boost::unique_lock<boost::mutex> lock(mutex_);
              SESSIONITR  itr;

              if (Module::VERBOSE() > PCAPLEVEL_DBG){
                  Logger::LOGINF("on_bfd_thread...(" FMT_LLU "/" FMT_LLU ")", curms, bfd_sessions_.size());
              }
              for(itr = bfd_sessions_.begin();itr != bfd_sessions_.end();++itr){
                  if ((itr->second).detect_time_ > curms){
                      if (Module::VERBOSE() > PCAPLEVEL_DBG){
                          Logger::LOGINF("detect : " FMT_LLU "/" FMT_LLU ": tx:%u term:%u discr:%u rx:%u"
                                  ,(itr->second).detect_time_, curms
                                  ,(itr->second).must_cease_tx_echo_
                                  ,(itr->second).must_terminate_
                                  ,(itr->second).remote_discr_
                                  ,(itr->second).remote_min_rx_interval_
                          );
                      }
                      if ((itr->second).must_cease_tx_echo_ == BFD_OFF &&
                          (itr->second).must_terminate_ == BFD_OFF &&
                          //                                  (itr->second).session_state_ == BFDSTATE_UP &&
                          (itr->second).remote_discr_ != 0 &&
                          (itr->second).remote_min_rx_interval_ != 0){
                          // BFD protocol , transmit..
                          transmit_unsafe(&(itr->second));
                      }
                  }
              }
          }
      }
      Logger::LOGINF("BfdSessionManager::on_bfd_thread. completed(%p)", pthread_self());
  }
  int BfdSessionManager::on_bfd_recieve(uint32_t ipaddr, uint16_t port,void* payload, size_t len, void* bfdev){
      bfd_ptr bfd = (bfd_ptr)(payload);
      BfdSession* psess = NULL;
      if (len != BFD_MINPKTLEN){ return(RETOK); }
      if (bfd->u.bit.multipoint != BFD_OFF) { return(RETOK); }  // unsupported multi point.
      if (bfd->h.head.vers != BFD_VERSION) { return(RETOK); }
      if (bfd->u.bit.auth != BFD_OFF) { return(RETOK); }        // unsupported auth.
      if (bfd->my_discr == BFD_OFF) { return(RETOK); }
      if (bfd->detect_mult == BFD_OFF) { return(RETOK); }
      if (bfd->your_discr == BFD_OFF && !(bfd->u.bit.state == BFDSTATE_DOWN || bfd->u.bit.state == BFDSTATE_ADMINDOWN)){
          return(RETOK);
      }

      if (bfd->your_discr != BFD_OFF){
          boost::unique_lock<boost::mutex> lock(*mutex());
          if (find_unsafe(ipaddr, port)==NULL){
              subscribe_unsafe(ipaddr, port);
              if ((psess = find_unsafe(ipaddr,port)) != NULL){
                  psess->remote_discr_ = bfd->my_discr;
                  psess->local_discr_  = bfd->your_discr;
                  psess->session_state_= BFDSTATE_DOWN;
                  transmit_unsafe(psess);
              }
              return(RETOK);
          }
      }
      boost::with_lock_guard(*mutex(), [&]{
          if ((psess = find_unsafe(ipaddr, port)) == NULL){
              if (bfd->u.bit.state == BFDSTATE_DOWN){
                  subscribe_unsafe(ipaddr, port);
                  psess = find_unsafe(ipaddr,port);
              }
              if (psess == NULL){ return; }
          }
          //
          if (psess->conn_ == NULL){
              char host[32] = {0};
              snprintf(host,sizeof(host)-1,"%u.%u.%u.%u",
                       ((uint8_t*)&ipaddr)[3],
                       ((uint8_t*)&ipaddr)[2],
                       ((uint8_t*)&ipaddr)[1],
                       ((uint8_t*)&ipaddr)[0]);
              if (Module::VERBOSE() > PCAPLEVEL_DBG){
                  Logger::LOGINF("con: %s : %u", host, BFDDFLT_UDPPORT);
              }
              psess->conn_ = Conn::create_client_side_connection(ios_,host,BFDDFLT_UDPPORT);
              psess->freefunc_ = Conn::release_instance;
              psess->conn_->setudpendp(host, BFDDFLT_UDPPORT);
              psess->local_discr_ = htonl(++local_discriminator_incr);
              if (local_discriminator_incr > 0x7fffffff){
                  local_discriminator_incr = 0;
              }
          }

          psess->remote_session_state_ = bfd->u.bit.state;
          psess->remote_demand_mode_ = bfd->u.bit.demand;
          psess->remote_discr_ = bfd->my_discr;
          psess->remote_min_rx_interval_ = bfd->min_rx_int;
          psess->received_min_rx_interval_ = MIN(psess->received_min_rx_interval_, bfd->min_rx_int);
#if 0 
          if (!bfd->min_echo_rx_int){
              psess->must_cease_tx_echo_ = BFD_ON;
          }
#endif
          if (bfd->u.bit.final){
              psess->must_terminate_ = BFD_ON;
          }
          uint64_t curms = Misc::GetMicrosecondArround();
          psess->detect_time_ = curms + (bfd->detect_mult * MAX(bfd->min_rx_int,psess->desired_min_tx_interval_));

          if (psess->session_state_ == BFDSTATE_ADMINDOWN){
              return;
          }
          if (psess->remote_session_state_ == BFDSTATE_ADMINDOWN){
              if (psess->session_state_ != BFDSTATE_DOWN){
                  psess->local_diag_ = BFDDIAG_NEIGHBORSAIDDOWN;
                  psess->session_state_ = BFDSTATE_DOWN;
              }
          }else{
              if (psess->session_state_ == BFDSTATE_DOWN){
                  if (psess->remote_session_state_ == BFDSTATE_DOWN){
                      psess->session_state_ = BFDSTATE_INIT;
                  }else if (psess->remote_session_state_ == BFDSTATE_INIT){
                      psess->session_state_ = BFDSTATE_UP;
                  }
              }else if (psess->session_state_ == BFDSTATE_INIT){
                  if (psess->remote_session_state_ == BFDSTATE_INIT || psess->remote_session_state_ == BFDSTATE_UP){
                      psess->session_state_ = BFDSTATE_UP;
                  }else{
                      psess->session_state_ = BFDSTATE_DOWN;
                      psess->remote_discr_ = 0;
                  }
              }else{  // session_staete_ == UP
                  if (psess->remote_session_state_ == BFDSTATE_DOWN){
                      psess->local_diag_ = BFDDIAG_NEIGHBORSAIDDOWN;
                      psess->session_state_ = BFDSTATE_DOWN;
                  }
              }
          }
          if (bfd->u.bit.poll == BFD_ON){
              psess->pollbit_on_ = BFD_ON;
              // system must transmit a ctrl packet
              // with poll bit set as soon as practicable.
              transmit_unsafe(psess);
          }
      });
      return(0);
  }
}; // namspace LTEEPC
