#!/bin/bash


# Resets JA to RTF ext clock

HOSTNAME="$(hostname -f)"

echo -e "reset_both_JAs.sh:${LINENO} |  \t Resetting JAs as ${USER} on ${HOSTNAME}..."
cd /home/mu2ehwdev/ots_spack
source setup_ots.sh HWDev
DTCFrontEndInterface_ExtCFOMain 0 -5
DTCFrontEndInterface_ExtCFOMain 1 -5

echo -e "reset_both_JAs.sh:${LINENO} |  \t Done resetting JAs as ${USER} on ${HOSTNAME}."
echo