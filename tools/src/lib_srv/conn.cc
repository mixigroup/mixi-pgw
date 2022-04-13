//
// Created by mixi on 29/nov/16.
//
#include "mixipgw_tools_def.h"
#include "lib_srv/server.hpp"
#include "lib/logger.hpp"
#include "lib/module.hpp"

using namespace MIXIPGW_TOOLS;

namespace MIXIPGW_TOOLS{
  //
  class EmptyEvent:public Event{
  public:
      EmptyEvent(int port):port_(port){}
      virtual ~EmptyEvent(){}
  private:
      int port_;
  public:
      virtual int on_hook_gtpc(ConnInterface*,void*,size_t){ return(RETERR); }
      virtual int on_notice(ConnInterface*,void*){ return(RETOK); }
      virtual int on_diam(void*,int){ return(RETOK); }
      virtual int on_signal(int){ return(RETOK); }
      virtual int server_port(void){ return(port_); }
  }; // class EmptyEvent
  //
  Conn* Conn::create_server_side_connection(boost::asio::io_service& ios, Event* event,BAIP::udp::socket* sock){
      Conn* pcon = new Conn(ios, event);
      if (Module::VERBOSE() > PCAPLEVEL_DBG){
          Logger::LOGINF("Conn::create_server_side_connection.(%p) : new pointer(%p)", sock, pcon);
      }
      if (pcon){
          if (Module::VERBOSE() > PCAPLEVEL_DBG){
              Logger::LOGINF("Conn::create_server_side_connection. setup(%p)", sock);
          }
          pcon->sock_ref_ = sock;
      }
      return(pcon);
  }

  Conn* Conn::create_client_side_connection(boost::asio::io_service& ios, int port, BAIP::udp::endpoint& endp){
      EmptyEvent* pev = new EmptyEvent(port);
      Conn* pcon = new Conn(ios, pev);
      if (pcon){
          pcon->empty_event_ = pev;
          pcon->client_side_sock_ = new BAIP::udp::socket(ios, endp);
          if (pcon->client_side_sock_){
              pcon->client_side_sock_->set_option(boost::asio::ip::udp::socket::send_buffer_size(4*1024*1024));
              pcon->client_side_sock_->set_option(boost::asio::ip::udp::socket::reuse_address(true));
              pcon->client_side_sock_->set_option(boost::asio::ip::unicast::hops(255));
              pcon->sock_ref_ = pcon->client_side_sock_;
          }
          Logger::LOGINF("Conn::create_client_side_connection.(%p) new pointer(%p)", pcon->client_side_sock_, pcon);
      }else{
          Logger::LOGERR("failed. allocate Conn");
      }
      //
      return(pcon);
  }

  Conn* Conn::create_client_side_connection(boost::asio::io_service& ios, const char* host, int port){
      EmptyEvent* pev = new EmptyEvent(port);
      Conn* pcon = new Conn(ios, pev);
      if (pcon){
          pcon->empty_event_ = pev;
          pcon->client_side_sock_ = new BAIP::udp::socket(ios);
          if (pcon->client_side_sock_){
              pcon->client_side_sock_->open(BAIP::udp::v4());
              pcon->client_side_sock_->set_option(boost::asio::ip::udp::socket::send_buffer_size(4*1024*1024));
              pcon->client_side_sock_->set_option(boost::asio::ip::udp::socket::reuse_address(true));
              pcon->client_side_sock_->set_option(boost::asio::ip::unicast::hops(255));
              pcon->sock_ref_ = pcon->client_side_sock_;
          }
          if (Module::VERBOSE() > PCAPLEVEL_DBG){
              Logger::LOGINF("Conn::create_client_side_connection.(%p:%s:%d): new pointer(%p)", pcon->client_side_sock_, host, port, pcon);
          }
          pcon->setudpendp(host, port);
      }else{
          Logger::LOGERR("failed. allocate Conn");
      }
      //
      return(pcon);
  }
  Conn* Conn::create_client_side_connection_with_localport(boost::asio::io_service& ios, const char* host, int port, const char* localif, int localport){
      EmptyEvent* pev = new EmptyEvent(port);
      Conn* pcon = new Conn(ios, pev);
      if (pcon){
          pcon->client_side_sock_ = new BAIP::udp::socket(ios);
          if (pcon->client_side_sock_){
              BAIP::udp::endpoint endp(boost::asio::ip::address::from_string(localif), localport);
              pcon->client_side_sock_->open(endp.protocol());
              pcon->client_side_sock_->set_option(boost::asio::ip::udp::socket::send_buffer_size(4*1024*1024));
              pcon->client_side_sock_->set_option(boost::asio::ip::udp::socket::reuse_address(true));
              pcon->client_side_sock_->set_option(boost::asio::ip::unicast::hops(255));
              pcon->client_side_sock_->bind(endp);
              pcon->sock_ref_ = pcon->client_side_sock_;
          }
          if (Module::VERBOSE() > PCAPLEVEL_DBG){
              Logger::LOGINF("Conn::create_client_side_connection.(%p) new pointer(%p)", pcon->client_side_sock_, pcon);
          }
          pcon->setudpendp(host, port);
      }else{
          Logger::LOGERR("failed. allocate Conn");
      }
      //
      return(pcon);
  }
  int Conn::sendudp(boost::asio::io_service& ios,const char* host,int port,void* data,size_t len){
      std::auto_ptr<Conn> pcon(Conn::create_client_side_connection(ios, host, port));
      return(pcon.get()->write_sync(data, len));
  }
  void Conn::release_instance(ConnInterface** ppcon){
      if (*ppcon != NULL){
          delete ((Conn*)(*ppcon));
      }
      *ppcon = NULL;
  }

  Conn::Conn(boost::asio::io_service& ios, Event* event):
          client_side_sock_(NULL),
          sock_ref_(NULL),
          ioservice_(&ios),
          event_(event),
          empty_event_(NULL){
      memset(writebuf_,0,sizeof(writebuf_));
      memset(readbuf_,0,sizeof(readbuf_));
  }
  Conn::~Conn(){
      if (Module::VERBOSE() > PCAPLEVEL_DBG){
          Logger::LOGINF("Conn::~Conn(%p)", this);
      }
      if (empty_event_ != NULL){
          delete empty_event_;
      }
      empty_event_ = NULL;
      if (client_side_sock_ != NULL){
          client_side_sock_->close();
          delete client_side_sock_;
      }
      client_side_sock_ = NULL;
  }
  BAIP::udp::endpoint& Conn::udpendp(void){ return(udpendp_); }
  boost::asio::io_service *Conn::ioservice(void){ return(ioservice_); }
  char* Conn::rbuf(void){ return(readbuf_); }
  size_t Conn::rsiz(void){ return(sizeof(readbuf_)); }
  //
  void Conn::setudpendp(BAIP::udp::endpoint* pendp){
      udpendp_ = *pendp;
      if (Module::VERBOSE() > PCAPLEVEL_DBG){
          Logger::LOGINF("Conn::setudpendp.(%p)", pendp);
      }
  }
  void Conn::setudpendp(const char* host, uint16_t port){
      char bf[32] = {0};
      snprintf(bf, sizeof(bf)-1,"%u", port);
      setudpendp(host, bf);
  }
  void Conn::setudpendp(const char* host, const char* port){
      BAIP::udp::resolver     resolver(*ioservice_);
      BAIP::udp::resolver::query query(BAIP::udp::v4(), host, port);
      BAIP::udp::resolver::iterator iter = resolver.resolve(query);
      //
      udpendp_ = (*iter);
      if (Module::VERBOSE() > PCAPLEVEL_DBG){
          Logger::LOGINF("Conn::setudpendp.(%s : %s)", host, port);
      }
  }
  int Conn::write(void* bf, size_t size){
      return(write_sync(bf, size));
  }
  int  Conn::write_sync(void* bf, size_t size){
      size_t s;
      if (size <= sizeof(writebuf_)){
          memcpy(writebuf_, bf, size);
          s = sock_ref_->send_to(boost::asio::buffer(writebuf_,size), udpendp_);
  //      Logger::LOGINF("Conn:write_sync::send_to(%u) %s",s, strerror(errno));
          return(RETOK);
      }
      Logger::LOGERR("write_sync failed.(%d : %d)", size, sizeof(writebuf_));
      return(RETWRN);
  }
  int Conn::address(std::string& host,uint16_t& port,std::string& lhost,uint16_t& lport){
      try{
          if (Module::VERBOSE() > PCAPLEVEL_DBG){
              Logger::LOGINF("Conn::address(local  : %s %u)",sock_ref_->local_endpoint().address().to_string().c_str(), sock_ref_->local_endpoint().port());
              Logger::LOGINF("Conn::address(remote : %s %u)",udpendp_remote_.address().to_string().c_str(), udpendp_remote_.port());
          }
      }catch( std::exception& e ){
          fprintf(stderr, "\nConn::address...%s\n",e.what());
      }
      host = udpendp_remote_.address().to_string();
      port = udpendp_remote_.port();

      lhost = sock_ref_->local_endpoint().address().to_string();
      lport = sock_ref_->local_endpoint().port();
      return(RETOK);
  }
  void Conn::handle_write(const boost::system::error_code& err) {
      Logger::LOGINF("handle_write/con : %08X /%x", this, err);
      // release after completed transaction.
      delete this;
  }
  void Conn::handle_read(BAIP::udp::endpoint& endp, const boost::system::error_code& err, size_t size) {
      if (Module::VERBOSE() > PCAPLEVEL_DBG){
          Logger::LOGINF("Conn::handle_read(%u)",size);
      }
      if (!err){
          udpendp_remote_ = endp;
          if (event_){
              if (event_->on_hook_gtpc(this, readbuf_, size) == RETOK){
                  return;
              }
          }
          if (size == sizeof(notification_item_packed_t) ){
              if (event_){
                  if (event_->on_notice(this, (notification_item_packed_ptr)readbuf_) == RETOK){
                      return;
                  }
              }
          }
      }else{
          Logger::LOGERR("handle_read/con : %08X /%x", this, err);
      }
  }
}; // namespace MIXIPGW_TOOLS
