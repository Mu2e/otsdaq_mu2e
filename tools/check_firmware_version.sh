#!/bin/bash

#input the version string to check against
# like 0xd5100695

if [[ $# -ne 1 ]]; then
    echo "1 argument required - usage: $0 < expected version e.g. 0xd5100695 >"
    exit 1
fi

userinput=$1
# the next "unsets" the command line input, so as not to pass it along unwittingly somewhere else
shift

basepath="/home/mu2eshift/ots_ops/"
echo "running setup on $HOSTNAME at $basepath"

# source ${basepath}/setup_ots.sh HWDev &>/dev/null
source /mu2e/spack_pcie/setup-env.sh
spack env activate pcie

Reset="" #`tput sgr0`         # Reset all
echo -e "${Reset}setup complete on $HOSTNAME at $basepath${Reset}"

#read firmware version
ver=$(my_cntl -d0 read 0x9004 2>1 | grep 0x)
# echo "DTC0 Firmware version: $ver"
if [[ "$ver" == "$userinput" ]]; then
    echo "===> $HOSTNAME DTC0 Exact version match. *"
else
    echo "===> $HOSTNAME DTC0 Version mismatch! --------> $ver and expected $userinput${Reset}"
fi
ver=$(my_cntl -d1 read 0x9004 2>1 | grep 0x)
# echo "DTC1 Firmware version: $ver"
if [[ "$ver" == "$userinput" ]]; then
    echo "===> $HOSTNAME DTC1 Exact version match. *"
else
    echo "===> $HOSTNAME DTC1 Version mismatch! --------> $ver and expected $userinput${Reset}"
fi
