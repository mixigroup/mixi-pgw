//
// Created by mixi on 2017/04/27.
//

#ifndef MIXIPGW_TOOLS_BFD_HPP
#define MIXIPGW_TOOLS_BFD_HPP

#include "mixipgw_tools_def.h"
#include "lib/const/bfd.h"

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/ref.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>



namespace BAIP = boost::asio::ip;
namespace MIXIPGW_TOOLS{
  class BfdSession{
  public:
      BfdSession(uint32_t , uint16_t );
      BfdSession(const BfdSession& );
      BfdSession();
  public:
      uint32_t    session_state_;
      uint32_t    remote_session_state_;
      uint32_t    local_discr_;
      uint32_t    remote_discr_;
      uint32_t    local_diag_;
      uint32_t    desired_min_tx_interval_;
      uint32_t    required_min_rx_interval_;
      uint32_t    remote_min_rx_interval_;
      uint32_t    demand_mode_;
      uint32_t    remote_demand_mode_;
      uint32_t    detect_mult_;
      uint32_t    auth_type_;
      uint32_t    rcv_auth_seq_;
      uint32_t    xmit_auth_seq_;
      uint32_t    auth_seq_known_;
      //
      uint32_t    must_cease_tx_echo_;
      uint32_t    must_terminate_;
      uint64_t    detect_time_;
      uint32_t    pollbit_on_;
      struct in_addr  peer_addr_;
      uint16_t    peer_port_;
      uint32_t    received_min_rx_interval_;
      ConnInterface*       conn_;
      release_conninterface freefunc_;
  };// class BfdSession
  //
  typedef std::map<ULONGLONG, BfdSession>           SESSION;
  typedef std::map<ULONGLONG, BfdSession>::iterator SESSIONITR;
  //
  class BfdSessionManager{
  public:
      BfdSessionManager(uint64_t);
      virtual ~BfdSessionManager();
  public:
      boost::mutex* mutex(void);
      void subscribe(uint32_t , uint16_t );
      void subscribe_unsafe(uint32_t , uint16_t );
      void unsubscribe(uint32_t , uint16_t );
      BfdSession* find_unsafe(uint32_t , uint16_t );
      void transmit_unsafe(BfdSession* );
      int sendudp(BfdSession* ,void* ,size_t );
      void on_bfd_thread(uint64_t delay);
      int on_bfd_recieve(uint32_t , uint16_t ,void* , size_t , void* );
  private:
      boost::mutex        mutex_;
      boost::thread_group threads_;
      boost::asio::io_service ios_;
      SESSION             bfd_sessions_;
  }; // class BfdSessionManager
};// namespace MIXIPGW_TOOLS


#endif //MIXIPGW_TOOLS_BFD_HPP
