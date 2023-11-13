source /home/xilinx/Vivado_Lab/2021.2/settings64.sh



SCRIPT_DIR="$( 
 cd "$(dirname "$(readlink "$0" || printf %s "$0")")"
 pwd -P 
)"
HOSTNAME="$(hostname -f)"

echo
echo "Erasing FPGA booting configuration from flash memory of both devices on ${HOSTNAME}..."
echo
echo "(note: some DTCs have different flash memory, mt28gu512aax1e-bpi-x16 vs (newer DTCs/default) mt28gu01gaax1e-bpi-x16 - use 1 or 2 args to specify)"
echo
echo "Number of arguments: $#"
FLASH_PART0="mt28gu01gaax1e-bpi-x16"
FLASH_PART1="mt28gu01gaax1e-bpi-x16"
MCS_FILE0=$1
MCS_FILE1=$1
if [ $# == 0 ]; then
    echo "No flash part parameter argument provided by user - defaulting to ${FLASH_PART0}"
elif [ $# == 1 ]; then
    FLASH_PART0=$1
    FLASH_PART1=$1
elif [ $# == 2 ]; then
    FLASH_PART0=$1
    FLASH_PART1=$2
else
    echo "Illegal number of arguments, must be 0, 1 or 2 to specify flash part for JTAG-0 and JTAG-1"
    echo -e "\t usage 0 arg:  erase_flash_both_DTCs.sh #default flash part ${FLASH_PART0}"
    echo -e "\t usage 1 args: erase_flash_both_DTCs.sh <flash part for both>"
    echo -e "\t usage 2 args: erase_flash_both_DTCs.sh <flash part for JTAG-0> <flash part for JTAG-1>"
    echo
    return  >/dev/null 2>&1 #return is used if script is sourced
	exit  #exit is used if script is run
fi

echo "JTAG-0 target flash: ${FLASH_PART0}" 
echo "JTAG-1 target flash: ${FLASH_PART1}" 

# echo "Loading this mcs file to both DTCs: $1" 
vivado_lab -mode batch -source ${SCRIPT_DIR}/erase_flash_both_DTCs.tcl -tclargs nofile ${FLASH_PART0} nofile ${FLASH_PART1}

#now reset
echo "Resetting PCIe on ${HOSTNAME}..."
source ${SCRIPT_DIR}/reset_PCIe.sh

echo echo
echo echo
echo "===> Done with ${HOSTNAME} erase of flash program and PCIe reset!"
echo echo
