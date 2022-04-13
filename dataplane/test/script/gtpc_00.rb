#!/usr/bin/ruby

require "socket"
DSTADR= "172.25.2.3"

udpc = UDPSocket.open()
gtpc_addr = Socket.pack_sockaddr_in(2123, DSTADR)

    gtpc_req = [
        0x48 ,0x20 ,0x00 ,0xd6 ,0x00 ,0x00 ,0x00 ,0x00 ,0x01 ,0xc0 ,0x0e ,0x00 ,0x01 ,0x00 ,0x07 ,0x00,
        0x71 ,0x65 ,0x57 ,0x74 ,0x98 ,0x87 ,0x10 ,0x4c ,0x00 ,0x08 ,0x00 ,0x19 ,0x57 ,0x76 ,0x66 ,0x55,
        0x85 ,0x09 ,0xf1 ,0x4b ,0x00 ,0x08 ,0x00 ,0x19 ,0x57 ,0x76 ,0x66 ,0x55 ,0x85 ,0x09 ,0xf1 ,0x56,
        0x00 ,0x0d ,0x00 ,0x18 ,0x02 ,0xf8 ,0x01 ,0x01 ,0x23 ,0x02 ,0xf8 ,0x01 ,0x00 ,0x01 ,0x23 ,0x45,
        0x53 ,0x00 ,0x03 ,0x00 ,0x02 ,0xf8 ,0x01 ,0x52 ,0x00 ,0x01 ,0x00 ,0x06 ,0x4d ,0x00 ,0x05 ,0x00,
        0x80 ,0x10 ,0x00 ,0x00 ,0x00 ,0x57 ,0x00 ,0x09 ,0x00 ,0x86 ,0x00 ,0x02 ,0x00 ,0x11 ,0xc0 ,0xa8,
        0x0f ,0xc9 ,0x47 ,0x00 ,0x0e ,0x00 ,0x09 ,0x61 ,0x66 ,0x66 ,0x69 ,0x72 ,0x6d ,0x65 ,0x64 ,0x31,
        0x03 ,0x63 ,0x74 ,0x63 ,0x80 ,0x00 ,0x01 ,0x00 ,0x02 ,0x63 ,0x00 ,0x01 ,0x00 ,0x01 ,0x4f ,0x00,
        0x05 ,0x00 ,0x01 ,0x0a ,0x80 ,0x00 ,0x01 ,0x7f ,0x00 ,0x01 ,0x00 ,0x00 ,0x48 ,0x00 ,0x08 ,0x00,
        0x00 ,0x00 ,0x27 ,0x10 ,0x00 ,0x00 ,0x27 ,0x10 ,0x5d ,0x00 ,0x2c ,0x00 ,0x49 ,0x00 ,0x01 ,0x00,
        0x05 ,0x57 ,0x00 ,0x09 ,0x02 ,0x84 ,0x00 ,0x13 ,0x00 ,0x81 ,0xc0 ,0xa8 ,0x0f ,0xc9 ,0x50 ,0x00,
        0x16 ,0x00 ,0x04 ,0x09 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
        0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x72 ,0x00 ,0x02 ,0x00 ,0x40 ,0x00 ,0x91 ,0x00,
        0x08 ,0x00 ,0x02 ,0xf8 ,0x01 ,0x00 ,0x00 ,0x00 ,0x16 ,0x02 

    ].pack("C*")

r = udpc.send(gtpc_req, 0, gtpc_addr)
print "gtpc_req :  #{r}\n"

sleep(1)

udpc.close