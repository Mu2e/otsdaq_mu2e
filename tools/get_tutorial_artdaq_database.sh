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
echo "**** Gettings otsdaq tutorial Database (configuration tables, etc.)... *********"
echo "********************************************************************************"
echo ""


if [ "x$ARTDAQ_DATABASE_URI" == "x" ]; then
	echo "Error."
	echo "Environment variable ARTDAQ_DATABASE_URI not setup!"
	echo "To setup, use 'export ARTDAQ_DATABASE_URI=filesystemdb://<path to database>'" 
	echo "           e.g. filesystemdb:///home/rrivera/databases/filesystemdb/test_db"
	echo 
	echo 
	echo
	exit    
fi

#Steps:
# download tutorial database
# bkup current database
# move download database into position

ADU_PATH=$(echo ${ARTDAQ_DATABASE_URI} | cut -d':' -f2)
echo "artdaq database filesystem URI Path = ${ADU_PATH}"

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

# download tutorial database
echo 
echo "*****************************************************"
echo "Downloading tutorial database.."
echo 
echo "wget otsdaq.fnal.gov/downloads/tutorial_artdaq_database.zip"
echo
wget otsdaq.fnal.gov/downloads/tutorial_artdaq_database.zip
echo
echo "Unzipping tutorial database.."
echo 
echo "unzip tutorial_artdaq_database.zip -d tmpd1234"
unzip tutorial_artdaq_database.zip -d tmpd1234

# bkup current database
echo 
echo "*****************************************************"
echo "Backing up current database.."
echo 
echo "mv ${ADU_PATH} ${ADU_PATH}.bak"
echo
rm -rf ${ADU_PATH}.bak
mv ${ADU_PATH} ${ADU_PATH}.bak

# move download user data into position
echo 
echo "*****************************************************"
echo "Installing tutorial data as database.."
echo 
echo "mv tmpd1234/databases/filesystemdb/test_db ${ADU_PATH}"
echo
mv tmpd1234/databases/filesystemdb/test_db ${ADU_PATH}

echo
echo "Cleaning up downloads.."
echo 
echo "rm -rf tmpd1234; rm -rf tutorial_artdaq_database.zip"
echo
rm -rf tmpd1234; rm -rf tutorial_artdaq_database.zip

echo 
echo "*****************************************************"
echo 
echo "otsdaq tutorial database installed!"
echo
echo

exit
