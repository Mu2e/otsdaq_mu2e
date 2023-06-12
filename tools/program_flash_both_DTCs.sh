source /home/xilinx/Vivado_Lab/2021.2/settings64.sh



SCRIPT_DIR="$( 
 cd "$(dirname "$(readlink "$0" || printf %s "$0")")"
 pwd -P 
)"
HOSTNAME="$(hostname -f)"

echo
echo "Programming flash memory of both JTAG devices on ${HOSTNAME}..."
echo
echo "(note: some DTCs have different flash memory, mt28gu512aax1e-bpi-x16 vs (newer DTCs/default) mt28gu01gaax1e-bpi-x16 - use 2 or more args to specify)"
echo
echo "Number of arguments: $#"
FLASH_PART0="mt28gu01gaax1e-bpi-x16"
FLASH_PART1="mt28gu01gaax1e-bpi-x16"
MCS_FILE0=$1
MCS_FILE1=$1
if [ $# == 1 ]; then
    echo "No flash part parameter argument provided by user - defaulting to ${FLASH_PART0}"
elif [ $# == 2 ]; then
    echo "MCS file to both DTCs: $1" 
    FLASH_PART0=$2
    FLASH_PART1=$2
elif [ $# == 3 ]; then
    echo "MCS file to both DTCs: $1" 
    FLASH_PART0=$2
    FLASH_PART1=$3
elif [ $# == 4 ]; then
    FLASH_PART0=$2
    MCS_FILE1=$3
    FLASH_PART1=$4
else
    echo "Illegal number of arguments, must be 1, 2, 3, or 4 to specify mcs/flash for JTAG-0 and JTAG-1"
    echo -e "\t usage 1 arg:  program_flash_both_DTCs.sh <mcs for both> #default flash part ${FLASH_PART0}"
    echo -e "\t usage 2 args: program_flash_both_DTCs.sh <mcs for both> <flash part for both>"
    echo -e "\t usage 3 args: program_flash_both_DTCs.sh <mcs for both> <flash part for JTAG-0> <flash part for JTAG-1>"
    echo -e "\t usage 4 args: program_flash_both_DTCs.sh <mcs for JTAG-0> <flash part for JTAG-0> <mcs for JTAG-1> <flash part for JTAG-1>"  
    echo
    return  >/dev/null 2>&1 #return is used if script is sourced
	exit  #exit is used if script is run
fi

echo "JTAG-0 mcs file: ${MCS_FILE0}" 
echo "JTAG-1 mcs file: ${MCS_FILE1}" 
echo "JTAG-0 target flash: ${FLASH_PART0}" 
echo "JTAG-1 target flash: ${FLASH_PART1}" 

# echo "Loading this mcs file to both DTCs: $1" 
vivado_lab -mode batch -source ${SCRIPT_DIR}/program_flash_both_DTCs.tcl -tclargs ${MCS_FILE0} ${FLASH_PART0} ${MCS_FILE1} ${FLASH_PART1}
# vivado_lab -mode batch -source program_flash_both_DTCs.tcl -tclargs $1 

#now reset
echo "Resetting PCIe on ${HOSTNAME}..."
source ${SCRIPT_DIR}/reset_PCIe.sh

echo echo
echo echo
echo "===> Done with ${HOSTNAME} mcs flash program and PCIe reset!"
echo echo