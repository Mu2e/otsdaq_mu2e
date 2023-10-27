source /home/xilinx/Vivado_Lab/2021.2/settings64.sh



SCRIPT_DIR="$( 
 cd "$(dirname "$(readlink "$0" || printf %s "$0")")"
 pwd -P 
)"
HOSTNAME="$(hostname -f)"

echo
echo "Programming FPGA booting configuration from flash memory of both devices on ${HOSTNAME}..."
echo


vivado_lab -mode batch -source ${SCRIPT_DIR}/boot_from_flash_both_DTCs.tcl 


#now reset
echo "Resetting PCIe on ${HOSTNAME}..."
source ${SCRIPT_DIR}/reset_PCIe.sh

echo echo
echo echo
echo "===> Done with ${HOSTNAME} mcs flash program and PCIe reset!"
echo echo
