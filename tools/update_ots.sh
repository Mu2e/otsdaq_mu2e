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


#download the update_ots_donotrun.sh
#chmod 755
#run the script

# download tutorial database
echo 
echo "*****************************************************"
echo "Downloading latest version of update script.."
echo
wget https://cdcvs.fnal.gov/redmine/projects/otsdaq/repository/demo/revisions/develop/raw/tools/update_ots_donotrun.sh -O update_ots_donotrun.sh
echo
echo "running the script..."
echo
echo
	
#change permissions so the script is executable
chmod 755 update_ots_donotrun.sh

#execute script
./update_ots_donotrun.sh

echo "cleaning up..."

#remove downloaded script
rm update_ots_donotrun.sh

echo "done."
echo