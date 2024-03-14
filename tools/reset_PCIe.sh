#!/bin/sh
#source /home/xilinx/Vivado_Lab/2021.2/settings64.sh


SCRIPT_DIR="$( 
 cd "$(dirname "$(readlink "$0" || printf %s "$0")")"
 pwd -P 
)"
HOSTNAME="$(hostname -f)"

echo "SCRIPT_DIR: ${SCRIPT_DIR}"

lspci | grep 'Xilinx.*7042' && foundXi=1 || foundXi=0

if [ "$foundXi" = 1 ];then
    echo "Found DTC or CFO (Xilinx) card; removing mu2e driver;"
    echo "first killing any processes that may be using the device."
    pids=`lsof /dev/mu2e* 2>/dev/null | awk '!/^COMMAND/{print$2;}' | uniq`
    test -n "$pids" && { echo "First attempt kill $pids"; kill $pids; }
    killall xdaq.exe
    sleep 3
    rmmod mu2e
else
    echo 'No Xilinx 7042 cards found -- exiting'
    exit 1
fi

echo
echo "Resetting each PCIe Xilinx device on ${HOSTNAME}..."
echo

while read -r line
do
    echo "$line"
    IFS=' ' read -r -a array <<< "$line"

    # for p in ${array[@]}; do
    #     echo $p
    # done
    
    echo "1" > /sys/bus/pci/devices/0000:${array[0]}/remove

done <<EOF
$(lspci | grep 'Xilinx.*7042')
EOF

sleep 1
echo "1" > /sys/bus/pci/rescan


echo "Now attempt to reload mu2e module via modprobe mu2e"
modprobe mu2e

cd /root

echo "Attempting to read firmware version on ${HOSTNAME}..."
source ./setup_pcie.sh
echo
echo "PCIe Device 0 firmware version on ${HOSTNAME}:"
my_cntl -d 0 read 0x9004 #device 0
echo
echo "PCIe Device 1 firmware version on ${HOSTNAME}:"
my_cntl -d 1 read 0x9004 #device 1
echo

cd - >/dev/null 2>&1 
echo
echo "Done with ${HOSTNAME} PCIe reset script!"
echo

