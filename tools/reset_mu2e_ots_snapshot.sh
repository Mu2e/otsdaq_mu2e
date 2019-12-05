#!/bin/bash
# reset_mu2e_ots_snapshot.sh
#	Launches the specified otsdaq snapshot from the Mu2e NFS server. 
#	Your username must be on the k5login for mu2eshift.
#
# usage: --name <snapshot name>
#
#   snapshot 
#		e.g. a, b, or c
#
#  example run: (if not compiled, use ./reset_mu2e_ots_snapshot.sh)
#	reset_mu2e_ots_snapshot.sh --name a
#

#setup default parameters
SNAPSHOT='a'

echo
echo "  |"
echo "  |"
echo "  |"
echo " _|_"
echo " \ /"
echo "  - "
echo -e `date +"%h%y %T"` "reset_mu2e_ots_snapshot.sh [${LINENO}]  \t ========================================================"
echo -e `date +"%h%y %T"` "reset_mu2e_ots_snapshot.sh [${LINENO}]  \t\t usage: --name <snapshot name>"
echo -e `date +"%h%y %T"` "reset_mu2e_ots_snapshot.sh [${LINENO}]  \t"
echo -e `date +"%h%y %T"` "reset_mu2e_ots_snapshot.sh [${LINENO}]  \t\t note: snapshot will default to '${SNAPSHOT}'"
echo -e `date +"%h%y %T"` "reset_mu2e_ots_snapshot.sh [${LINENO}]  \t"
echo -e `date +"%h%y %T"` "reset_mu2e_ots_snapshot.sh [${LINENO}]  \t\t for example..."
echo -e `date +"%h%y %T"` "reset_mu2e_ots_snapshot.sh [${LINENO}]  \t\t\t reset_mu2e_ots_snapshot.sh --name a"
echo -e `date +"%h%y %T"` "reset_mu2e_ots_snapshot.sh [${LINENO}]  \t"
echo -e `date +"%h%y %T"` "reset_mu2e_ots_snapshot.sh [${LINENO}]  \t"

#return  >/dev/null 2>&1 #return is used if script is sourced


echo
echo -e `date +"%h%y %T"` "reset_mu2e_ots_snapshot.sh [${LINENO}]  \t Extracting parameters..."
echo


if [[ "$1"  == "--name" && "x$2" != "x" ]]; then
	SNAPSHOT="$2"
elif [[ "x$1" == "x" ]]; then

	echo -e `date +"%h%y %T"` "reset_mu2e_ots_snapshot.sh [${LINENO}]  \t Illegal parameters.. See above for usage."
	return  >/dev/null 2>&1 #return is used if script is sourced
	exit  #exit is used if script is run ./reset...
fi


#echo -e `date +"%h%y %T"` "reset_mu2e_ots_snapshot.sh [${LINENO}]  \t SNAPSHOT \t= $SNAPSHOT"
echo		

source setup_ots.sh

ots --killall
killall -9 ots_udp_hw_emulator

#download and run get_snapshot_data script
wget https://cdcvs.fnal.gov/redmine/projects/otsdaq_mu2e/repository/demo/revisions/develop/raw/tools/get_mu2e_snapshot_data.sh -O get_snapshot_data.sh --no-check-certificate
chmod 755 get_snapshot_data.sh
./get_snapshot_data.sh --name ${SNAPSHOT}
	
#download and run get_snapshot_database script
wget https://cdcvs.fnal.gov/redmine/projects/otsdaq_mu2e/repository/demo/revisions/develop/raw/tools/get_mu2e_snapshot_database.sh -O get_snapshot_database.sh --no-check-certificate	
chmod 755 get_snapshot_database.sh
./get_snapshot_database.sh --name ${SNAPSHOT}

#clean up
rm get_snapshot_database.sh
rm get_snapshot_data.sh

ots --wiz #just to test activate the saved groups  
ots  #launch normal mode and open firefox

echo
echo
echo -e `date +"%h%y %T"` "reset_mu2e_ots_snapshot.sh [${LINENO}]  \t Snapshot reset script complete."






