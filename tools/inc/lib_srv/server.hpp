//
// Created by mixi on 29/nov/16.
//

#ifndef MIXIPGW_TOOLS_SERVER_H
#define MIXIPGW_TOOLS_SERVER_H

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/ref.hpp>


namespace BAIP = boost::asio::ip;
namespace MIXIPGW_TOOLS{
  // ----------
  class Conn;
  class ConnInterface;
  class Server;
  // ----------
  class Event{
  public:
      virtual int on_hook_gtpc(ConnInterface*,void*,size_t) = 0;
      virtual int on_notice(ConnInterface*,void*) { return(RETOK); }
      virtual int on_diam(void*,int)  { return(RETOK); }
      virtual int on_signal(int)      { return(RETOK); }
      virtual int server_port(void)   { return(port_); }
      virtual const char* server_if(void){ return(bindif_.c_str()); }
      virtual const char* server_mcastif(void){ return(NULL); }
  protected:
      int         port_;
      std::string bindif_;
      void*       tp_;
  }; // class Event
  // ----------
  class ConnInterface {
  public:
      virtual BAIP::udp::socket* client_side_sock(void) = 0;
      virtual int write(void* bf, size_t size) = 0;
      virtual boost::asio::io_service *ioservice(void) = 0;
      virtual int address(std::string&,uint16_t&,std::string&,uint16_t&) = 0;
      virtual BAIP::udp::endpoint* remote_endp(void){ return(NULL); }
      virtual void setudpendp(const char*,uint16_t) { return; }
  }; // class ConnInterface
  // ----------
  class EmptyEvent;
  class Conn: public ConnInterface {
      friend class Server;
  public:
      static Conn* create_server_side_connection(boost::asio::io_service&, Event*,BAIP::udp::socket*);
      static Conn* create_client_side_connection(boost::asio::io_service&, const char*, int);
      static Conn* create_client_side_connection_with_localport(boost::asio::io_service&, const char*, int, const char*, int);
      static Conn* create_client_side_connection(boost::asio::io_service&, int, BAIP::udp::endpoint&);
      static int sendudp(boost::asio::io_service&,const char*,int,void*,size_t);
      static void release_instance(ConnInterface**);
  public:
      virtual ~Conn();
      //
      BAIP::udp::endpoint& udpendp(void);

      char* rbuf(void);
      size_t rsiz(void);
      void setudpendp(BAIP::udp::endpoint*);
      void setudpendp(const char*, const char*);
      void start();
      //
      virtual BAIP::udp::socket* client_side_sock(void){ return(sock_ref_); }
      virtual int write(void* bf, size_t size);
      virtual boost::asio::io_service *ioservice(void);
      virtual int address(std::string&,uint16_t&,std::string&,uint16_t&);
      virtual BAIP::udp::endpoint* remote_endp(void){ return(&udpendp_remote_); }
      virtual void setudpendp(const char*, uint16_t);
      int  write_sync(void*, size_t);
  private:
      Conn(boost::asio::io_service&, Event*);
      Conn(){}

      void handle_write(const boost::system::error_code& err);
      void handle_read(BAIP::udp::endpoint& endp,const boost::system::error_code& err, size_t size);
      //
      BAIP::udp::socket   *client_side_sock_;
      BAIP::udp::socket   *sock_ref_;
      boost::asio::io_service *ioservice_;
      BAIP::udp::endpoint udpendp_;
      BAIP::udp::endpoint udpendp_remote_;
      Event               *event_;
      EmptyEvent          *empty_event_;
      char                readbuf_[2048];
      char                writebuf_[2048];
  }; // class Conn
  typedef void (*release_conninterface)(ConnInterface**);

  // -----
  class Server {
  public:
      Server(boost::asio::io_service& io_service, Event* event);
  private:
      void start();
      void handle_udp(Conn* pcon, const boost::system::error_code& err, std::size_t);
      static void userTasks(int);
  private:
      BAIP::udp::socket         udpsock_;
      boost::asio::io_service&  ios_;
      BAIP::udp::endpoint       udpendp_;
      Event*                    event_;
      int                       port_;
      static Server*            server_;
      static uint32_t           moduleid_;
  }; // class Server

  // --------
  class HttpClient{
  public:
      HttpClient(boost::asio::io_service& ,const std::string& ,const std::string& , const std::string& , const std::string& , const std::string& );
  public:
      std::string responsed_contents(void){ return(response_contents_); }
      int responsed_statuscode(void) { return(response_statuscode_); }
      int responsed_complete(void) { return(response_complete_); }
  private:
      void handle_resolve(const boost::system::error_code&, BAIP::tcp::resolver::iterator);
      void handle_connect(const boost::system::error_code&, BAIP::tcp::resolver::iterator);
      void handle_write_request(const boost::system::error_code&);
      void handle_read_status_line(const boost::system::error_code&);
      void handle_read_headers(const boost::system::error_code&);
      void handle_read_content(const boost::system::error_code&);
  private:
      BAIP::tcp::resolver         resolver_;
      BAIP::tcp::socket           socket_;
      boost::asio::streambuf      request_;
      boost::asio::streambuf      response_;
      std::vector<std::string>    response_header_;
      std::string                 response_contents_;
      int                         response_contentlen_;
      int                         response_statuscode_;
      int                         response_complete_;
  }; // class Client
}; // namespace MIXIPGW_TOOLS

#endif //MIXIPGW_TOOLS_SERVER_H
