source /home/xilinx/Vivado_Lab/2021.2/settings64.sh



SCRIPT_DIR="$( 
 cd "$(dirname "$(readlink "$0" || printf %s "$0")")"
 pwd -P 
)"
HOSTNAME="$(hostname -f)"

echo
echo "Programming flash memory of one JTAG device on ${HOSTNAME}..."
echo
echo "(note: some DTCs have different flash memory, mt28gu512aax1e-bpi-x16 vs (newer DTCs/default) mt28gu01gaax1e-bpi-x16 - use 2 or 3 args to specify)"
echo
echo "Number of arguments: $#"
FLASH_PART_N="mt28gu01gaax1e-bpi-x16"
MCS_FILE1=$1
if [ $# == 2 ]; then
    MCS_FILE_N=$2
elif [ $# == 3 ]; then
    MCS_FILE_N=$2
    FLASH_PART_N=$3
else
    echo "Illegal number of arguments, must be 2 or 3 to specify JTAG index and mcs/flash"
    echo -e "\t usage 1 arg:  program_flash_one_FPGA.sh <JTAG index N> <mcs for JTAG-N> #default flash part ${FLASH_PART_N}"
    echo -e "\t usage 2 args: program_flash_one_FPGA.sh <JTAG index N> <mcs for JTAG-N> <flash part for JTAG-N>"
    echo
    return  >/dev/null 2>&1 #return is used if script is sourced
	exit  #exit is used if script is run
fi

echo "JTAG index N: ${1}" 
echo "JTAG-N mcs file: ${MCS_FILE_N}" 
echo "JTAG-N target flash: ${FLASH_PART_N}" 

vivado_lab -mode batch -source ${SCRIPT_DIR}/program_flash_one_FPGA.tcl -tclargs ${1} ${MCS_FILE_N} ${FLASH_PART_N}

#now reset
echo "Resetting PCIe on ${HOSTNAME}..."
source ${SCRIPT_DIR}/reset_PCIe.sh

echo echo
echo echo
echo "===> Done with ${HOSTNAME} mcs flash program of one FPGA and PCIe reset!"
echo echo