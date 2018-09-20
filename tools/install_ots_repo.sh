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
#	ask user for package name
#	if package name is understood, install it


repo=`kdialog --title "Install ots Repository" --inputbox "Name of repository to install (e.g. 'prep'):" `
if [ "x$repo" == "x" ];then
  kdialog --msgbox "User decided not to proceed. Exiting."
  exit
fi

echo "repo=$repo"



repoFullName=""
repoInstallDir=""
writeAccess=0


if [ "$repo" == "prep" ]; then
  echo "installing $repo..."
  repoFullName="prepmodernization"
  repoInstallDir="otsdaq_prepmodernization"
elif [ "$repo" == "prep-dev" ]; then
  echo "installing $repo..."
  repoFullName="prepmodernization"
  repoInstallDir="otsdaq_prepmodernization"
  writeAccess=1
else  
  kdialog --sorry "Repository name $repo was not recognized. Please enter a valid repository name."
  exit
fi


kdialog --yesno "Are you sure you want to install the $repo repository?"
if [[ $? -eq 1 ]];then
  kdialog --msgbox "User decided not to proceed. Exiting."
  exit
fi

echo
echo

#at this point, we are going for it
#mrb install
#run tools/ots_repo_install.sh

# download tutorial database
echo 
echo "*****************************************************"
echo "Running mrb install.."
echo

if [[ $writeAccess -eq 1 ]];then
  echo "...with Write access."
  echo "mrb gitCheckout -d ${repoInstallDir} ssh://p-prepmodernization@cdcvs.fnal.gov/cvs/projects/${repoFullName}"
  cd $MRB_SOURCE # this is the 'srcs' directory that will be set in the course of setting up OTS-DAQ
  mrb gitCheckout -d ${repoInstallDir} ssh://p-prepmodernization@cdcvs.fnal.gov/cvs/projects/${repoFullName}
else
  echo "...with Read access."
  echo "mrb gitCheckout -d ${repoInstallDir} http://cdcvs.fnal.gov/projects/${repoFullName}"
  cd $MRB_SOURCE # this is the 'srcs' directory that will be set in the course of setting up OTS-DAQ
  mrb gitCheckout -d ${repoInstallDir} http://cdcvs.fnal.gov/projects/${repoFullName}
fi

echo
echo "running the repo specific install script..."
echo
echo

echo "${MRB_SOURCE}/${repoInstallDir}/tools/install_ots_repo.sh"
chmod 755 ${MRB_SOURCE}/${repoInstallDir}/tools/install_ots_repo.sh
(cd $Base; ${MRB_SOURCE}/${repoInstallDir}/tools/install_ots_repo.sh) #run script as if in base directory


echo
echo
echo "Complete!"
kdialog --msgbox "$repo repository installation complete!"  

