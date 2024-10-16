source /home/xilinx/Vivado_Lab/2021.2/settings64.sh


SCRIPT_DIR="$( 
 cd "$(dirname "$(readlink "$0" || printf %s "$0")")"
 pwd -P 
)"
HOSTNAME="$(hostname -f)"



echo -e "program_one_FPGA.sh:${LINENO} |  \t Programming one FPGA on ${HOSTNAME}..."
echo -e "program_one_FPGA.sh:${LINENO} |  \t Number of arguments: $#"

if [ $# == 2 ]; then
    BITFILE_N=$2
else
    echo -e "program_one_FPGA.sh:${LINENO} |  \t Illegal number of arguments, must be 2 to specify the JTAG index and bitfile"
    echo -e "\t usage 2 args: program_one_FPGA.sh <JTAG index N> <bitfile for JTAG-N>"
    echo
    return  >/dev/null 2>&1 #return is used if script is sourced
	exit  #exit is used if script is run
fi

echo -e "program_one_FPGA.sh:${LINENO} |  \t JTAG index N: ${1}" 
echo -e "program_one_FPGA.sh:${LINENO} |  \t JTAG-N bitfile: ${2}" 

vivado_lab -mode batch -source ${SCRIPT_DIR}/program_one_FPGA.tcl -tclargs $1 $2 | sed s/HIGH/HIGH\ \ \ \ \ \ \<\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\ \ \ Look\ here\!\ \(HIGH\ for\ success\ if\ no\ ERROR\ above\ or\ below\)\\\n\\\n/g

echo -e "program_one_FPGA.sh:${LINENO} |  \t Done programming bitfile to one FPGA on ${HOSTNAME}"


#now reset
echo -e "program_one_FPGA.sh:${LINENO} |  \t Resetting PCIe as ${USER} on ${HOSTNAME}..."
ssh root@${HOSTNAME} bash ${SCRIPT_DIR}/reset_PCIe_AL9.sh
# source ${SCRIPT_DIR}/reset_PCIe_AL9.sh

echo echo
echo echo
echo -e "program_one_FPGA.sh:${LINENO} |  \t ===> Done with ${HOSTNAME} programming one bitfile and PCIe reset!"
echo echo