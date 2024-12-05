source /home/xilinx/Vivado_Lab/2021.2/settings64.sh


SCRIPT_DIR="$( 
 cd "$(dirname "$(readlink "$0" || printf %s "$0")")"
 pwd -P 
)"
HOSTNAME="$(hostname -f)"

echo -e "program_both_DTCs.sh:${LINENO} |  \t Programming both bitfiles on ${HOSTNAME}..."
echo -e "program_both_DTCs.sh:${LINENO} |  \t Number of arguments: $#"
BITFILE0=$1
BITFILE1=$1
if [ $# == 1 ]; then
    echo -e "program_both_DTCs.sh:${LINENO} |  \t Loading this bitfile to both DTCS: ${BITFILE0}" 
elif [ $# == 2 ]; then
    BITFILE1=$2
else
    echo -e "program_both_DTCs.sh:${LINENO} |  \t Illegal number of arguments, must be 1 or 2 to specify the bitfile for JTAG-0 and JTAG-1"
    echo -e "\t usage 1 arg:  program_both_DTCs.sh <bitfile for both>"
    echo -e "\t usage 2 args: program_both_DTCs.sh <bitfile for JTAG-0> <bitfile for JTAG-1>"
    echo
    return  >/dev/null 2>&1 #return is used if script is sourced
	exit  #exit is used if script is run
fi

echo -e "program_both_DTCs.sh:${LINENO} |  \t JTAG-0 bitfile: ${BITFILE0}" 
echo -e "program_both_DTCs.sh:${LINENO} |  \t JTAG-1 bitfile: ${BITFILE1}" 
echo
echo -e "program_both_DTCs.sh:${LINENO} |  \t vivado_lab -mode batch -source ${SCRIPT_DIR}/program_both_DTCs.tcl -tclargs ${BITFILE0} ${BITFILE1}" 
vivado_lab -mode batch -source ${SCRIPT_DIR}/program_both_DTCs.tcl -tclargs ${BITFILE0} ${BITFILE1} | sed s/HIGH/HIGH\ \ \ \ \ \ \<\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\ \ \ Look\ here\!\ \(HIGH\ for\ success\ if\ no\ ERROR\ above\ or\ below\)\\\n\\\n/g
echo
echo -e "program_both_DTCs.sh:${LINENO} |  \t Done programming bitfile to both DTCs on ${HOSTNAME}"


#now reset
echo -e "program_both_DTCs.sh:${LINENO} |  \t Resetting PCIe as ${USER} on ${HOSTNAME}..."
ssh root@${HOSTNAME} bash ${SCRIPT_DIR}/reset_PCIe_AL9.sh
# source ${SCRIPT_DIR}/reset_PCIe_AL9.sh

echo echo
echo echo
echo -e "program_both_DTCs.sh:${LINENO} |  \t ===> Done with ${HOSTNAME} programming bitfile and PCIe reset!"
echo echo
