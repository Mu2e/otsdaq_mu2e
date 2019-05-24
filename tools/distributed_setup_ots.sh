#!/bin/bash

userinput=$1
# the next "unsets" the command line input, so as not to pass it along unwittinging somewhere else
shift        

if [ x$userinput == "x" ]; then
  echo "================================================="
  echo "usage:  distributed_setup_ots.sh <subsystem>"
  echo "... where  subsystem = (hwdev,sync)"
  echo "================================================="
  exit;
fi

basepath="notGoodBasepath"

if [ $userinput == "sync" ]; then
    basepath="mu2edaq/sync_demo"
elif [ $userinput == "hwdev" ]; then
    basepath="mu2ehwdev/test_stand"
elif [ $userinput == "stm" ]; then
    basepath="mu2estm/test_stand"
fi


echo "setup otsdaq on $HOSTNAME"

#export KRB5CCNAME=FILE:/tmp/krb5cc_otsdaq_develop_daq
source /home/${basepath}/ots/setup_ots.sh ${userinput}

Reset=`tput sgr0`         # Reset all  

echo -e "${Reset}===> start otsdaq on $HOSTNAME${Reset}"

ots #start ots in normal mode

if [[ "$HOSTNAME" == "mu2edaq06.fnal.gov" ]]; then
    echo -e "${Reset}Starting OTS on remote machines (XDAQ supervisors only)${Reset}"
    ssh mu2edaq04 /home/${basepath}/ots/distributed_setup_ots.sh ${userinput} &
    ssh mu2edaq05 /home/${basepath}/ots/distributed_setup_ots.sh ${userinput} &
elif [[ "$HOSTNAME" == "mu2edaq10.fnal.gov" ]]; then
    echo -e "${Reset}Starting OTS on remote machines (XDAQ supervisors only)${Reset}"
    ssh mu2edaq05 /home/${basepath}/ots/srcs/otsdaq_mu2e/tools/distributed_setup_ots.sh ${userinput} &
fi
