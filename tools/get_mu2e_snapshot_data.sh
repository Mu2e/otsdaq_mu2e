#!/bin/bash

# usage: --name <snapshot name>
#
#   snapshot 
#		e.g. ${SNAPSHOT} or first_demo or a or b or c
#
#  example run:
#	./get_mu2e_snapshot_data.sh --name first_demo
#

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


#setup defaul parameters
SNAPSHOT='a'

if [[ "$1"  == "--name" && "x$2" != "x" ]]; then
	SNAPSHOT="$2"
fi


echo -e `date +"%h%y %T"` "get_mu2e_snapshot_data.sh [${LINENO}]  \t SNAPSHOT \t= $SNAPSHOT"
echo		


#source setup_ots.sh

echo -e `date +"%h%y %T"` "get_mu2e_snapshot_data.sh [${LINENO}]  \t ********************************************************************************"
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_data.sh [${LINENO}]  \t ************ Gettings otsdaq snapshot Data (user settings, etc.)... ************"
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_data.sh [${LINENO}]  \t ********************************************************************************"
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_data.sh [${LINENO}]  \t "


if [ "x$USER_DATA" == "x" ]; then
	echo -e `date +"%h%y %T"` "get_mu2e_snapshot_data.sh [${LINENO}]  \t Error."
	echo -e `date +"%h%y %T"` "get_mu2e_snapshot_data.sh [${LINENO}]  \t Environment variable USER_DATA not setup!"
	echo -e `date +"%h%y %T"` "get_mu2e_snapshot_data.sh [${LINENO}]  \t To setup, use 'export USER_DATA=<path to user data>'"
	echo 
	echo
	echo -e `date +"%h%y %T"` "get_mu2e_snapshot_data.sh [${LINENO}]  \t (If you do not have a user data folder copy '<path to ots source>/otsdaq-demo/Data' as your starting point.)"
	echo
	exit    
fi

#Steps:
# download snapshot user data
# bkup current user data
# move download user data into position


#attempt to mkdir for full path so that it exists to move the user data to
# assuming mkdir is non-destructive
PATH_ARR=$(echo ${USER_DATA} | tr '/' "\n")
UD_PATH=""
for UD_EL in ${PATH_ARR[@]}
do
	#echo $UD_EL
	#echo $UD_PATH
	mkdir $UD_PATH &>/dev/null #hide output
	UD_PATH="$UD_PATH/$UD_EL"
done


# download snapshot user data
echo 
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_data.sh [${LINENO}]  \t *****************************************************"
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_data.sh [${LINENO}]  \t Downloading snapshot user data.."
echo 
cmd="scp mu2eshift@mu2edaq01.fnal.gov:/mu2e/DataFiles/UserSnapshots/snapshot_${SNAPSHOT}_Data.zip ."
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_data.sh [${LINENO}]  \t ${cmd}"
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
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_data.sh [${LINENO}]  \t Unzipping snapshot user data.."
echo 
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_data.sh [${LINENO}]  \t unzip snapshot_${SNAPSHOT}_Data.zip -d tmp01234"
unzip snapshot_${SNAPSHOT}_Data.zip -d tmp01234

# bkup current user data
echo 
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_data.sh [${LINENO}]  \t *****************************************************"
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_data.sh [${LINENO}]  \t Backing up current user data.."
echo 
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_data.sh [${LINENO}]  \t mv ${USER_DATA} ${USER_DATA}.bak"
echo
rm -rf ${USER_DATA}.bak
mv ${USER_DATA} ${USER_DATA}.bak

# move download user data into position
echo 
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_data.sh [${LINENO}]  \t *****************************************************"
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_data.sh [${LINENO}]  \t Installing snapshot data as user data.."
echo 
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_data.sh [${LINENO}]  \t mv tmp01234/NoGitData ${USER_DATA}"
echo
mv tmp01234/NoGitData ${USER_DATA}

echo
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_data.sh [${LINENO}]  \t Cleaning up downloads.."
echo 
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_data.sh [${LINENO}]  \t rm -rf tmp01234; rm -rf snapshot_${SNAPSHOT}_Data.zip"
echo
rm -rf tmp01234; rm -rf snapshot_${SNAPSHOT}_Data.zip

echo
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_data.sh [${LINENO}]  \t Preserving your run number.."
cp ${USER_DATA}.bak/ServiceData/RunNumber/* ${USER_DATA}/ServiceData/RunNumber/ #*/ fix comment text coloring

echo 
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_data.sh [${LINENO}]  \t *****************************************************"
echo 
echo -e `date +"%h%y %T"` "get_mu2e_snapshot_data.sh [${LINENO}]  \t otsdaq snapshot Data installed!"
echo
echo

exit
