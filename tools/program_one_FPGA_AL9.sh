#!/bin/bash

if [[ "${BASH_SOURCE[0]}" != "${0}" ]]; then
    echo "Error: this script must be executed, not sourced." >&2
    return 1 2>/dev/null || exit 1
fi

source /home/xilinx/Vivado_Lab/2021.2/settings64.sh

SCRIPT_DIR="$(
 cd "$(dirname "$(readlink "$0" || printf %s "$0")")"
 pwd -P
)"
HOSTNAME="$(hostname -f)"

cd /home/mu2ehwdev/
rm vivado_lab*.log 2>/dev/null
rm vivado_lab*.jou 2>/dev/null
rm vivado_lab*.str 2>/dev/null
rm hs_err*.log 2>/dev/null
rm err.log 2>/dev/null

lockfile="/tmp/mu2e.lock"
# Attempt to create the lock file atomically using ln
retriedA=0 retriedB=0
while ! ln -s "$$" "$lockfile" 2>/dev/null;do
    # Check if the existing lock file contains a valid PID
    if [ -L "$lockfile" ]; then
        pid=$(readlink "$lockfile")
        if [ -n "$pid" ] && kill -0 "$pid" 2>/dev/null; then
            ps=`ps aux|grep -v ssh`
            waitForCmd=`echo "$ps" | awk "/^[^ ]*  *$pid /"'{print}'`  # COMMAND assoc. w/ pid
            if [ -n "$waitForCmd" ];then
                test $retriedB -gt 39 && { echo "Failed 2 to acquire lock."; exit 1; }
                retriedB=$(($retriedB+1))
                echo "`date`: Waiting for $waitForCmd"
                sleep 2
                continue
            else
                : assume window where cmd just ended
            fi
        fi
    fi
    # Stale (pid not active), remove it and try again
    test $retriedA -gt 19 && { echo "Failed 1 to acquire lock."; exit 1; }
    rm -f "$lockfile"
    echo "`date`: Waiting for retriedA=$retriedA"
    sleep 4
    retriedA=$(($retriedA+1))
done
# Ensure lock file is removed on exit
trap 'rm -f "$lockfile"' EXIT

echo -e "$(date +%d%b%y.%T) program_one_FPGA.sh:${LINENO} |  \t Programming one FPGA on ${HOSTNAME}... SCRIPT_DIR=${SCRIPT_DIR}"
echo -e "$(date +%d%b%y.%T) program_one_FPGA.sh:${LINENO} |  \t Number of arguments: $#"

DORESET=1
if [ "x$1" == "xNORESET" ]; then
    echo -e "$(date +%d%b%y.%T) program_one_FPGA.sh:${LINENO} |  \t Not doing PCIe reset from program one FPGA script!"
    DORESET=0
    shift
fi

if [ $# == 2 ]; then
    BITFILE_N=$2
else
    echo -e "$(date +%d%b%y.%T) program_one_FPGA.sh:${LINENO} |  \t Illegal number of arguments, must be 2 to specify the JTAG index and bitfile"
    echo -e "\t usage 2 args: program_one_FPGA.sh <JTAG index N> <bitfile for JTAG-N>"
    echo
    return  >/dev/null 2>&1 #return is used if script is sourced
	exit  #exit is used if script is run
fi

echo -e "$(date +%d%b%y.%T) program_one_FPGA.sh:${LINENO} |  \t JTAG index N: ${1}"
echo -e "$(date +%d%b%y.%T) program_one_FPGA.sh:${LINENO} |  \t JTAG-N bitfile: ${2}"

vivado_lab -mode batch -source ${SCRIPT_DIR}/program_one_FPGA.tcl -tclargs $1 $2 2>&1 \
    | sed -E s/\(ERROR.*\)/\\1\ \ \ \ \ \ \<\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\ \ \ ERROR!/g \
    | sed s/HIGH/HIGH\ \ \ \ \ \ \<\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\ \ \ Look\ here\!\ \(HIGH\ for\ success\ if\ no\ ERROR\ above\ or\ below\)\\\n\\\n/g

echo -e "$(date +%d%b%y.%T) program_one_FPGA.sh:${LINENO} |  \t Done programming bitfile to one FPGA on ${HOSTNAME}"

if [ $DORESET == 0 ]; then
    echo -e "$(date +%d%b%y.%T) program_one_FPGA.sh:${LINENO} |  \t Skipping reset of PCIe. Done."
    echo
    return  >/dev/null 2>&1 #return is used if script is sourced
        exit  #exit is used if script is run
fi

#now reset
echo -e "$(date +%d%b%y.%T) program_one_FPGA.sh:${LINENO} |  \t Resetting PCIe as ${USER} on ${HOSTNAME}..."
# ssh root@${HOSTNAME} bash ${SCRIPT_DIR}/reset_PCIe_AL9.sh
sudo ${SCRIPT_DIR}/reset_PCIe_AL9.sh
# source ${SCRIPT_DIR}/reset_PCIe_AL9.sh

echo
echo
echo -e "$(date +%d%b%y.%T) program_one_FPGA.sh:${LINENO} |  \t ===> Done with ${HOSTNAME} programming one bitfile and PCIe reset!"
echo
