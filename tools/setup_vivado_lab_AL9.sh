#script can be sourced or executed, but must source to leave vivado_lab setup for running

source /home/xilinx/Vivado_Lab/2021.2/settings64.sh

SCRIPT_DIR="$( 
    cd "$(dirname "$(readlink "${BASH_SOURCE[0]}" || printf %s "${BASH_SOURCE[0]}")")" 
    pwd -P 
)"

HOSTNAME="$(hostname -f)"

echo -e "setup_vivado_lab.sh:${LINENO} |  \t Clearing and setting up vivado lab..."

killall -9 vivado_lab
killall -9 java

rm -rf vivado*.jou
rm -rf vivado*.log

vivado_lab -mode batch -source ${SCRIPT_DIR}/clear_vivado_lab.tcl

#try twice
vivado_lab -mode batch -source ${SCRIPT_DIR}/clear_vivado_lab.tcl