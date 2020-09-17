#!/bin/bash
HOSTS=(05 06 14 17 16)
# We don't know if 11 and 10 are in the right order
DTCORDER=(1 0 3 2 5 4 9 8 7 6)
DTCIDX=0
# remember dtcs are 1 0 order in chain because confusion

test -e script.p && rm script.p

cat <<-EOF >script.p
	set autoscale
	unset log
	unset label
	set ytic auto
	set xtic auto
	set title "DTC lock loss collection over 16 minutes on 10 DTC chain"
	set terminal pngcairo enhanced font "Times New Roman,12.0" size 1280,720 truecolor
	set output "lock.png"
EOF

PLOTCMD="plot "
DOCOMMA=0
for i in ${HOSTS[@]}; do
	# Get real hostname+file ext, messy
	DATA_FILE=$(echo ~/otsdaq_mu2edaq$i*_lock.txt)
        if [ $DOCOMMA -eq 0 ]; then
		DOCOMMA=1
	else
		PLOTCMD="${PLOTCMD}, "
	fi
	# Here's where swapped DTCS matter since we label
	# This should be gotten from filename but whatever it's too hard
	TITLEPRE="mu2edaq$i DTC"
	PLOTCMD="${PLOTCMD} '${DATA_FILE}' using 1:2 title '${TITLEPRE} ${DTCORDER[$DTCIDX]}'"
	DTCIDX=$(($DTCIDX+1))
	
	PLOTCMD="${PLOTCMD}, '${DATA_FILE}' using 1:3 title '${TITLEPRE} ${DTCORDER[$DTCIDX]}'"
	DTCIDX=$(($DTCIDX+1))
done
echo $PLOTCMD
echo "$PLOTCMD" >>script.p
