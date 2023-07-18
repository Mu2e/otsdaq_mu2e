source /home/xilinx/Vivado_Lab/2021.2/settings64.sh


SCRIPT_DIR="$( 
 cd "$(dirname "$(readlink "$0" || printf %s "$0")")"
 pwd -P 
)"
HOSTNAME="$(hostname -f)"



echo "Programming both bitfiles on ${HOSTNAME}..."
echo "Number of arguments: $#"

if [ $# == 2 ]; then
    BITFILE_N=$2
else
    echo "Illegal number of arguments, must be 2 to specify the JTAG index and bitfile"
    echo -e "\t usage 2 args: program_one_FPGA.sh <JTAG index N> <bitfile for JTAG-N>"
    echo
    return  >/dev/null 2>&1 #return is used if script is sourced
	exit  #exit is used if script is run
fi

echo "JTAG index N: ${1}" 
echo "JTAG-N bitfile: ${2}" 

vivado_lab -mode batch -source ${SCRIPT_DIR}/program_one_FPGA.tcl -tclargs $1 $2 

echo "Done programming bitfile to one FPGA on ${HOSTNAME}"


#now reset
echo "Resetting PCIe on ${HOSTNAME}..."
source ${SCRIPT_DIR}/reset_PCIe.sh

echo echo
echo echo
echo "===> Done with ${HOSTNAME} programming one bitfile and PCIe reset!"
echo echo