#script can be sourced or executed, but must source to leave vivado_lab setup for running

XILINX_INSTALL_PATH="/home/xilinx/Vivado_Lab/2021.2/"
source $XILINX_INSTALL_PATH/settings64.sh
echo "--------------------------"
echo "You could also try installing USB drivers as root:"
echo "#   ksu"
echo "#   cd ${XILINX_INSTALL_PATH}/data/xicom/cable_drivers/lin64/install_script/install_drivers"
echo "#   ./install_drivers"
echo "#   reboot #might be needed"

echo "--------------------------"
echo "To export from an ILA in vivado_lab tcl:"
echo "#   source /home/mu2ehwdev/.Xilinx/vivado_lab/2021.2/Vivado_lab_init.tcl"
echo "#   write_ila_csv 8    ;# to export ILA_8"

SCRIPT_DIR="$(
  cd "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")" && pwd -P
)"

# SCRIPT_DIR="$(
#     cd "$(dirname "$(readlink "${BASH_SOURCE[0]}" || printf %s "${BASH_SOURCE[0]}")")"
#     pwd -P
# )"

echo "--------------------------"
echo "Or try to unbind and bind USB JTAG:"
echo "# source ${SCRIPT_DIR}/find_JTAG_usb_AL9.sh"
echo "--------------------------"
echo "Check last user to touch FPGAs: cat /home/mu2ehwdev/.Xil/vivado_user_log.txt | grep <node> #like trk-03"

HOSTNAME="$(hostname -f)"

echo -e "$(date +%d%b%y.%T) setup_vivado_lab.sh:${LINENO} |  \t Clearing and setting up vivado lab..."

killall -9 vivado_lab
killall -9 java
killall -9 hw_server

rm -rf vivado*.jou
rm -rf vivado*.log
rm -rf .Xilinx #should be at ~/.Xilinx or /tmp/.Xilinx
rm -rf /tmp/.Xilinx

vivado_lab -mode batch -source ${SCRIPT_DIR}/clear_vivado_lab.tcl

#try twice
vivado_lab -mode batch -source ${SCRIPT_DIR}/clear_vivado_lab.tcl
