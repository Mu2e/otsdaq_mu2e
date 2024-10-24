#!/bin/sh
#
# create_mu2e_ots_snapshot.sh 
#	Creates snapshot of Data and databases by making zip files with transfers to mu2e NFS
#	After this script, others users can pull the snapshot to clone a 'golden' setup
# 	or experts can pull the sanpshot to try to reproduce problems.
#
# Your username must be on the k5login for mu2eshift.
#
# NOTE!!! <snapshot name> must be unique, or snapshot will ask if you want to overwrite!
#
# usage: --name <snapshot name> --data <path of user data to clone> --database <path of database to clone>
# 
#   snapshot 
#		e.g. a, b, or c
#
#	data 
#		is the full path to the $USER_DATA (NoGitData) folder for the snapshot
#	database
#		is the full path to the databases folder for the snapshot
#
#  example run: (if not compiled, use ./create_mu2e_ots_snapshot.sh)
#	./create_mu2e_ots_snapshot.sh --name a --data /home/rrivera/ots/NoGitData --database /home/rrivera/databases
#

echo
echo
echo -e `date +"%h%y %T"` "create_mu2e_ots_snapshot.sh [${LINENO}]  \t Do not source this script, run it as create_mu2e_ots_snapshot.sh"
# return  >/dev/null 2>&1 #return is used if script is sourced
		
echo
echo -e `date +"%h%y %T"` "create_mu2e_ots_snapshot.sh [${LINENO}]  \t Extracting parameters..."
echo

SRC=${PWD}
UDATA=${USER_DATA}
UDATABASES=`echo ${ARTDAQ_DATABASE_URI}|sed 's|.*//|/|'`
		

if [[ "$1"  == "--name" && "x$2" != "x" ]]; then
	SNAPSHOT="$2"
fi

if [[ "$3"  == "--data" && "x$4" != "x" ]]; then
	UDATA="$4"
fi

if [[ "$5"  == "--database" && "x$6" != "x" ]]; then
	UDATABASES="$6"
fi

echo -e `date +"%h%y %T"` "create_mu2e_ots_snapshot.sh [${LINENO}]  \t SNAPSHOT \t= $SNAPSHOT"
echo -e `date +"%h%y %T"` "create_mu2e_ots_snapshot.sh [${LINENO}]  \t DATA     \t= $UDATA"
echo -e `date +"%h%y %T"` "create_mu2e_ots_snapshot.sh [${LINENO}]  \t DATABASE \t= $UDATABASES"

if [[ "x$SNAPSHOT" == "x" ]]; then 		#require a snapshot name
	echo -e `date +"%h%y %T"` "create_mu2e_ots_snapshot.sh [${LINENO}]  \t usage: --name <snapshot name> --data <path of user data to clone> --database <path of database to clone>"
	echo -e `date +"%h%y %T"` "create_mu2e_ots_snapshot.sh [${LINENO}]  \t"
	echo -e `date +"%h%y %T"` "create_mu2e_ots_snapshot.sh [${LINENO}]  \t\t for example..."
	echo -e `date +"%h%y %T"` "create_mu2e_ots_snapshot.sh [${LINENO}]  \t\t\t create_mu2e_ots_snapshot.sh --name a"
	echo -e `date +"%h%y %T"` "create_mu2e_ots_snapshot.sh [${LINENO}]  \t"
	exit
fi

echo
echo -e `date +"%h%y %T"` "create_mu2e_ots_snapshot.sh [${LINENO}]  \t Creating UserData and UserDatabases zips..."
echo

echo -e `date +"%h%y %T"` "create_mu2e_ots_snapshot.sh [${LINENO}]  \t user data directory: ${UDATA}"
echo -e `date +"%h%y %T"` "create_mu2e_ots_snapshot.sh [${LINENO}]  \t user databases directory: ${UDATABASES}"


#copy folders to SRC location and cleanup
if [[ "$UDATABASES" == "skip" ]]; then 
	echo -e `date +"%h%y %T"` "create_mu2e_ots_snapshot.sh [${LINENO}]  \t skipping copy of database"
else
	rm -rf ${SRC}/snapshot_${SNAPSHOT}_databases  >/dev/null 2>&1; #replace databases 
	cp -r ${UDATABASES} ${SRC}/snapshot_${SNAPSHOT}_databases;
	rm -rf ${SRC}/snapshot_${SNAPSHOT}_databases/filesystemdb/test_db.*; 
	rm -rf ${SRC}/snapshot_${SNAPSHOT}_databases/filesystemdb/test_db_*;
	rm -rf ${SRC}/snapshot_${SNAPSHOT}_databases/filesystemdb/test_dbb*; #remove backups or bkups
fi

rm -rf ${SRC}/snapshot_${SNAPSHOT}_Data >/dev/null 2>&1; #replace data
cp -r ${UDATA} ${SRC}/snapshot_${SNAPSHOT}_Data; 
rm ${SRC}/snapshot_${SNAPSHOT}_Data/ServiceData/ActiveConfigurationGroups.cf* >/dev/null 2>&1;
rm ${SRC}/snapshot_${SNAPSHOT}_Data/ServiceData/ActiveTableGroups.cfg.*; 
rm -rf ${SRC}/snapshot_${SNAPSHOT}_Data/ConfigurationInfo.*
rm -rf ${SRC}/snapshot_${SNAPSHOT}_Data/XDAQConfigurations/otsConfigurationNoRU*
rm -rf ${SRC}/snapshot_${SNAPSHOT}_Data/XDAQConfigurations/archive
rm -rf ${SRC}/snapshot_${SNAPSHOT}_Data/TableInfo.updateots.bk
rm -rf ${SRC}/snapshot_${SNAPSHOT}_Data/OutputData/*   #*/ fix comment text coloring
rm -rf ${SRC}/snapshot_${SNAPSHOT}_Data/Logs/*   #*/ fix comment text coloring
rm -rf ${SRC}/snapshot_${SNAPSHOT}_Data/ServiceData/RunNumber/*  #*/ fix comment text coloring
rm -rf ${SRC}/snapshot_${SNAPSHOT}_Data/ServiceData/MacroHistory/* #*/ fix comment text coloring
rm -rf ${SRC}/snapshot_${SNAPSHOT}_Data/ServiceData/ProgressBarData 
rm -rf ${SRC}/snapshot_${SNAPSHOT}_Data/ServiceData/RunControlData
rm -rf ${SRC}/snapshot_${SNAPSHOT}_Data/ServiceData/ConsoleSnapshots
rm -rf ${SRC}/snapshot_${SNAPSHOT}_Data/ServiceData/LoginData/UsersData/TooltipData
#if setup_ots exists, copy into Data
if [ -e ${SRC}/setup_ots.sh ]; then
    echo "setup_ots exists, so copying to USER_DATA." 
	cp ${SRC}/setup_ots.sh ${SRC}/snapshot_${SNAPSHOT}_Data/
fi
#if mongodb_setup exists, copy into Data
if [ -e ${SRC}/mongodb_setup.sh ]; then
    echo "mongodb_setup exists, so copying to USER_DATA." 
	cp ${SRC}/mongodb_setup.sh ${SRC}/snapshot_${SNAPSHOT}_Data/
fi
#if db_setup_ots exists, copy into Data
if [ -e ${SRC}/db_setup_ots.sh ]; then
    echo "db_setup_ots exists, so copying to USER_DATA." 
	cp ${SRC}/db_setup_ots.sh ${SRC}/snapshot_${SNAPSHOT}_Data/
fi


#make demo zips from repo
rm snapshot_${SNAPSHOT}_Data.zip  >/dev/null 2>&1
mv NoGitData NoGitData.mv.bk.snapshot >/dev/null 2>&1; #for safety attempt to move and then restore temporary folder
cp -r ${SRC}/snapshot_${SNAPSHOT}_Data NoGitData;
zip -q -r snapshot_${SNAPSHOT}_Data.zip NoGitData; 
rm -rf NoGitData; 
mv NoGitData.mv.bk.snapshot NoGitData >/dev/null 2>&1; 
#remove clean copies
rm -rf ${SRC}/snapshot_${SNAPSHOT}_Data

if [[ "$UDATABASES" == "skip" ]]; then 
	echo -e `date +"%h%y %T"` "create_mu2e_ots_snapshot.sh [${LINENO}]  \t skipping zip of database"
else
	rm snapshot_${SNAPSHOT}_database.zip  >/dev/null 2>&1
	mv databases databases.mv.bk.snapshot >/dev/null 2>&1; #for safety attempt to move and then restore temporary folder
	cp -r ${SRC}/snapshot_${SNAPSHOT}_databases databases; 
	zip -q -r snapshot_${SNAPSHOT}_database.zip databases; 
	rm -rf databases; 
	mv databases.mv.bk.snapshot databases >/dev/null 2>&1;
	#remove clean copies
	rm -rf ${SRC}/snapshot_${SNAPSHOT}_databases
fi




echo
echo -e `date +"%h%y %T"` "create_mu2e_ots_snapshot.sh [${LINENO}]  \t Done creating UserData and UserDatabases zips."
echo


echo
echo -e `date +"%h%y %T"` "create_mu2e_ots_snapshot.sh [${LINENO}]  \t Transferring to mu2e NFS server (you must be on k5login list for mu2eshift)..."
echo


# scp snapshot_${SNAPSHOT}_Data.zip mu2eshift@mu2edaq01.fnal.gov:/mu2e/DataFiles/UserSnapshots/
SOURCE_FILE="snapshot_${SNAPSHOT}_Data.zip"
USER_REMOTE="mu2eshift@mu2egateway01.fnal.gov"
DESTINATION="/mu2e/DataFiles/UserSnapshots/"

# Check if the file exists on the remote server
if ssh ${USER_REMOTE} "[ -e ${DESTINATION}$(basename "$SOURCE_FILE") ]"; then
    echo "File #{$SOURCE_FILE} already exists at the destination. Overwrite? (y/n)"
    read -r response
    if [[ "$response" == "y" ]]; then
        scp "$SOURCE_FILE" "${USER_REMOTE}:${DESTINATION}"
    else
        echo "File not copied."
    fi
else
	scp "$SOURCE_FILE" "${USER_REMOTE}:$DESTINATION"
fi

rm snapshot_${SNAPSHOT}_Data.zip

if [[ "$UDATABASES" == "skip" ]]; then 
	echo -e `date +"%h%y %T"` "create_mu2e_ots_snapshot.sh [${LINENO}]  \t skipping scp of database"
else
	# scp snapshot_${SNAPSHOT}_database.zip mu2eshift@mu2edaq01.fnal.gov:/mu2e/DataFiles/UserSnapshots/
	SOURCE_FILE="snapshot_${SNAPSHOT}_database.zip"

	# Check if the file exists on the remote server
	if ssh ${USER_REMOTE} "[ -e ${DESTINATION}$(basename "$SOURCE_FILE") ]"; then
		echo "File #{$SOURCE_FILE} already exists at the destination. Overwrite? (y/n)"
		read -r response
		if [[ "$response" == "y" ]]; then
			scp "$SOURCE_FILE" "${USER_REMOTE}:${DESTINATION}"
		else
			echo "File not copied."
		fi
	else
		scp "$SOURCE_FILE" "${USER_REMOTE}:$DESTINATION"
	fi
	rm snapshot_${SNAPSHOT}_database.zip 
fi




echo -e `date +"%h%y %T"` "create_mu2e_ots_snapshot.sh [${LINENO}]  \t SNAPSHOT \t= $SNAPSHOT"
echo -e `date +"%h%y %T"` "create_mu2e_ots_snapshot.sh [${LINENO}]  \t DATA     \t= $UDATA"
echo -e `date +"%h%y %T"` "create_mu2e_ots_snapshot.sh [${LINENO}]  \t DATABASE \t= $UDATABASES"

echo
echo -e `date +"%h%y %T"` "create_mu2e_ots_snapshot.sh [${LINENO}]  \t Done handling snapshot of UserData and UserDatabases!"
echo



	