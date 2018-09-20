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


#Steps:
#	ask if user wants to start normal mode 
#	ask if user wants to start wiz mode


kdialog --yesno "Start OTS (in Normal Mode)?"
if [[ $? -eq 0 ]];then

	dbusRef=`kdialog --progressbar "Launching OTS (in Normal Mode)" 3`
	qdbus $dbusRef Set "" value 1
	
	StartOTS.sh --wiz #just to test activate the saved groups
	
	qdbus $dbusRef Set "" value 2
	
	StartOTS.sh --firefox #launch normal mode and open firefox
	
	qdbus $dbusRef Set "" value 3
	qdbus $dbusRef close	
else
  kdialog --yesno "Start OTS in Wizard Mode?"
  if [[ $? -eq 0 ]]; then
	dbusRef=`kdialog --progressbar "Launching OTS in Wizard Mode" 2`
	qdbus $dbusRef Set "" value 1
	
    StartOTS.sh --wiz --firefox #launch wiz mode and open firefox
	
	qdbus $dbusRef Set "" value 2
	qdbus $dbusRef close
  fi
fi

kdialog --msgbox "Start OTS script complete."