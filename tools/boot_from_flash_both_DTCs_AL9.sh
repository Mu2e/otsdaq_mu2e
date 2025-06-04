source /home/xilinx/Vivado_Lab/2021.2/settings64.sh

SCRIPT_DIR="$(
 cd "$(dirname "$(readlink "$0" || printf %s "$0")")"
 pwd -P
)"
HOSTNAME="$(hostname -f)"

echo
echo "Programming FPGA booting configuration from flash memory of both devices on ${HOSTNAME}..."
echo

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

vivado_lab -mode batch -source ${SCRIPT_DIR}/boot_from_flash_both_DTCs.tcl

#now reset
echo "Resetting PCIe on ${HOSTNAME}..."
source ${SCRIPT_DIR}/reset_PCIe_AL9.sh

echo echo
echo echo
echo "===> Done with ${HOSTNAME} mcs flash program and PCIe reset!"
echo echo
