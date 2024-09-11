#!/bin/sh
#
# create_mu2e_ops_snapshot.sh 
#	Creates snapshot of all Data and databases folder for ops by making zip files with transfers to mu2e NFS
#	After this script, others users can pull the snapshot to clone a 'golden' setup
# 	or experts can pull the sanpshot to try to reproduce problems.
#
# Your username must be on the k5login for mu2eshift.
#
# NOTE!!! <snapshot name> must be unique, or snapshot will ask if you want to overwrite!
#
# usage: --name <snapshot name>
# 
#   snapshot name could be anything that defines the user data/databases moment you are capturing
#		e.g. a, b, or c
#		e.g. postGlobalRun2
#		e.g. bug_crashOnReset
#

echo
echo
echo -e `date +"%h%y %T"` "create_mu2e_ops_snapshot.sh [${LINENO}]  \t Do not source this script, run it as create_mu2e_ops_snapshot.sh"
echo -e `date +"%h%y %T"` "create_mu2e_ops_snapshot.sh [${LINENO}]  \t"
echo -e `date +"%h%y %T"` "create_mu2e_ops_snapshot.sh [${LINENO}]  \t\t for example..."
echo -e `date +"%h%y %T"` "create_mu2e_ops_snapshot.sh [${LINENO}]  \t\t\t create_mu2e_ops_snapshot.sh --name a"
		
echo
echo -e `date +"%h%y %T"` "create_mu2e_ops_snapshot.sh [${LINENO}]  \t Extracting parameters..."
echo

SRC=${PWD}
		

if [[ "$1"  == "--name" && "x$2" != "x" ]]; then
	SNAPSHOT="$2"
fi


echo -e `date +"%h%y %T"` "create_mu2e_ops_snapshot.sh [${LINENO}]  \t SNAPSHOT \t= $SNAPSHOT"

if [[ "x$SNAPSHOT" == "x" ]]; then 		#require a snapshot name
	echo -e `date +"%h%y %T"` "create_mu2e_ops_snapshot.sh [${LINENO}]  \t usage: --name <snapshot name>"
	echo -e `date +"%h%y %T"` "create_mu2e_ops_snapshot.sh [${LINENO}]  \t"
	echo -e `date +"%h%y %T"` "create_mu2e_ops_snapshot.sh [${LINENO}]  \t\t for example..."
	echo -e `date +"%h%y %T"` "create_mu2e_ops_snapshot.sh [${LINENO}]  \t\t\t create_mu2e_ops_snapshot.sh --name a"
	echo -e `date +"%h%y %T"` "create_mu2e_ops_snapshot.sh [${LINENO}]  \t"
	exit
fi

UDATABASES="${SRC}/databases_HWDev"
echo -e `date +"%h%y %T"` "create_mu2e_ops_snapshot.sh [${LINENO}]  \t DATABASE \t= $UDATABASES"
UDATA="${SRC}/Data_HWDev"
echo -e `date +"%h%y %T"` "create_mu2e_ops_snapshot.sh [${LINENO}]  \t DATA     \t= $UDATA"
UDATA="${SRC}/Data_shift"
echo -e `date +"%h%y %T"` "create_mu2e_ops_snapshot.sh [${LINENO}]  \t DATA     \t= $UDATA"
UDATA="${SRC}/Data_shift1"
echo -e `date +"%h%y %T"` "create_mu2e_ops_snapshot.sh [${LINENO}]  \t DATA     \t= $UDATA"
UDATA="${SRC}/Data_trigger"
echo -e `date +"%h%y %T"` "create_mu2e_ops_snapshot.sh [${LINENO}]  \t DATA     \t= $UDATA"

wget https://github.com/Mu2e/otsdaq_mu2e/raw/develop/tools/create_mu2e_ots_snapshot.sh  -O create_mu2e_ots_snapshot.sh --no-check-certificate	
chmod 755 create_mu2e_ots_snapshot.sh

UDATA="${SRC}/Data_HWDev"
./create_mu2e_ots_snapshot.sh --name "${SNAPSHOT}" --data $UDATA --database $UDATABASES
UDATA="${SRC}/Data_shift"
./create_mu2e_ots_snapshot.sh --name "${SNAPSHOT}_shift" --data $UDATA --database skip
UDATA="${SRC}/Data_shift1"
./create_mu2e_ots_snapshot.sh --name "${SNAPSHOT}_shift1" --data $UDATA --database skip
UDATA="${SRC}/Data_trigger"
./create_mu2e_ots_snapshot.sh --name "${SNAPSHOT}_trigger" --data $UDATA --database skip


UDATABASES="${SRC}/databases_HWDev"
echo -e `date +"%h%y %T"` "create_mu2e_ops_snapshot.sh [${LINENO}]  \t DATABASE \t= $UDATABASES"
UDATA="${SRC}/Data_HWDev"
echo -e `date +"%h%y %T"` "create_mu2e_ops_snapshot.sh [${LINENO}]  \t DATA     \t= $UDATA"
UDATA="${SRC}/Data_shift"
echo -e `date +"%h%y %T"` "create_mu2e_ops_snapshot.sh [${LINENO}]  \t DATA     \t= $UDATA"
UDATA="${SRC}/Data_shift1"
echo -e `date +"%h%y %T"` "create_mu2e_ops_snapshot.sh [${LINENO}]  \t DATA     \t= $UDATA"
UDATA="${SRC}/Data_trigger"
echo -e `date +"%h%y %T"` "create_mu2e_ops_snapshot.sh [${LINENO}]  \t DATA     \t= $UDATA"


#clean up
rm create_mu2e_ots_snapshot.sh

echo
echo -e `date +"%h%y %T"` "create_mu2e_ops_snapshot.sh [${LINENO}]  \t Done handling ops snapshot of UserData and UserDatabases!"
echo



	