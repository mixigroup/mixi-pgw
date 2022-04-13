//
// Created by mixi on 29/nov/16.
//

#include "mixipgw_tools_def.h"
#include "lib/logger.hpp"
#include "lib/module.hpp"
#include "lib_srv/server.hpp"
//
namespace MIXIPGW_TOOLS{

  Server* Server::server_ = NULL;
  static pthread_mutex_t sockinit_mtx = PTHREAD_MUTEX_INITIALIZER;

  Server::Server(boost::asio::io_service& io_service, Event* event) :
          udpsock_(io_service),
          ios_(io_service),
          event_(event){
      pthread_mutex_lock(&sockinit_mtx);
      Server::server_ = this;
      //
      signal(SIGUSR1, Server::userTasks);
      signal(SIGUSR2, Server::userTasks);
      //
      port_ = event->server_port();
      BAIP::udp::endpoint endpoint(boost::asio::ip::address::from_string(event->server_if()), port_);
      Logger::LOGINF("address::from_string(if: %s/port: %u)", event->server_if(),port_);
      udpsock_.open(endpoint.protocol());
      udpsock_.set_option(boost::asio::ip::udp::socket::reuse_address(true));
      // no definition in boost asio, set to native desctiptor
#ifndef __APPLE__
      int disable = 1;
      int len = sizeof(disable);
      auto ret = setsockopt(udpsock_.native(), SOL_SOCKET, SO_NO_CHECK, (void*)&disable, (socklen_t)len);
      Logger::LOGINF("setsockopt:nochekc(%d: %s)",ret, strerror(ret));
      ret = getsockopt(udpsock_.native(), SOL_SOCKET, SO_NO_CHECK, (void*)&disable, (socklen_t*)&len);

      Logger::LOGINF("getsockopt        (%d: %d: %s)", disable, ret, strerror(ret));
#endif
      if (!event->server_mcastif()){
          udpsock_.set_option(boost::asio::ip::udp::socket::receive_buffer_size(4*1024*1024));
      }
      udpsock_.bind(endpoint);
      // multicast address
      if (event->server_mcastif()){
          boost::asio::ip::address addr(boost::asio::ip::address::from_string(event->server_mcastif()));
          udpsock_.set_option( boost::asio::ip::multicast::join_group(addr));
      }
      //
      start();
      pthread_mutex_unlock(&sockinit_mtx);
  }
  void Server::start() {
      if (Module::VERBOSE() > PCAPLEVEL_DBG){
          Logger::LOGINF("Server::start(%u)",port_);
      }
      Conn* pConn =  Conn::create_server_side_connection(udpsock_.get_io_service(),event_,&udpsock_);
      udpsock_.async_receive_from(boost::asio::buffer(pConn->rbuf(), pConn->rsiz()),udpendp_,
                                  boost::bind(&Server::handle_udp, this, pConn,boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
  }
  void Server::handle_udp(Conn* pcon, const boost::system::error_code& err, std::size_t size){
      if (Module::VERBOSE() > PCAPLEVEL_DBG){
          Logger::LOGINF("Server::handle_udp(%u)",size);
      }
      if (!err) {
          pcon->handle_read(udpendp_, err, size);
      }
      if(pcon){
          delete pcon;
      }
      pcon = NULL;
      start();
  }
  void Server::userTasks(int signo){
      Logger::LOGINF("userTasks:%d/%0x", signo, Server::server_);
      if (Server::server_ != NULL){
          if (Server::server_->event_ != NULL){
              Server::server_->event_->on_signal(signo);
          }
      }
  }
}; // namespace MIXIPGW_TOOLS
