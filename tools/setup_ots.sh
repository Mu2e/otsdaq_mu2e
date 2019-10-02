7#!/bin/sh
echo # This script is intended to be sourced.

userinput=$1
# the next "unsets" the command line input, so as not to pass it to the mrb setup
export LOGNAME=$USER #ksu might have messed up LOGNAME

if [ x$userinput == "x" ]; then
  echo -e "setup [${LINENO}]  \t --> You are user $USER on $HOSTNAME in directory `pwd`"
  echo -e "setup [${LINENO}]  \t ================================================="
  echo -e "setup [${LINENO}]  \t usage:  source setup_ots.sh <subsystem>"
  echo -e "setup [${LINENO}]  \t ... where  subsystem = (sync,stm,stmdbtest,calo,trigger,02,dcs,hwdev,tracker,shift,dqmcalo)"
  echo -e "setup [${LINENO}]  \t ================================================="
  return 1;
fi

basepath="notGoodBasepath"
userdataappend="notGoodAppendageToUserData"
repository="notGoodRepository"

if [ $userinput == "sync" ]; then
    export OTS_MAIN_PORT=2015
    export OTS_WIZ_MODE_MAIN_PORT=3015
    export CONSOLE_SUPERVISOR_IP=192.168.157.6
    basepath="mu2edaq/sync_demo"   
    repository="otsdaq_mu2e"
    userdataappend=""
elif [ $userinput == "stm" ]; then
    export OTS_MAIN_PORT=3035
    export OTS_WIZ_MODE_MAIN_PORT=3035
    export CONSOLE_SUPERVISOR_IP=192.168.157.11
    basepath="mu2estm/test_stand"
    repository="otsdaq_mu2e_stm"
    userdataappend=""
elif [ $userinput == "stmdbtest" ]; then
    export OTS_MAIN_PORT=3040
    export OTS_WIZ_MODE_MAIN_PORT=3040
    export CONSOLE_SUPERVISOR_IP=192.168.157.11
    basepath="mu2estm/test_stand"
    repository="otsdaq_mu2e_stm"
    userdataappend="_dbtest"
elif [ $userinput == "calo" ]; then
    export OTS_MAIN_PORT=3025
    export OTS_WIZ_MODE_MAIN_PORT=3025
    export CONSOLE_SUPERVISOR_IP=192.168.157.11
    basepath="mu2ecalo/test_stand"
    repository="otsdaq_mu2e_calorimeter"
    userdataappend=""
elif [ $userinput == "tracker" ]; then
    export OTS_MAIN_PORT=3065
    export OTS_WIZ_MODE_MAIN_PORT=3065
    export CONSOLE_SUPERVISOR_IP=192.168.157.11
    basepath="mu2etrk/test_stand"
    repository="otsdaq_mu2e_tracker"
    userdataappend=""
elif [ $userinput == "shift" ]; then
    export OTS_MAIN_PORT=3075
    export OTS_WIZ_MODE_MAIN_PORT=3075
    export CONSOLE_SUPERVISOR_IP=192.168.157.12
    basepath="mu2eshift/test_stand"
    repository="otsdaq_mu2e"
    userdataappend="_shift"
elif [ $userinput == "shift1" ]; then
    export OTS_MAIN_PORT=4015
    export OTS_WIZ_MODE_MAIN_PORT=4015
    export CONSOLE_SUPERVISOR_IP=192.168.157.12
    basepath="mu2eshift/test_stand"
    repository="otsdaq_mu2e"
    userdataappend="_shift1"
elif [ $userinput == "shift2" ]; then
    export OTS_MAIN_PORT=4025
    export OTS_WIZ_MODE_MAIN_PORT=4025
    export CONSOLE_SUPERVISOR_IP=192.168.157.12
    basepath="mu2eshift/test_stand"
    repository="otsdaq_mu2e"
    userdataappend="_shift2"
elif [ $userinput == "crv" ]; then
    export OTS_MAIN_PORT=3085
    export OTS_WIZ_MODE_MAIN_PORT=3085
    export CONSOLE_SUPERVISOR_IP=192.168.157.12
    basepath="mu2ecrv/test_stand"
    repository="otsdaq_mu2e_crv"
    userdataappend=""
elif [ $userinput == "trigger" ]; then
    export OTS_MAIN_PORT=3045
    export OTS_WIZ_MODE_MAIN_PORT=3045
    export CONSOLE_SUPERVISOR_IP=192.168.157.11
    basepath="mu2etrig/test_stand"
    repository="otsdaq_mu2e_trigger"
    userdataappend=""
elif [ $userinput == "tem" ]; then
    export OTS_MAIN_PORT=4045
    export OTS_WIZ_MODE_MAIN_PORT=4045
    export CONSOLE_SUPERVISOR_IP=192.168.157.11
    basepath="mu2etem/test_stand"
    repository="otsdaq_mu2e_extmon"
    userdataappend=""
elif [ $userinput == "dqmcalo" ]; then
    export OTS_MAIN_PORT=3095
    export OTS_WIZ_MODE_MAIN_PORT=3095
    export CONSOLE_SUPERVISOR_IP=192.168.157.11
    basepath="mu2etrig/test_stand_dqmcalo"
    repository="otsdaq_mu2e_trigger"
    userdataappend="_dqmcalo"
elif [ $userinput == "02" ]; then
    export OTS_MAIN_PORT=2015
    export CONSOLE_SUPERVISOR_IP=127.0.0.1
    basepath="mu2edaq/sync_demo"
    repository="otsdaq_mu2e"
    userdataappend="_02"
elif [ $userinput == "dcs" ]; then
    export OTS_MAIN_PORT=5019
    export CONSOLE_SUPERVISOR_IP=127.0.0.1
    basepath="mu2edcs/dcs_ots_demo"
    repository="otsdaq_mu2e"
    userdataappend="DCS"
elif [ $userinput == "hwdev" ]; then
    export OTS_MAIN_PORT=3055
    export OTS_WIZ_MODE_MAIN_PORT=3055
    export CONSOLE_SUPERVISOR_IP=192.168.157.5
    basepath="mu2ehwdev/test_stand"
    repository="otsdaq_mu2e"
    userdataappend="_HWDev"
else
    echo -e "Invalid parameter!"
    return 1;
fi

#setup ots path append
if [[ $userinput == "hwdev" || $userinput == "shift" || $userinput == "dcs" ]]; then
    otsPathAppend=""
else
    otsPathAppend=$userdataappend
fi


sh -c "[ `ps $$ | grep bash | wc -l` -gt 0 ] || { echo 'Please switch to the bash shell before running the otsdaq-demo.'; exit; }" || exit

echo -e "setup [${LINENO}]  \t ======================================================"
echo -e "setup [${LINENO}]  \t Initially your products path was PRODUCTS=${PRODUCTS}"

echo -e "setup [${LINENO}]  \t otsPathAppend=${otsPathAppend}"
echo -e "setup [${LINENO}]  \t userdataappend=${userdataappend}"


unsetup_all

#unalias because the original VM aliased for users
unalias kx >/dev/null 2>&1
unalias ots >/dev/null 2>&1

PRODUCTS=

PRODUCTS_SAVE=${PRODUCTS:+${PRODUCTS}\:}/cvmfs/fermilab.opensciencegrid.org/products/artdaq:/mu2e/ups
# keep the next since "products is huge and we don't want to copy it to other areas" - Ryan
source /home/mu2edaq/sync_demo/ots/products/setup
        PRODUCTS=${PRODUCTS:+${PRODUCTS}}${PRODUCTS_SAVE:+\:${PRODUCTS_SAVE}}  

setup mrb
setup git
source /home/${basepath}/ots${otsPathAppend}/localProducts*/setup
source mrbSetEnv

ulimit -c unlimited

echo -e "setup [${LINENO}]  \t Now your products path is PRODUCTS=${PRODUCTS}"
echo

echo -e "setup [${LINENO}]  \t To use trace, do \"tshow | grep . | tdelta -d 1 -ct 1\" with appropriate grep re to"
echo -e "setup [${LINENO}]  \t filter traces. Piping into the tdelta command to add deltas and convert"
echo -e "setup [${LINENO}]  \t the timestamp."

# Setup environment when building with MRB (As there's no setupARTDAQOTS file)


export OTSDAQ_DEMO_LIB=${MRB_BUILDDIR}/${repository}/lib
#export OTSDAQ_LIB=${MRB_BUILDDIR}/otsdaq/lib
#export OTSDAQ_UTILITIES_LIB=${MRB_BUILDDIR}/otsdaq_utilities/lib
#Done with Setup environment when building with MRB (As there's no setupARTDAQOTS file)

# MRB should set this itself
#export CETPKG_INSTALL=/home/mu2edaq/sync_demo/ots/products
 
#make the number of build threads dependent on the number of cores on the machine:
export CETPKG_J=$((`cat /proc/cpuinfo|grep processor|tail -1|awk '{print $3}'` + 1))

#make logfile on local directory to make logging faster
echo 

echo -e "setup [${LINENO}]  \t Remove old logs, make new link /home/${basepath}/ots${otsPathAppend}/srcs/${repository}/Data${userdataappend}/Logs to /scratch/mu2e/otsdaqLog_${userinput}"
if [ -e /scratch/mu2e/otsdaqLogs_${userinput} ]; then
    echo -e "setup [${LINENO}]  \t Logfile /scratch/mu2e/otsdaqLog_${userinput} exists"
else 
    echo -e "setup [${LINENO}]  \t Create logfile /scratch/mu2e/otsdaqLog_${userinput}"
    mkdir /scratch/mu2e/otsdaqLogs_${userinput}
fi

echo 
rm -rf /home/${basepath}/ots${otsPathAppend}/srcs/${repository}/Data${userdataappend}/Logs
ln -sf /scratch/mu2e/otsdaqLogs_${userinput} /home/${basepath}/ots${otsPathAppend}/srcs/${repository}/Data${userdataappend}/Logs
rm -rf /home/${basepath}/ots${otsPathAppend}/srcs/${repository}/Data${userdataappend}/ARTDAQConfigurations
ln -sf /scratch/mu2e/otsdaqLogs_${userinput} /home/${basepath}/ots${otsPathAppend}/srcs/${repository}/Data${userdataappend}/ARTDAQConfigurations
rm -rf /home/${basepath}/ots${otsPathAppend}/srcs/${repository}/Data${userdataappend}/TriggerConfigurations
ln -sf /scratch/mu2e/otsdaqLogs_${userinput} /home/${basepath}/ots${otsPathAppend}/srcs/${repository}/Data${userdataappend}/TriggerConfigurations

export OTS_OWNER=Mu2e

export USER_DATA="/home/${basepath}/ots${otsPathAppend}/srcs/${repository}/Data${userdataappend}"
export ARTDAQ_DATABASE_URI="filesystemdb:///home/${basepath}/ots${otsPathAppend}/srcs/${repository}/databases${userdataappend}/filesystemdb/test_db"
export OTSDAQ_DATA="/home/${basepath}/ots${otsPathAppend}/srcs/${repository}/Data${userdataappend}/OutputData"
export USER_WEB_PATH=/home/${basepath}/ots${otsPathAppend}/srcs/${repository}/UserWebGUI
offlineFhiclDir=/mu2e/ups/offline/trig_0_4_2/fcl
triggerEpilogDir=/home/${basepath}/ots/srcs/otsdaq_mu2e_trigger/Data/TriggerConfigurations
dataFilesDir=/mu2e/DataFiles
export FHICL_FILE_PATH=$FHICL_FILE_PATH:$USER_DATA:$offlineFhiclDir:$triggerEpilogDir:$dataFilesDir
export MU2E_SEARCH_PATH=$MU2E_SEARCH_PATH:/mu2e/DataFiles

echo -e "setup [${LINENO}]  \t Now your user data path is USER_DATA \t\t = ${USER_DATA}"
echo -e "setup [${LINENO}]  \t Now your database path is ARTDAQ_DATABASE_URI \t = ${ARTDAQ_DATABASE_URI}"
echo -e "setup [${LINENO}]  \t Now your output data path is OTSDAQ_DATA \t = ${OTSDAQ_DATA}"
echo -e "setup [${LINENO}]  \t Now your output data path is USER_WEB_PATH \t = ${USER_WEB_PATH}"
echo

alias rawEventDump="art -c /home/${basepath}/ots${userdataappend}/srcs/otsdaq/artdaq-ots/ArtModules/fcl/rawEventDump.fcl"
alias kx='ots -k'

echo
echo -e "setup [${LINENO}]  \t Now use 'ots --wiz' to configure otsdaq"
echo -e "setup [${LINENO}]  \t  	Then use 'ots' to start otsdaq"
echo -e "setup [${LINENO}]  \t  	Or use 'ots --help' for more options"
echo
echo -e "setup [${LINENO}]  \t     use 'kx' to kill otsdaq processes"
echo

#=============================
#Trace setup and helpful commented lines:
export TRACE_MSGMAX=0 #Activating TRACE
#echo Turning on all memory tracing via: tonMg 0-63 
#tonMg 0-63

tonMg 0-4  #enable trace to memory
tonSg 0-3  #enable trace to slow path (i.e. UDP)
toffSg 4-64 #apparently not turned off by default?

#enable kernel trace to memory buffer:
#+test -f /proc/trace/buffer && { export TRACE_FILE=/proc/trace/buffer; tlvls | grep 'KERNEL 0xffffffff00ffffff' >/dev/null || { tonMg 0-63; toffM 24-31 -nKERNEL; }; }

#tlvls #to see what is enabled by name
#tonS -N DTC* 0-63 #to enable by name
#tshow | grep DTC #to see memory printouts by name

#end Trace helpful info
#============================

#setup ninja generator
#============================
setup ninja v1_8_2
alias makeninja='pushd $MRB_BUILDDIR; ninja; popd'
alias mb='makeninja'
alias mz='mrb z; mrbsetenv; mrb b --generator ninja'
