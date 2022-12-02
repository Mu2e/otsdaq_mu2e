#!/bin/bash

# Opens a remote file somewhat transparently using tail -f

if [ "x$1" == "x" ]; then
    echo
    echo
    echo "    Usage: vtail_mu2e.sh <node name> <file name>"
    echo
    echo "    e.g.: vtail_mu2e.sh mu2e-cfo-01.fnal.gov /home/mu2ehwdev/ots/srcs/otsdaq_mu2e_config/Data_HWDev/Logs/otsdaq_quiet_run-gateway-mu2e-cfo-01.fnal.gov-3055.txt"
    echo
    echo
    exit
fi 

echo "Viewing file with 'tail -f' from node ${1}: $2"

ssh ${1} tail -f $2


