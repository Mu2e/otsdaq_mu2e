source /home/xilinx/Vivado_Lab/2021.2/settings64.sh


SCRIPT_DIR="$( 
 cd "$(dirname "$(readlink "$0" || printf %s "$0")")"
 pwd -P 
)"
HOSTNAME="$(hostname -f)"



lspci | grep Xilinx

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

done < <(lspci | grep Xilinx)

sleep 1
echo "1" > /sys/bus/pci/rescan
cd /root

echo "Attempting to read firmware version on ${HOSTNAME}..."
source setup_pcie.sh
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

