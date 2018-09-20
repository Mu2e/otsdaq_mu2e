#!/bin/bash

if ! [ -e setup_ots.sh ]; then
  kdialog --sorry "You must run this script from an OTSDAQ installation directory!"
  exit 1
fi

Base=$PWD
#commenting out unique filename generation
# no need to keep more than one past log for standard users 
#alloutput_file=$( date | awk -v "SCRIPTNAME=$(basename $0)" '{print SCRIPTNAME"_"$1"_"$2"_"$3"_"$4".script"}' )
#stderr_file=$( date | awk -v "SCRIPTNAME=$(basename $0)" '{print SCRIPTNAME"_"$1"_"$2"_"$3"_"$4"_stderr.script"}' )
#exec  > >(tee "$Base/log/$alloutput_file")
mkdir "$Base/script_log"  &>/dev/null #hide output
rm "$Base/script_log/$(basename $0).script"
rm "$Base/script_log/$(basename $0)_stderr.script"
exec  > >(tee "$Base/script_log/$(basename $0).script")
#exec 2> >(tee "$Base/script_log/$stderr_file")
exec 2> >(tee "$Base/script_log/$(basename $0)_stderr.script")

source setup_ots.sh


#ask if user is sure they want to reset ots
# if no, do nothing, done
# if yes, proceed

#ask if user wants to reset ots for the 'First Demo' tutorial
# if yes, replace user data and database with tutorial data
# else, ask if user want to resest ots to the default starting point
#	if yes, replace user data and database with otsdaq_demo repo data
#	else, do nothing, done

kdialog --yesno "Are you sure you want to reset OTS (and clear all user data)? Note you will have the option to reset to tutorial data or default data."
if [[ $? -eq 1 ]];then #no
	echo "User decided to not continue with reset. Exiting reset script."
	kdialog --msgbox "User decided to not continue with reset. Exiting reset script."
	exit
fi


kdialog --yesno "Reset user data for 'First Demo' tutorial?"
if [[ $? -eq 0 ]];then #yes
	echo "User decided to reset to 'First Demo' tutorial data."
	
	dbusRef=`kdialog --progressbar "Installing 'First Demo' tutorial user data and database..." 5`
	qdbus $dbusRef Set "" value 1
	
	########################################
	########################################
	## Setup USER_DATA and databases
	########################################
	########################################
	
	#Take from tutorial data 
	export USER_DATA="$MRB_SOURCE/otsdaq_demo/NoGitData"
		
	#... you must already have ots setup (i.e. $USER_DATA must point to the right place).. if you are using the virtual machine, this happens automatically when you start up the VM.
	
	#download get_tutorial_data script
	wget https://cdcvs.fnal.gov/redmine/projects/otsdaq/repository/demo/revisions/develop/raw/tools/get_tutorial_data.sh -O get_tutorial_data.sh
	qdbus $dbusRef Set "" value 2
	
	#change permissions so the script is executable
	chmod 755 get_tutorial_data.sh
	
	#execute script
	./get_tutorial_data.sh
	qdbus $dbusRef Set "" value 3
	
	export ARTDAQ_DATABASE_URI="filesystemdb://$MRB_SOURCE/otsdaq_demo/NoGitDatabases/filesystemdb/test_db"
	#... you must already have ots setup (i.e. $ARTDAQ_DATABASE_URI must point to the right place).. if you are using the virtual machine, this happens automatically when you start up the VM.
	
	#download get_tutorial_data script
	wget https://cdcvs.fnal.gov/redmine/projects/otsdaq/repository/demo/revisions/develop/raw/tools/get_tutorial_database.sh -O get_tutorial_database.sh
	qdbus $dbusRef Set "" value 5
	
	#change permissions so the script is executable
	chmod 755 get_tutorial_database.sh
	
	#execute script
	./get_tutorial_database.sh
	qdbus $dbusRef Set "" value 5
	
	########################################
	########################################
	## END Setup USER_DATA and databases
	########################################
	########################################
	
	qdbus $dbusRef close

    echo "Now your user data path is USER_DATA = ${USER_DATA}"
    echo "Now your database path is ARTDAQ_DATABASE_URI = ${ARTDAQ_DATABASE_URI}"
	
	echo
	echo
	echo "reset script complete."
	kdialog --msgbox "Reset script complete."
	exit
fi

kdialog --yesno "Reset user data to default data?"
if [[ $? -eq 1 ]];then #no
	echo "User decided to not reset user data. Exiting reset script."
	kdialog --msgbox "User decided to not reset user data. Exiting reset script."
	exit
fi


echo "User decided to reset to default data."

dbusRef=`kdialog --progressbar "Installing Default user data and database..." 5`
qdbus $dbusRef Set "" value 1

########################################
########################################
## Setup USER_DATA and databases defaults
########################################
########################################
rm -rf $MRB_SOURCE/otsdaq_demo/NoGitData.bak
mv $MRB_SOURCE/otsdaq_demo/NoGitData $MRB_SOURCE/otsdaq_demo/NoGitData.bak
qdbus $dbusRef Set "" value 2
cp -a $MRB_SOURCE/otsdaq_demo/Data $MRB_SOURCE/otsdaq_demo/NoGitData
qdbus $dbusRef Set "" value 3

rm -rf $MRB_SOURCE/otsdaq_demo/NoGitDatabases.bak
mv $MRB_SOURCE/otsdaq_demo/NoGitDatabases $MRB_SOURCE/otsdaq_demo/NoGitDatabases.bak
qdbus $dbusRef Set "" value 4
cp -a $MRB_SOURCE/otsdaq_demo/databases $MRB_SOURCE/otsdaq_demo/NoGitDatabases
qdbus $dbusRef Set "" value 5

export USER_DATA="$MRB_SOURCE/otsdaq_demo/NoGitData"
export ARTDAQ_DATABASE_URI="filesystemdb://$MRB_SOURCE/otsdaq_demo/NoGitDatabases/filesystemdb/test_db"

echo "Now your user data path is USER_DATA = ${USER_DATA}"
echo "Now your database path is ARTDAQ_DATABASE_URI = ${ARTDAQ_DATABASE_URI}"

qdbus $dbusRef close

echo
echo
echo "reset script complete."
kdialog --msgbox "Reset script complete."
