#!/bin/bash

# Nevermind the previous way was too clever and didn't properly setup env
#Abort if these fail since we don't want to remove the file during a fail

cd ~/test_stand/ots/
source ./setup_ots.sh >~/${HOSTNAME}_ots_out.txt 2>~/${HOSTNAME}_ots_err.txt

echo "${HOSTNAME} Source successful"

OUTFILE=${HOME}/otsdaq_${HOSTNAME}_lock.txt
echo "${HOSTNAME} Writing to ${OUTFILE}"

# Remove if exists
#rm -f $OUTFILE
# actually just rename to prevent danger
mv -f $OUTFILE ${OUTFILE}.old

echo "${HOSTNAME} Resetting register before run"
my_cntl -d 0 write 0x93C8 0x0 >/dev/null 2>/dev/null
my_cntl -d 1 write 0x93C8 0x0 >/dev/null 2>/dev/null

# Run for 16 minutes
for i in {1..960}; do
	LINE="$(date +%s%3N) $(my_cntl -d 0 read 0x93C8 | head -n 1) $(my_cntl -d 1 read 0x93C8 | head -n 1)"
	echo "$LINE" >>${OUTFILE}
        echo "${HOSTNAME} $LINE"
	sleep 1
done
