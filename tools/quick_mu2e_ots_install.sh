#!/bin/bash
#
# This script is expected to be sourced as root from the directory into which you want ots installed.
#
# Note: you can try to install as a standard user, but the yum install commands will probably fail
# 	(this might be ok, if the system has already been setup).
#
# cd my/install/path
# wget https://cdcvs.fnal.gov/redmine/projects/otsdaq-utilities/repository/revisions/develop/raw/tools/quick_mu2e_ots_install.sh -O quick_mu2e_ots_install.sh --no-check-certificate
# chmod 755 quick_mu2e_ots_install.sh
# source quick_mu2e_ots_install.sh
#

USER=$(whoami)
FOR_USER=$(stat -c "%U" $PWD)
FOR_GROUP=$(stat -c "%G" $PWD)
	
echo -e "quick_mu2e_ots_install.sh [${LINENO}]  "
echo -e "quick_mu2e_ots_install.sh [${LINENO}]  \t ~~ quick_mu2e_ots_install ~~ "
echo -e "quick_mu2e_ots_install.sh [${LINENO}]  "
echo -e "quick_mu2e_ots_install.sh [${LINENO}]  \t usage: source quick_mu2e_ots_install.sh"
echo -e "quick_mu2e_ots_install.sh [${LINENO}]  "
echo -e "quick_mu2e_ots_install.sh [${LINENO}]  \t user '$USER' installing for target user '$FOR_USER $FOR_GROUP'"
echo -e "quick_mu2e_ots_install.sh [${LINENO}]  \t (note: target user is set based on the owner of $PWD)"
echo -e "quick_mu2e_ots_install.sh [${LINENO}]  "


# at this point, there must have been valid parameters

if [ $USER == "root" ]; then

	#install ots dependencies
	yum install -y libuuid-devel openssl-devel python-devel
	
	#install cvmfs
	yum install -y https://ecsft.cern.ch/dist/cvmfs/cvmfs-release/cvmfs-release-latest.noarch.rpm
	yum clean all
	yum install -y cvmfs cvmfs-config-default
	
	mkdir /etc
	mkdir /etc/cvmfs
	mkdir /etc/cvmfs/default.d
	
	rm -rf /etc/cvmfs/default.d/70-artdaq.conf
	echo "CVMFS_REPOSITORIES=fermilab.opensciencegrid.org" >> /etc/cvmfs/default.d/70-artdaq.conf
	echo "CVMFS_HTTP_PROXY=DIRECT" >> /etc/cvmfs/default.d/70-artdaq.conf
	
	#refresh cvmfs
	cvmfs_config setup
	#Check if CernVM-FS mounts the specified repositories by (restart if failure): 
	cvmfs_config probe || service autofs restart

fi

#install ots
mv ots oldOts/ && mkdir oldOts && rm -rf oldOts/ots && mv ots oldOts/
rm -rf ots
rm quick_mu2e_ots_install.zip
wget https://otsdaq.fnal.gov/downloads/quick_mu2e_ots_install.zip
unzip quick_mu2e_ots_install.zip
cd ots


#update all
REPO_DIR="$(find srcs/ -maxdepth 1 -iname 'otsdaq*')"
		
for p in ${REPO_DIR[@]}; do
	if [ -d $p ]; then
		if [ -d $p/.git ]; then
		
			bp=$(basename $p)
						
			echo -e "UpdateOTS.sh [${LINENO}]  \t Repo directory found as: $bp"
			
			cd $p
			if [ $bp == "otsdaq_mu2e_config" ]; then
				git checkout .  #get all Data and databases
			fi 
			git pull
			cd -
		fi
	fi	   
done

#setup qualifiers
rm -rf change_ots_qualifiers.sh
cp srcs/otsdaq_utilities/tools/change_ots_qualifiers.sh .
chmod 755 change_ots_qualifiers.sh
./change_ots_qualifiers.sh DEFAULT s85:e17:prof

source setup_ots.sh

#update all (need to do again, after setup, or else ninja does not do mrbsetenv correctly(?))
REPO_DIR="$(find srcs/ -maxdepth 1 -iname 'otsdaq*')"
		
for p in ${REPO_DIR[@]}; do
	if [ -d $p ]; then
		if [ -d $p/.git ]; then
		
			bp=$(basename $p)
						
			echo -e "UpdateOTS.sh [${LINENO}]  \t Repo directory found as: $bp"
			
			cd $p
			git pull
			cd -
		fi
	fi	   
done

#clean ninja compile
mz 


if [ $USER == "root" ]; then
	chown -R $FOR_USER ../ots
	chgrp -R $FOR_GROUP ../ots
fi

#remove self so users do not install twice!
rm -rf ../quick_mu2e_ots_install.sh

echo -e "quick_mu2e_ots_install.sh [${LINENO}]  \t =================="
echo -e "quick_mu2e_ots_install.sh [${LINENO}]  \t quick_mu2e_ots_install script done!"
echo -e "quick_mu2e_ots_install.sh [${LINENO}]  \t"
echo -e "quick_mu2e_ots_install.sh [${LINENO}]  \t Next time, cd to ${PWD}:"
echo -e "quick_mu2e_ots_install.sh [${LINENO}]  \t\t source setup_ots.sh     #########################################   #to setup ots"
echo -e "quick_mu2e_ots_install.sh [${LINENO}]  \t\t mb                      #########################################   #for incremental build"
echo -e "quick_mu2e_ots_install.sh [${LINENO}]  \t\t mz                      #########################################   #for clean build"
echo -e "quick_mu2e_ots_install.sh [${LINENO}]  \t\t UpdateOTS.sh            #########################################   #to see update options"
echo -e "quick_mu2e_ots_install.sh [${LINENO}]  \t\t ./change_ots_qualifiers.sh           ############################   #to see qualifier options"
echo -e "quick_mu2e_ots_install.sh [${LINENO}]  \t\t chmod 755 reset_ots_tutorial.sh; ./reset_ots_tutorial.sh --list     #to see tutorial options"
echo -e "quick_mu2e_ots_install.sh [${LINENO}]  \t\t reset_mu2e_ots_snapshot.sh —name trigger_Dev_20200116     			 #to reset ots to a named mu2e snapshot"
echo -e "quick_mu2e_ots_install.sh [${LINENO}]  \t\t ots -w                  #########################################   #to run ots in wiz(safe) mode"
echo -e "quick_mu2e_ots_install.sh [${LINENO}]  \t\t ots                     #########################################   #to run ots in normal mode"

echo -e "quick_mu2e_ots_install.sh [${LINENO}]  \t *******************************"
echo -e "quick_mu2e_ots_install.sh [${LINENO}]  \t *******************************"






