#!/usr/bin/ruby

require "socket"

udp = UDPSocket.open()
gtpc_addr = Socket.pack_sockaddr_in(2123, "127.0.0.1")

gtpc_delete_bearer_req = [
  0x48,0x63,0x00,0x0d,
  0x77,0x88,0x99,0xaa,
  0x03,0xc0,0x08,0x00,
  0x49,0x00,0x01,0x00,0x05

].pack("C*")

r = udp.send(gtpc_delete_bearer_req, 0, gtpc_addr)
print "gtpc_delete_bearer_req : #{r}\n"

udp.close
