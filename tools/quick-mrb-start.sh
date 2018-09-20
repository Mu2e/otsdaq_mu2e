#! /bin/bash
# quick-mrb-start.sh - Eric Flumerfelt, May 20, 2016
# Downloads otsdaq_demo as an MRB-controlled repository

unsetup_all >/dev/null 2>&1
		
git_status=`git status 2>/dev/null`
git_sts=$?
if [ $git_sts -eq 0 ];then
    echo "This script is designed to be run in a fresh install directory!"
    exit 1
fi

starttime=`date`
Base=$PWD
test -d products || mkdir products
test -d download || mkdir download
test -d log || mkdir log

env_opts_var=`basename $0 | sed 's/\.sh$//' | tr 'a-z-' 'A-Z_'`_OPTS
USAGE="\
   usage: `basename $0` [options]
examples: `basename $0`
          `basename $0` --run-ots
          `basename $0` --debug
--run-ots     runs otsdaq
--debug       perform a debug build
--develop     Install the develop version of the software (may be unstable!)
--tag         Install a specific tag of otsdaq
-e, -s        Use specific qualifiers when building otsdaq
-v            Be more verbose
-x            set -x this script
-w            Check out repositories read/write
"

# Process script arguments and options
eval env_opts=\${$env_opts_var-} # can be args too
eval "set -- $env_opts \"\$@\""
op1chr='rest=`expr "$op" : "[^-]\(.*\)"`   && set -- "-$rest" "$@"'
op1arg='rest=`expr "$op" : "[^-]\(.*\)"`   && set --  "$rest" "$@"'
reqarg="$op1arg;"'test -z "${1+1}" &&echo opt -$op requires arg. &&echo "$USAGE" &&exit'
args= do_help= opt_v=0; opt_w=0; opt_develop=0; opt_skip_extra_products=0;
while [ -n "${1-}" ];do
    if expr "x${1-}" : 'x-' >/dev/null;then
        op=`expr "x$1" : 'x-\(.*\)'`; shift   # done with $1
        leq=`expr "x$op" : 'x-[^=]*\(=\)'` lev=`expr "x$op" : 'x-[^=]*=\(.*\)'`
        test -n "$leq"&&eval "set -- \"\$lev\" \"\$@\""&&op=`expr "x$op" : 'x\([^=]*\)'`
        case "$op" in
            \?*|h*)     eval $op1chr; do_help=1;;
            v*)         eval $op1chr; opt_v=`expr $opt_v + 1`;;
            x*)         eval $op1chr; set -x;;
			s*)         eval $op1arg; squalifier=$1; shift;;
			e*)         eval $op1arg; equalifier=$1; shift;;
			w*)         eval $op1chr; opt_w=`expr $opt_w + 1`;;
            -tag)     eval $op1arg; tag=$1; shift;;
            -run-ots)  opt_run_ots=--run-ots;;
	    -debug)     opt_debug=--debug;;
			-develop) opt_develop=1;;
            *)          echo "Unknown option -$op"; do_help=1;;
        esac
    else
        aa=`echo "$1" | sed -e"s/'/'\"'\"'/g"` args="$args '$aa'"; shift
    fi
done
eval "set -- $args \"\$@\""; unset args aa
set -u   # complain about uninitialed shell variables - helps development

test -n "${do_help-}" -o $# -ge 2 && echo "$USAGE" && exit

if [[ -n "${tag:-}" ]] && [[ $opt_develop -eq 1 ]]; then 
    echo "The \"--tag\" and \"--develop\" options are incompatible - please specify only one."
    exit
fi

# JCF, 1/16/15
# Save all output from this script (stdout + stderr) in a file with a
# name that looks like "quick-start.sh_Fri_Jan_16_13:58:27.script" as
# well as all stderr in a file with a name that looks like
# "quick-start.sh_Fri_Jan_16_13:58:27_stderr.script"
alloutput_file=$( date | awk -v "SCRIPTNAME=$(basename $0)" '{print SCRIPTNAME"_"$1"_"$2"_"$3"_"$4".script"}' )
stderr_file=$( date | awk -v "SCRIPTNAME=$(basename $0)" '{print SCRIPTNAME"_"$1"_"$2"_"$3"_"$4"_stderr.script"}' )
exec  > >(tee "$Base/log/$alloutput_file")
exec 2> >(tee "$Base/log/$stderr_file")

function detectAndPull() {
	local startDir=$PWD
	cd $Base/download
	local packageName=$1
	local packageOs=$2
	if [[ "$packageOs" != "noarch" ]]; then
		local packageOsArch="$2-x86_64"
		packageOs=`echo $packageOsArch|sed 's/-x86_64-x86_64/-x86_64/g'`
	fi

	if [ $# -gt 2 ];then
		local qualifiers=$3
		if [[ "$qualifiers" == "nq" ]]; then
			qualifiers=
		fi
	fi
	if [ $# -gt 3 ];then
		local packageVersion=$4
	else
		local packageVersion=`curl http://scisoft.fnal.gov/scisoft/packages/${packageName}/ 2>/dev/null|grep ${packageName}|grep "id=\"v"|tail -1|sed 's/.* id="\(v.*\)".*/\1/'`
	fi
	local packageDotVersion=`echo $packageVersion|sed 's/_/\./g'|sed 's/v//'`

	if [[ "$packageOs" != "noarch" ]]; then
		local upsflavor=`ups flavor`
		local packageQualifiers="-`echo $qualifiers|sed 's/:/-/g'`"
		local packageUPSString="-f $upsflavor -q$qualifiers"
	fi
	local packageInstalled=`ups list -aK+ $packageName $packageVersion ${packageUPSString-}|grep -c "$packageName"`
	if [ $packageInstalled -eq 0 ]; then
		local packagePath="$packageName/$packageVersion/$packageName-$packageDotVersion-${packageOs}${packageQualifiers-}.tar.bz2"
		wget http://scisoft.fnal.gov/scisoft/packages/$packagePath >/dev/null 2>&1
		local packageFile=$( echo $packagePath | awk 'BEGIN { FS="/" } { print $NF }' )

		if [[ ! -e $packageFile ]]; then
			if [[ "$packageOs" == "slf7-x86_64" ]]; then
				# Try sl7, as they're both valid...
				detectAndPull $packageName sl7-x86_64 ${qualifiers:-"nq"} $packageVersion
			else
				echo "Unable to download $packageName"
				return 1
			fi
		else
			local returndir=$PWD
			cd $Base/products
			tar -xjf $Base/download/$packageFile
			cd $returndir
		fi
	fi
	cd $startDir
}
cd $Base/download

# 28-Feb-2017, KAB: use central products areas, if available and not skipped
# 10-Mar-2017, ELF: Re-working how this ends up in the setupARTDAQDEMO script
PRODUCTS_SET=""
if [[ $opt_skip_extra_products -eq 0 ]]; then
  FERMIOSG_ARTDAQ_DIR="/cvmfs/fermilab.opensciencegrid.org/products/artdaq"
  FERMIAPP_ARTDAQ_DIR="/grid/fermiapp/products/artdaq"
  for dir in $FERMIOSG_ARTDAQ_DIR $FERMIAPP_ARTDAQ_DIR;
  do
    # if one of these areas has already been set up, do no more
    for prodDir in $(echo ${PRODUCTS:-""} | tr ":" "\n")
    do
      if [[ "$dir" == "$prodDir" ]]; then
        break 2
      fi
    done
    if [[ -f $dir/setup ]]; then
      echo "Setting up artdaq UPS area... ${dir}"
      source $dir/setup
      break
    fi
  done
  CENTRAL_PRODUCTS_AREA="/products"
  for dir in $CENTRAL_PRODUCTS_AREA;
  do
    # if one of these areas has already been set up, do no more
    for prodDir in $(echo ${PRODUCTS:-""} | tr ":" "\n")
    do
      if [[ "$dir" == "$prodDir" ]]; then
        break 2
      fi
    done
    if [[ -f $dir/setup ]]; then
      echo "Setting up central UPS area... ${dir}"
      source $dir/setup
      break
    fi
  done
  PRODUCTS_SET="${PRODUCTS:-}"
fi

echo "Cloning cetpkgsupport to determine current OS"
git clone http://cdcvs.fnal.gov/projects/cetpkgsupport
os=`./cetpkgsupport/bin/get-directory-name os`

if [[ "$os" == "u14" ]]; then
	echo "-H Linux64bit+3.19-2.19" >../products/ups_OVERRIDE.`hostname`
fi

# Get all the information we'll need to decide which exact flavor of the software to install
notag=0
if [ -z "${tag:-}" ]; then 
  if [[ $opt_develop -eq 0 ]];then
    tag=master
  else
    tag=develop
  fi
  notag=1;
fi
if [[ -e product_deps ]]; then mv product_deps product_deps.save; fi
wget https://cdcvs.fnal.gov/redmine/projects/otsdaq/repository/demo/revisions/$tag/raw/ups/product_deps
demo_version=`grep "parent otsdaq_demo" $Base/download/product_deps|awk '{print $3}'`
if [[ $notag -eq 1 ]] && [[ $opt_develop -eq 0 ]]; then
  tag=$demo_version

  # 06-Mar-2017, KAB: re-fetch the product_deps file based on the tag
  mv product_deps product_deps.orig
  wget https://cdcvs.fnal.gov/redmine/projects/otsdaq/repository/demo/revisions/$tag/raw/ups/product_deps
  demo_version=`grep "parent otsdaq_demo" $Base/download/product_deps|awk '{print $3}'`
  tag=$demo_version
fi
otsdaq_version=`grep "^otsdaq " $Base/download/product_deps | awk '{print $2}'`
utilities_version=`grep "^otsdaq_utilities " $Base/download/product_deps | awk '{print $2}'`
defaultQuals=`grep "defaultqual" $Base/download/product_deps|awk '{print $2}'`
defaultE=`echo $defaultQuals|cut -f1 -d:`
defaultS=`echo $defaultQuals|cut -f2 -d:`
if [ -n "${equalifier-}" ]; then 
	equalifier="e${equalifier}";
else
	equalifier=$defaultE
fi
if [ -n "${squalifier-}" ]; then
	squalifier="s${squalifier}"
else
	squalifier=$defaultS
fi
if [[ -n "${opt_debug:-}" ]] ; then
	build_type="debug"
else
	build_type="prof"
fi

wget http://scisoft.fnal.gov/scisoft/bundles/tools/pullProducts
chmod +x pullProducts
./pullProducts $Base/products ${os} otsdaq-${otsdaq_version} ${squalifier}-${equalifier} ${build_type}
    if [ $? -ne 0 ]; then
	echo "Error in pullProducts."
	exit 1
    fi
rm -rf *.bz2 *.txt
source $Base/products/setup
setup mrb
setup git
setup gitflow
setup nodejs v4_5_0

export MRB_PROJECT=otsdaq_demo
cd $Base
mrb newDev -f -v $demo_version -q ${equalifier}:${squalifier}:${build_type}
set +u
source $Base/localProducts_otsdaq_demo_${demo_version}_${equalifier}_${squalifier}_${build_type}/setup
set -u

cd $MRB_SOURCE
if [[ $opt_develop -eq 1 ]]; then
if [ $opt_w -gt 0 ];then
mrb gitCheckout -d otsdaq_utilities ssh://p-otsdaq@cdcvs.fnal.gov/cvs/projects/otsdaq-utilities
mrb gitCheckout ssh://p-otsdaq@cdcvs.fnal.gov/cvs/projects/otsdaq
mrb gitCheckout -d otsdaq_demo ssh://p-otsdaq@cdcvs.fnal.gov/cvs/projects/otsdaq-demo
mrb gitCheckout -d otsdaq_components ssh://p-components@cdcvs.fnal.gov/cvs/projects/components
else
mrb gitCheckout -d otsdaq_utilities http://cdcvs.fnal.gov/projects/otsdaq-utilities
mrb gitCheckout http://cdcvs.fnal.gov/projects/otsdaq
mrb gitCheckout -d otsdaq_demo http://cdcvs.fnal.gov/projects/otsdaq-demo
mrb gitCheckout -d otsdaq_components http://cdcvs.fnal.gov/projects/components
fi
else
if [ $opt_w -gt 0 ];then
mrb gitCheckout -t ${demo_version} -d otsdaq_demo ssh://p-otsdaq@cdcvs.fnal.gov/cvs/projects/otsdaq-demo
else
mrb gitCheckout -t ${demo_version} -d otsdaq_demo http://cdcvs.fnal.gov/projects/otsdaq-demo
fi
fi




cd $Base
    cat >setup_ots.sh <<-EOF
	echo # This script is intended to be sourced.

	sh -c "[ \`ps \$\$ | grep bash | wc -l\` -gt 0 ] || { echo 'Please switch to the bash shell before running the otsdaq-demo.'; exit; }" || exit

	echo -e "setup [${LINENO}]  \t ======================================================"
	echo -e "setup [${LINENO}]  \t Initially your products path was PRODUCTS=\${PRODUCTS}"
	
	#unalias because the original VM aliased for users
	unalias kx >/dev/null 2>&1
	unalias StartOTS.sh >/dev/null 2>&1
	
	PRODUCTS_SAVE=\${PRODUCTS:+\${PRODUCTS}}${PRODUCTS_SET:+\:${PRODUCTS_SET}}
	source $Base/products/setup
        PRODUCTS=\${PRODUCTS:+\${PRODUCTS}}\${PRODUCTS_SAVE:+\:\${PRODUCTS_SAVE}}
        

	setup mrb
	setup git
	source $Base/localProducts_otsdaq_demo_${demo_version}_${equalifier}_${squalifier}_${build_type}/setup
	source mrbSetEnv
	echo -e "setup [${LINENO}]  \t Now your products path is PRODUCTS=\${PRODUCTS}"
	echo

		# Setup environment when building with MRB (As there's no setupARTDAQOTS file)
		
		  export OTSDAQ_DEMO_LIB=\${MRB_BUILDDIR}/otsdaq_demo/lib
		  #export OTSDAQ_LIB=\${MRB_BUILDDIR}/otsdaq/lib
		  #export OTSDAQ_UTILITIES_LIB=\${MRB_BUILDDIR}/otsdaq_utilities/lib
		#Done with Setup environment when building with MRB (As there's no setupARTDAQOTS file)
	
	
        export CETPKG_INSTALL=$Base/products
		export CETPKG_J=4
		
		export OTS_MAIN_PORT=2015

        export USER_DATA="$MRB_SOURCE/otsdaq_demo/NoGitData"
        export ARTDAQ_DATABASE_URI="filesystemdb://$MRB_SOURCE/otsdaq_demo/NoGitDatabases/filesystemdb/test_db"
        export OTSDAQ_DATA="$MRB_SOURCE/otsdaq_demo/NoGitData/OutputData"
        		
        echo -e "setup [${LINENO}]  \t Now your user data path is USER_DATA \t\t = \${USER_DATA}"
        echo -e "setup [${LINENO}]  \t Now your database path is ARTDAQ_DATABASE_URI \t = \${ARTDAQ_DATABASE_URI}"
        echo -e "setup [${LINENO}]  \t Now your output data path is OTSDAQ_DATA \t = \${OTSDAQ_DATA}"
		echo
		
        alias rawEventDump="art -c $MRB_SOURCE/otsdaq/artdaq-ots/ArtModules/fcl/rawEventDump.fcl"
        alias kx='StartOTS.sh -k'
       
        echo
        echo -e "setup [${LINENO}]  \t Now use 'StartOTS.sh --wiz' to configure otsdaq"
        echo -e "setup [${LINENO}]  \t  	Then use 'StartOTS.sh' to start otsdaq"
        echo -e "setup [${LINENO}]  \t  	Or use 'StartOTS.sh --help' for more options"
        echo
        echo -e "setup [${LINENO}]  \t     use 'kx' to kill otsdaq processes"
		echo
  
	EOF
    #

########################################
########################################
## Setup USER_DATA and databases
########################################
########################################
cd $Base
#cp -a $MRB_SOURCE/otsdaq_demo/Data $MRB_SOURCE/otsdaq_demo/NoGitData

#Take from tutorial data 
export USER_DATA="$MRB_SOURCE/otsdaq_demo/NoGitData"
		
#... you must already have ots setup (i.e. $USER_DATA must point to the right place).. if you are using the virtual machine, this happens automatically when you start up the VM.

#download get_tutorial_data script
wget https://cdcvs.fnal.gov/redmine/projects/otsdaq/repository/demo/revisions/develop/raw/tools/get_tutorial_data.sh -O get_tutorial_data.sh

#change permissions so the script is executable
chmod 755 get_tutorial_data.sh

#execute script
./get_tutorial_data.sh


export ARTDAQ_DATABASE_URI="filesystemdb://$MRB_SOURCE/otsdaq_demo/NoGitDatabases/filesystemdb/test_db"
#... you must already have ots setup (i.e. $ARTDAQ_DATABASE_URI must point to the right place).. if you are using the virtual machine, this happens automatically when you start up the VM.

#download get_tutorial_database script
wget https://cdcvs.fnal.gov/redmine/projects/otsdaq/repository/demo/revisions/develop/raw/tools/get_tutorial_database.sh -O get_tutorial_database.sh

#change permissions so the script is executable
chmod 755 get_tutorial_database.sh

#execute script
./get_tutorial_database.sh




#copy tutorial launching scripts
echo
echo -e "UpdateOTS.sh [${LINENO}]  \t updating tutorial launch scripts..."
rm get_tutorial_data.sh &>/dev/null 2>&1 #hide output
rm get_tutorial_database.sh &>/dev/null 2>&1 #hide output
rm reset_ots_tutorial.sh &>/dev/null 2>&1 #hide output
wget https://cdcvs.fnal.gov/redmine/projects/otsdaq/repository/demo/revisions/develop/raw/tools/reset_ots_tutorial.sh -O reset_ots_tutorial.sh	
chmod 755 reset_ots_tutorial.sh


########################################
########################################
## END Setup USER_DATA and databases
########################################
########################################	
	
	
# Build artdaq_demo
cd $MRB_BUILDDIR
set +u
source mrbSetEnv
set -u
export CETPKG_J=$((`cat /proc/cpuinfo|grep processor|tail -1|awk '{print $3}'` + 1))
mrb build    # VERBOSE=1
installStatus=$?
		
echo
echo

if [ $installStatus -eq 0 ]; then
    echo "otsdaq-demo has been installed correctly. Use 'source setup_ots.sh' to setup your otsdaq software, then follow the instructions or visit the project redmine page for more info: https://cdcvs.fnal.gov/redmine/projects/otsdaq/wiki"
    echo	
	echo "In the future, when you open a new terminal, just use 'source setup_ots.sh' to setup your ots installation."
	echo
else
    echo "BUILD ERROR!!! SOMETHING IS VERY WRONG!!!"
    echo
	echo
fi

endtime=`date`

echo "Install start time: $starttime"
echo "Install end time:   $endtime"

