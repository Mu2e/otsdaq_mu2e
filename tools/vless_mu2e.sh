#!/bin/bash

# Opens a remote file somewhat transparently using less

if [ "x$1" == "x" ]; then
    echo
    echo
    echo "    Usage: vless_mu2e.sh <node name> <file name>"
    echo
    echo "    e.g.: vless_mu2e.sh mu2e-cfo-01.fnal.gov /home/mu2ehwdev/ots/srcs/otsdaq_mu2e_config/Data_HWDev/Logs/otsdaq_quiet_run-gateway-mu2e-cfo-01.fnal.gov-3055.txt"
    echo "    e.g.: vless_mu2e.sh mu2e-trk-05.fnal.gov /home/mu2ehwdev/ots/srcs/otsdaq_mu2e_config/Data_HWDev/Logs/otsdaq_quiet_run-mu2e-trk-05.fnal.gov-3061.txt"
    echo
    echo
    exit
fi 

echo "Opening file in 'less' from node ${1}: $2"

scp ${1}:$2 .tmpLogFile && less .tmpLogFile && rm .tmpLogFile



