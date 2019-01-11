#!/bin/bash

echo "setup otsdaq on $HOSTNAME"

#export KRB5CCNAME=FILE:/tmp/krb5cc_otsdaq_develop_daq
source /home/mu2edaq/sync_demo/ots/setup_ots.sh

Reset=`tput sgr0`         # Reset all  

echo -e "${Reset}===> start otsdaq on $HOSTNAME${Reset}"

StartOTS.sh

if [[ "$HOSTNAME" == "mu2edaq06.fnal.gov" ]]; then
    echo -e "${Reset}Starting OTS on remote machines (XDAQ supervisors only)${Reset}"
    ssh mu2edaq04 /home/mu2edaq/sync_demo/ots/sync_demo_ots.sh &
    ssh mu2edaq05 /home/mu2edaq/sync_demo/ots/sync_demo_ots.sh &
#    ssh mu2edaq02 /home/mu2edaq/sync_demo/ots/sync_demo_ots.sh
fi
