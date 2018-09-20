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


#ask if user is sure they want to update ots
# if no, do nothing, done
# if yes, proceed

#check the current version and compare 
# if latest, do nothing, done
# else, 
#	if no known update process, say so, done
#	else, conduct update process

kdialog --yesno "Are you sure you want to update OTSDAQ (possibly to an unstable version)?"
if [[ $? -eq 1 ]];then #no
	echo "User decided to not continue with update. Exiting update script."
	kdialog --msgbox "User decided to not continue with update. Exiting update script."
	exit
fi

# Setup environment when building with MRB 
if [ "x$MRB_BUILDDIR" != "x" ] && [ -e $OTSDAQ_DIR/CMakeLists.txt ]; then
  export OTSDAQ_LIB=${MRB_BUILDDIR}/otsdaq/lib
fi

OTS_VERSION_STRING="$(cat ${OTSDAQ_LIB}/../tools/VERSION_STRING)"
				
echo "OTS_VERSION_STRING=$OTS_VERSION_STRING"

#check the current version and compare 
# if latest, do nothing, done
# else, 
#	if no known update process, say so, done
#	else, conduct update process
		
if [ "$OTS_VERSION_STRING" == "starting_version_for_v2_of_VM" ]; then
	echo "Updating..."
	dbusRef="$(kdialog --progressbar 'Updating your OTSDAQ installation to latest... 
(this may take several minutes to compile the updates)' 4)"

	qdbus $dbusRef Set "" value 1
	
	UpdateOTS.sh
	
	qdbus $dbusRef Set "" value 2
		
	mrb b
	
	qdbus $dbusRef Set "" value 3

	# download user data to pull components: XDAQConfigurations *.xml and ConfigurationInfo *.xml 
	echo 
	echo "*****************************************************"
	echo "Downloading baseline user data.."
	echo 
	echo "wget otsdaq.fnal.gov/downloads/tutorial_Data_v2.zip"
	echo
	wget otsdaq.fnal.gov/downloads/tutorial_Data_v2.zip
	echo
	echo "Unzipping tutorial user data.."
	echo 
	echo "unzip tutorial_Data_v2.zip -d tmp01234"
	unzip tutorial_Data_v2.zip -d tmp01234	

	# bkup current user data
	echo 
	echo "*****************************************************"
	echo "Backing up current user data.."
	echo 
	echo "cp -r ${USER_DATA} ${USER_DATA}.bak"
	echo
	rm -rf ${USER_DATA}.bak
	cp -r ${USER_DATA} ${USER_DATA}.bak
	
	# move download user data components into position
	echo 
	echo "*****************************************************"
	echo "Installing tutorial data as user data.."
	echo 
	echo "cp tmp01234/NoGitData/XDAQConfigurations/*.xml ${USER_DATA}/XDAQConfigurations/"
	echo
	cp tmp01234/NoGitData/XDAQConfigurations/*.xml ${USER_DATA}/XDAQConfigurations/
	echo 
	echo "cp tmp01234/NoGitData/ConfigurationInfo/*.xml ${USER_DATA}/ConfigurationInfo/"
	echo
	cp tmp01234/NoGitData/ConfigurationInfo/*.xml ${USER_DATA}/ConfigurationInfo/
	
	echo
	echo "Cleaning up downloads.."
	echo 
	echo "rm -rf tmp01234; rm -rf tutorial_Data_v2.zip"
	echo
	rm -rf tmp01234; rm -rf tutorial_Data_v2.zip
	
	qdbus $dbusRef Set "" value 4

	qdbus $dbusRef close
else
	echo "Unknown update procedure from current version to latest. Aborting update script."
	kdialog --msgbox "Unknown update procedure from current version to latest. Aborting update script."
	exit
fi

echo
echo
echo "Update complete."
kdialog --msgbox "Update complete."
echo
