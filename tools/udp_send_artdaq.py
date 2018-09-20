#!/usr/bin/env python
 # This file (udp_send_artdaq.py) was created by Ron Rechenmacher <ron@fnal.gov> on
 # Jan 15, 2015. "TERMS AND CONDITIONS" governing this file are in the README
 # or COPYING file. If you do not have such a file, one can be obtained by
 # contacting Ron or Fermi Lab in Batavia IL, 60510, phone: 630-840-3000.
 # $RCSfile: .emacs.gnu,v $
 # rev="$Revision: 1.23 $$Date: 2012/01/23 15:32:40 $";

import sys
import socket
USAGE='send host:port seqnum'

# first byte is cmd
# 2nd byte is seqnum (yes, just 8 bits)
# rest is data (up to 1498)

buf=''

def main(argv):
    print('len(argv)=%d'%(len(argv),))
    if len(argv) != 3: print(USAGE); sys.exit()
    node,port = argv[1].split(':')
    buf='x'
    buf+=chr(int(argv[2])&0xff)
    buf+="This is the ARTDAQ UDP test string. It contains exactly 109 characters, making for a total size of 111 bytes."
    s = socket.socket( socket.AF_INET, socket.SOCK_DGRAM )
    s.sendto( buf, (node,int(port)) )
    pass


if __name__ == "__main__": main(sys.argv)
