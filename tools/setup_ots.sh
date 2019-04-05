echo # This script is intended to be sourced.
###########################################
# usage:  source setup_ots.sh <subsystem>
#   subsystem = hwdev, sync, stm, calo
###########################################

userinput=$1
# the next "unsets" the command line input, so as not to pass it to the mrb setup
shift        

if [ x$userinput == "x" ]; then
  echo "--> You are user $USER in the directory `pwd`"
  echo "================================================="
  echo "usage:  source setup_ots.sh <subsystem>"
  echo "... where  subsystem = (hwdev,sync,stm,calo,trigger)"
  echo "================================================="
  return 1;
fi

basepath="notGoodBasepath"
userdataappend="notGoodAppendageToUserData"
repository="notGoodRepository"

if [ $userinput == "sync" ]; then
    export OTS_MAIN_PORT=2015
    export OTS_WIZ_MODE_MAIN_PORT=3015
    basepath="mu2edaq/sync_demo"   #keep this so that Rick can be happy "in his world" (i.e., using 02)
    repository="otsdaq_mu2e"
    userdataappend=""
elif [ $userinput == "stm" ]; then
    export OTS_MAIN_PORT=3035
    export OTS_WIZ_MODE_MAIN_PORT=3035
    basepath="mu2estm/test_stand"
    repository="otsdaq_mu2e_stm"
    userdataappend=""
elif [ $userinput == "calo" ]; then
    export OTS_MAIN_PORT=3025
    export OTS_WIZ_MODE_MAIN_PORT=3025
    basepath="mu2ecalo/test_stand"
    repository="otsdaq_mu2e_calorimeter"
    userdataappend=""
elif [ $userinput == "trigger" ]; then
    export OTS_MAIN_PORT=3045
    export OTS_WIZ_MODE_MAIN_PORT=3045
    basepath="mu2etrigger/test_stand"
    repository="otsdaq_mu2e_trigger"
    userdataappend=""
elif [ $userinput == "hwdev" ]; then
    export OTS_MAIN_PORT=3055
    export OTS_WIZ_MODE_MAIN_PORT=3055
    basepath="mu2ehwdev/test_stand"
    repository="otsdaq_mu2e"
    userdataappend="_HWDev"
fi


sh -c "[ `ps $$ | grep bash | wc -l` -gt 0 ] || { echo 'Please switch to the bash shell before running the otsdaq-demo.'; exit; }" || exit

echo -e "setup [275]  \t ======================================================"
echo -e "setup [275]  \t Initially your products path was PRODUCTS=${PRODUCTS}"

unsetup_all

#unalias because the original VM aliased for users
unalias kx >/dev/null 2>&1
unalias ots >/dev/null 2>&1

PRODUCTS=

PRODUCTS_SAVE=${PRODUCTS:+${PRODUCTS}\:}/cvmfs/fermilab.opensciencegrid.org/products/artdaq:/mu2e/ups
# keep the next since "products is huge and we don't want to copy it" - Ryan
source /home/mu2edaq/sync_demo/ots/products/setup
        PRODUCTS=${PRODUCTS:+${PRODUCTS}}${PRODUCTS_SAVE:+\:${PRODUCTS_SAVE}}  

setup mrb
setup git
source /home/${basepath}/ots/localProducts*/setup
source mrbSetEnv

ulimit -c unlimited

echo -e "setup [275]  \t Now your products path is PRODUCTS=${PRODUCTS}"
echo

# Setup environment when building with MRB (As there's no setupARTDAQOTS file)

export OTSDAQ_DEMO_LIB=${MRB_BUILDDIR}/${repository}/lib
#export OTSDAQ_LIB=${MRB_BUILDDIR}/otsdaq/lib
#export OTSDAQ_UTILITIES_LIB=${MRB_BUILDDIR}/otsdaq_utilities/lib
#Done with Setup environment when building with MRB (As there's no setupARTDAQOTS file)

# I believe we keep this for the same "products" hugeness reason, above
export CETPKG_INSTALL=/home/mu2edaq/sync_demo/ots/products
export CETPKG_J=16


export USER_DATA="/home/${basepath}/ots/srcs/${repository}/Data${userdataappend}"
export ARTDAQ_DATABASE_URI="filesystemdb:///home/${basepath}/ots/srcs/${repository}/databases${userdataappend}/filesystemdb/test_db"
export OTSDAQ_DATA="/home/${basepath}/ots/srcs/${repository}/Data${userdataappend}/OutputData"

echo -e "setup [275]  \t Now your user data path is USER_DATA \t\t = ${USER_DATA}"
echo -e "setup [275]  \t Now your database path is ARTDAQ_DATABASE_URI \t = ${ARTDAQ_DATABASE_URI}"
echo -e "setup [275]  \t Now your output data path is OTSDAQ_DATA \t = ${OTSDAQ_DATA}"
echo

alias rawEventDump="art -c /home/${basepath}/ots/srcs/otsdaq/artdaq-ots/ArtModules/fcl/rawEventDump.fcl"
alias kx='ots -k'

echo
echo -e "setup [275]  \t Now use 'ots --wiz' to configure otsdaq"
echo -e "setup [275]  \t  	Then use 'ots' to start otsdaq"
echo -e "setup [275]  \t  	Or use 'ots --help' for more options"
echo
echo -e "setup [275]  \t     use 'kx' to kill otsdaq processes"
echo
echo Activating TRACE via export TRACE_MSGMAX=0
export TRACE_MSGMAX=0
#echo Turning on all memory tracing via: tonMg 0-63 
#tonMg 0-63
echo Do tshow to show the trace memory buffer.
echo "Do \"tshow | grep . | tdelta -d 1 -ct 1\" with appropriate grep re to"
echo "filter traces. Piping into the tdelta command to add deltas and convert"
echo "the timestamp."

