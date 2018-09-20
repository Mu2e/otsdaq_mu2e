#!/bin/bash

# usage: --tutorial <tutorial name> --version <version string>
#
#   tutorial 
#		e.g. ${TUTORIAL} or artdaq
#   version 
#		usually looks like v2_2 to represent v2.2 release, for example 
#		(underscores might more universal for web downloads than periods)
#
#  example run:
#	./get_tutorial_data.sh --tutorial first_demo --version v2_2
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
TUTORIAL='first_demo'
VERSION='v2_2'

if [[ "$1"  == "--tutorial" && "x$2" != "x" ]]; then
	TUTORIAL="$2"
fi

if [[ "$3"  == "--version" && "x$4" != "x" ]]; then
	VERSION="$4"
fi

echo -e `date +"%h%y %T"` "get_tutorial_data.sh [${LINENO}]  \t TUTORIAL \t= $TUTORIAL"
echo -e `date +"%h%y %T"` "get_tutorial_data.sh [${LINENO}]  \t VERSION  \t= $VERSION"
echo		


source setup_ots.sh

echo -e `date +"%h%y %T"` "get_tutorial_data.sh [${LINENO}]  \t ********************************************************************************"
echo -e `date +"%h%y %T"` "get_tutorial_data.sh [${LINENO}]  \t ************ Gettings otsdaq tutorial Data (user settings, etc.)... ************"
echo -e `date +"%h%y %T"` "get_tutorial_data.sh [${LINENO}]  \t ********************************************************************************"
echo -e `date +"%h%y %T"` "get_tutorial_data.sh [${LINENO}]  \t "


if [ "x$USER_DATA" == "x" ]; then
	echo -e `date +"%h%y %T"` "get_tutorial_data.sh [${LINENO}]  \t Error."
	echo -e `date +"%h%y %T"` "get_tutorial_data.sh [${LINENO}]  \t Environment variable USER_DATA not setup!"
	echo -e `date +"%h%y %T"` "get_tutorial_data.sh [${LINENO}]  \t To setup, use 'export USER_DATA=<path to user data>'"
	echo 
	echo
	echo -e `date +"%h%y %T"` "get_tutorial_data.sh [${LINENO}]  \t (If you do not have a user data folder copy '<path to ots source>/otsdaq-demo/Data' as your starting point.)"
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
echo -e `date +"%h%y %T"` "get_tutorial_data.sh [${LINENO}]  \t *****************************************************"
echo -e `date +"%h%y %T"` "get_tutorial_data.sh [${LINENO}]  \t Downloading tutorial user data.."
echo 
echo -e `date +"%h%y %T"` "get_tutorial_data.sh [${LINENO}]  \t wget otsdaq.fnal.gov/downloads/tutorial_${TUTORIAL}_${VERSION}_Data.zip"
echo
wget otsdaq.fnal.gov/downloads/tutorial_${TUTORIAL}_${VERSION}_Data.zip
echo
echo -e `date +"%h%y %T"` "get_tutorial_data.sh [${LINENO}]  \t Unzipping tutorial user data.."
echo 
echo -e `date +"%h%y %T"` "get_tutorial_data.sh [${LINENO}]  \t unzip tutorial_${TUTORIAL}_${VERSION}_Data.zip -d tmp01234"
unzip tutorial_${TUTORIAL}_${VERSION}_Data.zip -d tmp01234

# bkup current user data
echo 
echo -e `date +"%h%y %T"` "get_tutorial_data.sh [${LINENO}]  \t *****************************************************"
echo -e `date +"%h%y %T"` "get_tutorial_data.sh [${LINENO}]  \t Backing up current user data.."
echo 
echo -e `date +"%h%y %T"` "get_tutorial_data.sh [${LINENO}]  \t mv ${USER_DATA} ${USER_DATA}.bak"
echo
rm -rf ${USER_DATA}.bak
mv ${USER_DATA} ${USER_DATA}.bak

# move download user data into position
echo 
echo -e `date +"%h%y %T"` "get_tutorial_data.sh [${LINENO}]  \t *****************************************************"
echo -e `date +"%h%y %T"` "get_tutorial_data.sh [${LINENO}]  \t Installing tutorial data as user data.."
echo 
echo -e `date +"%h%y %T"` "get_tutorial_data.sh [${LINENO}]  \t mv tmp01234/NoGitData ${USER_DATA}"
echo
mv tmp01234/NoGitData ${USER_DATA}

echo
echo -e `date +"%h%y %T"` "get_tutorial_data.sh [${LINENO}]  \t Cleaning up downloads.."
echo 
echo -e `date +"%h%y %T"` "get_tutorial_data.sh [${LINENO}]  \t rm -rf tmp01234; rm -rf tutorial_${TUTORIAL}_${VERSION}_Data.zip"
echo
rm -rf tmp01234; rm -rf tutorial_${TUTORIAL}_${VERSION}_Data.zip

echo 
echo -e `date +"%h%y %T"` "get_tutorial_data.sh [${LINENO}]  \t *****************************************************"
echo 
echo -e `date +"%h%y %T"` "get_tutorial_data.sh [${LINENO}]  \t otsdaq tutorial Data installed!"
echo
echo

exit
