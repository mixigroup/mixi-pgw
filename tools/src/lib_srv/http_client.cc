//
// Created by mixi on 2017/02/14.
//

#include "mixipgw_tools_def.h"
#include "lib/logger.hpp"
#include "lib_srv/server.hpp"

namespace MIXIPGW_TOOLS{
  HttpClient::HttpClient(boost::asio::io_service& io_service,const std::string& method,const std::string& server, const std::string& path, const std::string& request, const std::string& port) :
          resolver_(io_service),socket_(io_service),response_contentlen_(0),response_statuscode_(0),
          response_complete_(RETERR){
      std::ostream request_stream(&request_);
      request_stream << method << " " << path << " HTTP/1.0\r\n";
      request_stream << "Host: " << server << "\r\n";
      request_stream << "Accept: */*\r\n";
      request_stream << "Content-Length:" << request.length() << "\r\n";
      request_stream << "Connection: close\r\n\r\n";
      request_stream << request << "\r\n";
      //
      BAIP::tcp::resolver::query query(server, port);
      resolver_.async_resolve(query,
                              boost::bind(&HttpClient::handle_resolve, this,
                                          boost::asio::placeholders::error,
                                          boost::asio::placeholders::iterator));
  }
  void HttpClient::handle_resolve(const boost::system::error_code& err, BAIP::tcp::resolver::iterator endpoint_iterator) {
      if (!err) {
          BAIP::tcp::endpoint endpoint = *endpoint_iterator;
          socket_.async_connect(endpoint,boost::bind(&HttpClient::handle_connect, this,
                                                     boost::asio::placeholders::error, ++endpoint_iterator));
      }else{
          Logger::LOGERR("handle_resolve(%s)%s:%u", err.message().c_str(),__FILE__,__LINE__);
      }
  }
  void HttpClient::handle_connect(const boost::system::error_code& err, BAIP::tcp::resolver::iterator endpoint_iterator){
      if (!err) {
          boost::asio::async_write(socket_, request_, boost::bind(&HttpClient::handle_write_request, this,
                                                                  boost::asio::placeholders::error));
      }else if (endpoint_iterator != BAIP::tcp::resolver::iterator()) {
          socket_.close();
          BAIP::tcp::endpoint endpoint = *endpoint_iterator;
          socket_.async_connect(endpoint,boost::bind(&HttpClient::handle_connect, this,
                                                     boost::asio::placeholders::error, ++endpoint_iterator));
      } else {
          Logger::LOGERR("handle_connect code(%s)%s:%u", err.message().c_str(),__FILE__,__LINE__);
      }
  }
  void HttpClient::handle_write_request(const boost::system::error_code& err) {
      if (!err) {
          boost::asio::async_read_until(socket_, response_, "\r\n",
                                        boost::bind(&HttpClient::handle_read_status_line, this,
                                                    boost::asio::placeholders::error));
      } else {
          Logger::LOGERR("handle_write_request(%s)", err.message().c_str());
      }
  }
  void HttpClient::handle_read_status_line(const boost::system::error_code& err){
      if (!err){
          //
          std::istream response_stream(&response_);
          std::string http_version;
          response_stream >> http_version;
          response_stream >> response_statuscode_;
          std::string status_message;
          std::getline(response_stream, status_message);
          if (!response_stream || http_version.substr(0, 5) != "HTTP/"){
              Logger::LOGERR("Invalid response");
              return;
          }
          if (response_statuscode_ != 200){
              Logger::LOGERR("Response returned with status code(%d)", response_statuscode_);
              return;
          }
          //
          boost::asio::async_read_until(socket_, response_, "\r\n\r\n", boost::bind(&HttpClient::handle_read_headers,
                                                                                    this, boost::asio::placeholders::error));
      }else{
          Logger::LOGERR("handle_read_status_line(%s)", err.message().c_str());
      }
  }
  void HttpClient::handle_read_headers(const boost::system::error_code& err){
      if (!err){
          std::istream response_stream(&response_);
          std::string header;
          const char* ckey = "Content-Length: ";
          size_t ckeylen = strlen(ckey);
          while (std::getline(response_stream, header) && header != "\r"){
              // contents length
              if (header.length() > ckeylen && strncasecmp(header.c_str(), ckey, ckeylen) == 0){
                  response_contentlen_ = atoi(header.substr(ckeylen).c_str());
              }
              response_header_.push_back(header);
          }
          if (response_.size() > 0){
              if (response_.size() >= response_contentlen_){
                  response_contents_ += std::string(boost::asio::buffer_cast<const char*>(response_.data()));
                  response_complete_ = RETOK;
              }
          }
          boost::asio::async_read(socket_, response_, boost::asio::transfer_at_least(1),
                                  boost::bind(&HttpClient::handle_read_content, this, boost::asio::placeholders::error));
      } else {
          Logger::LOGERR("handle_read_headers(%u)", err);
      }
  }
  void HttpClient::handle_read_content(const boost::system::error_code& err){
      if (!err && response_.size() > 0){
          response_contents_ += std::string(boost::asio::buffer_cast<const char*>(response_.data()));
          boost::asio::async_read(socket_, response_,boost::asio::transfer_at_least(1),
                                  boost::bind(&HttpClient::handle_read_content, this,boost::asio::placeholders::error));
      }else if (err != boost::asio::error::eof){
          Logger::LOGERR("handle_read_content(%s)", err.message().c_str());
      }
  }
}; // namespace MIXIPGW_TOOLS
