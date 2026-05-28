#!/bin/bash

# Call the FPGA boot-from-flash script for each JTAG found
# Usage: ./program_flash all_FPGA_AL9.sh [<optional d for dryrun>]

if [[ "${BASH_SOURCE[0]}" != "${0}" ]]; then
    echo "Error: this script must be executed, not sourced." >&2
    return 1 2>/dev/null || exit 1
fi


SCRIPT_DIR="$(
 cd "$(dirname "$(readlink "$0" || printf %s "$0")")"
 pwd -P
)"

DORESET=1
if [[ "x$1" == "xNORESET" ]]; then
    echo -e "$(date +%d%b%y.%T) boot_from_flash_all_DTCs_AL9.sh:${LINENO} |  \t No reset!"
    DORESET=0
    shift
fi

NO_OTS_KILL=0
if [[ "x$1" == "xNOKILL" ]]; then
    echo -e "$(date +%d%b%y.%T) boot_from_flash_all_DTCs_AL9.sh:${LINENO} |  \t No-ots-kill specified."
    NO_OTS_KILL=1
    shift
fi

DRYRUN=$1
echo -e "$(date +%d%b%y.%T) boot_from_flash_all_DTCs_AL9.sh:${LINENO} |  \t Checking ${HOSTNAME} JTAGs to boot from flash for each JTAG"

# For dry run, only print the commands that would be executed
if [[ "${DRYRUN}" != "" ]]; then
    echo -e "$(date +%d%b%y.%T) boot_from_flash_all_DTCs_AL9.sh:${LINENO} |  \t DRYRUN $DRYRUN"
    run_cmd() {
        echo -e "boot_from_flash_all_DTCs_AL9.sh | \t $*"
    }
    run_source() {
        echo -e "boot_from_flash_all_DTCs_AL9.sh | \t source $*"
    }
else
    run_cmd() {
        "$@"
    }
    run_source() {
        source "$@"
    }
fi

run_source /home/xilinx/Vivado_Lab/2021.2/settings64.sh

# Look for potential JTAGs to program, then program them (unless dryrun requested)
INDEX=0
for d in /sys/bus/usb/devices/*; do
  if [[ -f "$d/idVendor" && -f "$d/idProduct" ]]; then
    vendor=$(<"$d/idVendor")
    product=$(<"$d/idProduct")
    if [[ "$vendor" == "0403" && "$product" == "6014" ]] ||
	   [[ "$vendor" == "03fd" && "$product" == "0008" ]]; then
	   echo -e "$(date +%d%b%y.%T) boot_from_flash_all_DTCs_AL9.sh:${LINENO} |  \t Found JTAG with vendor ${vendor} and product ${product}"
     #try both flash types!
     run_cmd vivado_lab -mode batch -source ${SCRIPT_DIR}/boot_from_flash_one_DTC.tcl -tclargs ${INDEX}
	   ((INDEX++))
    fi
  fi
done


if [[ "$DORESET" -eq 0 ]]; then
    echo -e "$(date +%d%b%y.%T) boot_from_flash_all_DTCs_AL9.sh:${LINENO} |  \t Skipping reset of PCIe. Done."
    echo
    return  >/dev/null 2>&1 #return is used if script is sourced
        exit  #exit is used if script is run
fi

echo -e "$(date +%d%b%y.%T) boot_from_flash_all_DTCs_AL9.sh:${LINENO} |  \t Handling reset of PCIe as ${USER} on ${HOSTNAME}."

if [[ "$NO_OTS_KILL" -eq 1 ]]; then
  echo -e "$(date +%d%b%y.%T) boot_from_flash_all_DTCs_AL9.sh:${LINENO} |  \t Handling reset of PCIe without xdaq kill: sudo ${SCRIPT_DIR}/reset_PCIe_AL9.sh NOKILL"
  run_cmd sudo ${SCRIPT_DIR}/reset_PCIe_AL9.sh NOKILL
else
  echo -e "$(date +%d%b%y.%T) boot_from_flash_all_DTCs_AL9.sh:${LINENO} |  \t Handling reset of PCIe: sudo ${SCRIPT_DIR}/reset_PCIe_AL9.sh"
  run_cmd sudo ${SCRIPT_DIR}/reset_PCIe_AL9.sh
fi

# Print a summary result
echo -e "$(date +%d%b%y.%T) boot_from_flash_all_DTCs_AL9.sh:${LINENO} |  \t ===> Done with ${HOSTNAME}. Found ${INDEX} JTAGs, booted from flash and PCIe reset for each!"
