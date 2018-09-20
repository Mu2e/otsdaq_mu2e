#!/usr/bin/env python
 # This file (udp_send_artdaq.py) was created by Ron Rechenmacher <ron@fnal.gov> on
 # Jan 15, 2015. "TERMS AND CONDITIONS" governing this file are in the README
 # or COPYING file. If you do not have such a file, one can be obtained by
 # contacting Ron or Fermi Lab in Batavia IL, 60510, phone: 630-840-3000.
 # $RCSfile: .emacs.gnu,v $
 # rev="$Revision: 1.23 $$Date: 2012/01/23 15:32:40 $";

import sys
import socket
import random
from time import sleep
USAGE='send host:port seqnum [packetCount] [outoforder]'

# first byte is cmd
# 2nd byte is seqnum (yes, just 8 bits)
# rest is data (up to 1498)

buf=''

def main(argv):
    print('len(argv)=%d'%(len(argv),))
    packetCount = 1
    outOfOrder = False
    if len(argv) < 3: print(USAGE); sys.exit()
    if len(argv) >= 4: packetCount = int(argv[3])
    if len(argv) >= 5: outOfOrder = int(argv[4]) == 1
    if len(argv) >= 6: json = int(argv[5]) == 1

    node,port = argv[1].split(':')
    seqnum= int(argv[2])&0xff
    s = socket.socket( socket.AF_INET, socket.SOCK_DGRAM )  

    if json:
        packetsSent = 0
        while packetsSent < packetCount:
            buf=chr(0x10)
            buf+=chr(seqnum)
            temperature = random.randrange(55,100)
            humidity = random.randrange(10,99)
            buf+="{\"temperature\":" + str(temperature) + ",\"humidity\":" + str(humidity) + ",\"ledState\":\"green\"}"
            s.sendto(buf, (node,int(port)) )
            packetsSent += 1
            seqnum += 1
            sleep(0.5)
    else:
        if packetCount > 1:
            buf=chr(0x21)
            buf+=chr(seqnum)
            buf+="This is the first ARTDAQ UDP test string. It contains exactly 115 characters, making for a total size of 117 bytes."
            s.sendto( buf, (node,int(port)) )
            seqnum += 1

            packetsSent = 2
            while packetsSent < packetCount:
                buf=chr(0x22)
                buf+=chr(seqnum & 0xff)
                buf+="This is a ARTDAQ UDP test string. It contains exactly 107 characters, making for a total size of 109 bytes."
                s.sendto( buf, (node,int(port)) )
                packetsSent += 1
                seqnum += 1

            if outOfOrder:
               buf=chr(0x22)
               buf+=chr((seqnum + 1) & 0xff)
               buf+="This is the first out-of-order ARTDAQ UDP Test String. It should go after the second one in the output."
               s.sendto( buf, (node,int(port)) )
               buf=chr(0x22)
               buf+=chr(seqnum & 0xff)
               buf+="This is the second out-of-order ARTDAQ UDP Test String. It should come before the first one in the output."
               s.sendto( buf, (node,int(port)) )
               seqnum += 2

            buf=chr(0x23)
            buf+=chr(seqnum & 0xff)
            buf+="This is the last ARTDAQ UDP test string. It contains exactly 114 characters, making for a total size of 116 bytes."
            s.sendto( buf, (node,int(port)) )
        else:
            buf=chr(0x20)
            buf+=chr(seqnum & 0xff)
            buf+="This is the ARTDAQ UDP test string. It contains exactly 109 characters, making for a total size of 111 bytes."
            s.sendto( buf, (node,int(port)) )

    pass


if __name__ == "__main__": main(sys.argv)
