#!/bin/bash

# Call the FPGA programming script for each JTAG found
# Usage: ./program_all_FPGA_AL9.sh <bit file> [<optional d for dryrun>]

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
    echo -e "program_all_FPGA_AL9.sh:${LINENO} |  \t No reset!"
    DORESET=0
    shift
fi

NO_OTS_KILL=0
if [[ "x$1" == "xNOKILL" ]]; then
    echo -e "program_all_FPGA_AL9.sh:${LINENO} |  \t No-ots-kill specified."
    NO_OTS_KILL=1
    shift
fi

BITFILE=$1
DRYRUN=$2
if [ ! -f $BITFILE ]; then
    echo -e "program_all_FPGA_AL9.sh:${LINENO} |  \t No bit file ${BITFILE} found"
    exit 1
fi
echo -e "program_all_FPGA_AL9.sh:${LINENO} |  \t Checking ${HOSTNAME} JTAGs to program bitfile ${BITFILE} for each JTAG"

# For dry run, only print the JTAGs found
COMMANDHEAD=""
if [[ "${DRYRUN}" != "" ]]; then
    echo -e "program_all_FPGA_AL9.sh:${LINENO} |  \t DRYRUN $DRYRUN"
    COMMANDHEAD="echo -e program_all_FPGA_AL9.sh | \t "
fi

# Look for potential JTAGs to program, then program them (unless dryrun requested)
INDEX=0
for d in /sys/bus/usb/devices/*; do
  if [[ -f "$d/idVendor" && -f "$d/idProduct" ]]; then
    vendor=$(<"$d/idVendor")
    product=$(<"$d/idProduct")
    if [[ "$vendor" == "0403" && "$product" == "6014" ]] ||
	   [[ "$vendor" == "03fd" && "$product" == "0008" ]]; then
      echo -e "program_all_FPGA_AL9.sh:${LINENO} |  \t Found JTAG with vendor ${vendor} and product ${product}"
      ${COMMANDHEAD} /home/mu2ehwdev/program_one_FPGA_AL9.sh NORESET ${INDEX} ${BITFILE}
      ((INDEX++))
    fi
  fi
done


if [[ "$DORESET" -eq 0 ]]; then
    echo -e "program_all_FPGA_AL9.sh:${LINENO} |  \t Skipping reset of PCIe. Done."
    echo
    return  >/dev/null 2>&1 #return is used if script is sourced
        exit  #exit is used if script is run
fi

echo -e "program_all_FPGA_AL9.sh:${LINENO} |  \t Handling reset of PCIe."

if [ $NO_OTS_KILL == 1 ]; then
  echo -e "program_all_FPGA_AL9.sh:${LINENO} |  \t Handling reset of PCIe without xdaq kill: sudo ${SCRIPT_DIR}/reset_PCIe_AL9.sh NOKILL"
  ${COMMANDHEAD} sudo ${SCRIPT_DIR}/reset_PCIe_AL9.sh NOKILL
else
  echo -e "program_all_FPGA_AL9.sh:${LINENO} |  \t Handling reset of PCIe: sudo ${SCRIPT_DIR}/reset_PCIe_AL9.sh"
  ${COMMANDHEAD} sudo ${SCRIPT_DIR}/reset_PCIe_AL9.sh
fi

# Print a summary result
echo -e "program_all_FPGA_AL9.sh:${LINENO} |  \t ===> Done with ${HOSTNAME}. Found ${INDEX} JTAGs, programmed  bitfile and PCIe reset for each!"
