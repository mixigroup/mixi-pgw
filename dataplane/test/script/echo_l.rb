#!/usr/bin/ruby

require "socket"
SRCIF = "172.25.0.2"
DSTADR= "172.25.0.3"

udp = UDPSocket.open()
udpu = UDPSocket.open()
udp.bind(SRCIF, 2123)
udpu.bind(SRCIF, 2152)
gtpu_addr = Socket.pack_sockaddr_in(2152, DSTADR)
gtpc_addr = Socket.pack_sockaddr_in(2123, DSTADR)

gtpu_echo_res = [
    0x30, 0x02, 0x00, 0x08,  0x00, 0x00, 0x00, 0x00
].pack("C*")
gtpc_echo_res = [
    0x40, 0x02, 0x00, 0x05,  0x00, 0x00, 0x00, 0x00,
    0x03, 0x00, 0x01, 0x00,  0x00
].pack("C*")
gtpu_echo_req = [
    0x30, 0x01, 0x00, 0x08,  0x00, 0x00, 0x00, 0x00
].pack("C*")
gtpc_echo_req = [
    0x40, 0x01, 0x00, 0x05,  0x00, 0x00, 0x00, 0x00,
    0x03, 0x00, 0x01, 0x00,  0x00
].pack("C*")

1.times do |i|
    r = udpu.send(gtpu_echo_res, 0, gtpu_addr)
    print "gtpu_echo_res : #{r}\n"

    r = udp.send(gtpc_echo_res, 0, gtpc_addr)
    print "gtpc_echo_res : #{r}\n"

    sleep(1)
end
sleep(1)

1.times do |i|
    r = udpu.send(gtpu_echo_req, 0, gtpu_addr)
    print "gtpu_echo_req : #{r}\n"

    r = udp.send(gtpc_echo_req, 0, gtpc_addr)
    print "gtpc_echo_req : #{r}\n"

    sleep(1)
end

udp.close
udpu.close
