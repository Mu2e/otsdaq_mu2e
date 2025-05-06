source /home/xilinx/Vivado_Lab/2021.2/settings64.sh


SCRIPT_DIR="$(
 cd "$(dirname "$(readlink "$0" || printf %s "$0")")"
 pwd -P
)"
HOSTNAME="$(hostname -f)"

lockfile="/tmp/mu2e.lock"
# Attempt to create the lock file atomically using ln
retried=0
while ! ln -s "$$" "$lockfile" 2>/dev/null;do
    # Check if the existing lock file contains a valid PID
    if [ -L "$lockfile" ]; then
        pid=$(readlink "$lockfile")
        if [ -n "$pid" ] && kill -0 "$pid" 2>/dev/null; then
            tempPid=`ps aux | awk '/[0-9] \/bin\/bash .*[r]ead_dtc_temps/{print$2}'`  # last digit of TIME and then COMMAND
            if [ -n "$tempPid" ];then
                for xx in `seq 5`;do kill $tempPid;sleep .02; done
                sleep 2
                if kill -0 "$pid" 2>/dev/null; then
                    echo "read_dtc_temps script is running (pid=$tempPid) and could not kill"
                    exit 1
                fi
                # Temp and killed successfully -- get out
                break
            else
                # Not Temp, someone else
                echo "Some active non-read_dtc_temps script (w/ pid=$pid) has lock - wait and try later"
                exit 0
            fi
        fi
    fi
    # Stale (pid not active), remove it and try again
    test $retried -gt 0 && { echo "Failed to acquire lock."; exit 1; }
    rm -f "$lockfile"
    retried=$(($retried+1))
done
# Ensure lock file is removed on exit
trap 'rm -f "$lockfile"' EXIT

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

vivado_lab -mode batch -source ${SCRIPT_DIR}/program_one_FPGA.tcl -tclargs $1 $2 2>&1 \
    | sed -E s/\(ERROR.*\)/\\1\ \ \ \ \ \ \<\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\ \ \ ERROR!/g \
    | sed s/HIGH/HIGH\ \ \ \ \ \ \<\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\=\ \ \ Look\ here\!\ \(HIGH\ for\ success\ if\ no\ ERROR\ above\ or\ below\)\\\n\\\n/g

echo -e "program_one_FPGA.sh:${LINENO} |  \t Done programming bitfile to one FPGA on ${HOSTNAME}"


#now reset
echo -e "program_one_FPGA.sh:${LINENO} |  \t Resetting PCIe as ${USER} on ${HOSTNAME}..."
ssh root@${HOSTNAME} bash ${SCRIPT_DIR}/reset_PCIe_AL9.sh
# source ${SCRIPT_DIR}/reset_PCIe_AL9.sh

echo echo
echo echo
echo -e "program_one_FPGA.sh:${LINENO} |  \t ===> Done with ${HOSTNAME} programming one bitfile and PCIe reset!"
echo echo
