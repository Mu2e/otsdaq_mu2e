#!/bin/bash

# usage: --name <snapshot name>
#
#   snapshot 
#		e.g. a, b, or c
#
#  example run:
#	./get_mu2e_snapshot_database.sh --name a
#

# if ! [ -e setup_ots.sh ]; then
	echo -e `date +"%h%y %T"` "get_snapshot_data.sh [${LINENO}]  \t You must run this script from an OTSDAQ installation directory!"
#   exit 1
# fi

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

#setup defaul parameters
SNAPSHOT='a'

if [[ "$1"  == "--name" && "x$2" != "x" ]]; then
	SNAPSHOT="$2"
fi


echo -e `date +"%h%y %T"` "get_snapshot_data.sh [${LINENO}]  \t SNAPSHOT \t= $SNAPSHOT"
echo		

#source setup_ots.sh

echo -e `date +"%h%y %T"` "get_mu2e_snapshot_database.sh [${LINENO}]  \t ********************************************************************************"
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_database.sh [${LINENO}]  \t **** Gettings otsdaq snapshot Database (configuration tables, etc.)... *********"
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_database.sh [${LINENO}]  \t ********************************************************************************"
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_database.sh [${LINENO}]  \t "


if [ "x$ARTDAQ_DATABASE_URI" == "x" ]; then
	echo -e `date +"%h%y %T"` "get_mu2e_snapshot_database.sh [${LINENO}]  \t Error."
	echo -e `date +"%h%y %T"` "get_mu2e_snapshot_database.sh [${LINENO}]  \t Environment variable ARTDAQ_DATABASE_URI not setup!"
	echo -e `date +"%h%y %T"` "get_mu2e_snapshot_database.sh [${LINENO}]  \t To setup, use 'export ARTDAQ_DATABASE_URI=filesystemdb://<path to database>'" 
	echo -e `date +"%h%y %T"` "get_mu2e_snapshot_database.sh [${LINENO}]  \t            e.g. filesystemdb:///home/rrivera/databases/filesystemdb/test_db"
	echo 
	echo 
	echo
	exit    
fi

#Steps:
# download snapshot database
# bkup current database
# move download database into position

ADU_PATH=$(echo ${ARTDAQ_DATABASE_URI} | cut -d':' -f2)
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_database.sh [${LINENO}]  \t artdaq database filesystem URI Path = ${ADU_PATH}"

#attempt to mkdir for full path so that it exists to move the database to
# assuming mkdir is non-destructive
ADU_ARR=$(echo ${ADU_PATH} | tr '/' "\n")
ADU_PATH=""
for ADU_EL in ${ADU_ARR[@]}
do
	#echo $ADU_EL
	#echo $ADU_PATH
	mkdir $ADU_PATH &>/dev/null #hide output
	ADU_PATH="$ADU_PATH/$ADU_EL"
done

# download snapshot database
echo 
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_database.sh [${LINENO}]  \t *****************************************************"
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_database.sh [${LINENO}]  \t Downloading snapshot database.."
echo 
echo
cmd="scp mu2eshift@mu2egateway01.fnal.gov:/mu2e/DataFiles/UserSnapshots/snapshot_${SNAPSHOT}_database.zip ."
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_database.sh [${LINENO}]  \t ${cmd}"
echo

errlog=.tmp_scp_error.txt
${cmd} 2> "$errlog"
if [[ -s "$errlog" ]]; then
    error=`cat "$errlog"`
    # File exists and has a size greater than zero                                                                                                                                  
    echo "THERE WAS AN SCP ERROR: You can use the following error to decide what to do"
    echo $error
    rm $errlog
    exit
else
    echo "SCP OK"
    rm $errlog
fi

echo
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_database.sh [${LINENO}]  \t Unzipping snapshot database.."
chmod 755 -R tmpd1234 #make sure it can be deleted
rm -rf tmpd1234
echo 
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_database.sh [${LINENO}]  \t unzip snapshot_${SNAPSHOT}_database.zip -d tmpd1234"
unzip -q snapshot_${SNAPSHOT}_database.zip -d tmpd1234 #q for quiet

# bkup current database
echo 
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_database.sh [${LINENO}]  \t *****************************************************"
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_database.sh [${LINENO}]  \t Backing up current database.."
echo 

chmod 755 -R ${ADU_PATH}.bak #make sure it can be deleted
chmod 755 -R ${ADU_PATH} #make sure it can be moved

rm -rf ${ADU_PATH}.bak
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_database.sh [${LINENO}]  \t mv ${ADU_PATH} ${ADU_PATH}.bak"
echo
mv ${ADU_PATH} ${ADU_PATH}.bak

# move download user data into position
echo 
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_database.sh [${LINENO}]  \t *****************************************************"
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_database.sh [${LINENO}]  \t Installing snapshot database as database.."
echo 
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_database.sh [${LINENO}]  \t mv tmpd1234/databases/filesystemdb/test_db ${ADU_PATH}"
echo

#hard to be sure of depth of table folders, so check
if [ -d tmpd1234/databases/XDAQContextTable ]; then
	echo -e `date +"%h%y %T"` "get_mu2e_snapshot_database.sh [${LINENO}]  \t mv tmpd1234/databases ${ADU_PATH}"
	mv tmpd1234/databases ${ADU_PATH}
elif [ -d tmpd1234/databases/filesystemdb/XDAQContextTable ]; then
	echo -e `date +"%h%y %T"` "get_mu2e_snapshot_database.sh [${LINENO}]  \t mv tmpd1234/databases/filesystemdb ${ADU_PATH}"
	mv tmpd1234/databases/filesystemdb ${ADU_PATH}
else
	echo -e `date +"%h%y %T"` "get_mu2e_snapshot_database.sh [${LINENO}]  \t mv tmpd1234/databases/filesystemdb/test_db ${ADU_PATH}"	
	mv tmpd1234/databases/filesystemdb/test_db ${ADU_PATH}
fi

echo
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_database.sh [${LINENO}]  \t Cleaning up downloads.."
echo 
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_database.sh [${LINENO}]  \t rm -rf tmpd1234; rm -rf snapshot_${SNAPSHOT}_database.zip"
echo
rm -rf tmpd1234; rm -rf snapshot_${SNAPSHOT}_database.zip

echo 
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_database.sh [${LINENO}]  \t *****************************************************"
echo 
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_database.sh [${LINENO}]  \t otsdaq snapshot database installed!"
echo
echo

exit
