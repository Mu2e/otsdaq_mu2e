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

echo "********************************************************************************"
echo "************ Gettings otsdaq tutorial Data (user settings, etc.)... ************"
echo "********************************************************************************"
echo ""


if [ "x$USER_DATA" == "x" ]; then
	echo "Error."
	echo "Environment variable USER_DATA not setup!"
	echo "To setup, use 'export USER_DATA=<path to user data>'"
	echo 
	echo
	echo "(If you do not have a user data folder copy '<path to ots source>/otsdaq-demo/Data' as your starting point.)"
	echo
	exit    
fi

#Steps:
# download tutorial user data
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


# download tutorial user data
echo 
echo "*****************************************************"
echo "Downloading tutorial user data.."
echo 
echo "wget otsdaq.fnal.gov/downloads/tutorial_artdaq_Data.zip"
echo
wget otsdaq.fnal.gov/downloads/tutorial_artdaq_Data.zip
echo
echo "Unzipping tutorial user data.."
echo 
echo "unzip tutorial_artdaq_Data.zip -d tmp01234"
unzip tutorial_artdaq_Data.zip -d tmp01234

# bkup current user data
echo 
echo "*****************************************************"
echo "Backing up current user data.."
echo 
echo "mv ${USER_DATA} ${USER_DATA}.bak"
echo
rm -rf ${USER_DATA}.bak
mv ${USER_DATA} ${USER_DATA}.bak

# move download user data into position
echo 
echo "*****************************************************"
echo "Installing tutorial data as user data.."
echo 
echo "mv tmp01234/NoGitData ${USER_DATA}"
echo
mv tmp01234/NoGitData ${USER_DATA}

echo
echo "Cleaning up downloads.."
echo 
echo "rm -rf tmp01234; rm -rf tutorial_artdaq_Data.zip"
echo
rm -rf tmp01234; rm -rf tutorial_artdaq_Data.zip

echo 
echo "*****************************************************"
echo 
echo "otsdaq tutorial Data installed!"
echo
echo

exit
