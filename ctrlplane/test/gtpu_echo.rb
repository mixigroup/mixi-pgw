#!/usr/bin/ruby

require "socket"



udp = UDPSocket.open()
udp.bind("192.168.56.100", 2123)
sockaddr = Socket.pack_sockaddr_in(2123, "192.168.56.1")

msg = [
  0x30, 0x02, 0x00, 0x08,  0x00, 0x00, 0x00, 0x00
].pack("C*")



10.times do |i|
  r = udp.send(msg, 0, sockaddr)
  sleep(1)
end

udp.close