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
                echo INFO: about to wget $packageName-$packageDotVersion-${packageOs}${packageQualifiers-}
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

curl https://scisoft.fnal.gov/scisoft/packages/cetpkgsupport/v1_14_01/cetpkgsupport-1.14.01-noarch.tar.bz2|tar -jxf -
mv cetpkgsupport/v1_14_01/bin/get-directory-name .
os=`$Base/download/get-directory-name os`

if [[ "$os" == "u14" ]]; then
	echo "-H Linux64bit+3.19-2.19" >../products/ups_OVERRIDE.`hostname`
fi

# Get all the information we'll need to decide which exact flavor of the software to install
notag=0
if [ -z "${tag:-}" ]; then 
  if [[ $opt_develop -eq 0 ]];then
    tag=stable
  else
    tag=develop
  fi
  notag=1;
fi
if [[ -e product_deps ]]; then mv product_deps product_deps.save; fi
wget  https://raw.githubusercontent.com/Mu2e/otsdaq_mu2e/$tag/ups/product_deps
wget  https://raw.githubusercontent.com/Mu2e/otsdaq_mu2e/$tag/CMakeLists.txt
demo_version=v`grep "project" $Base/download/CMakeLists.txt|grep -oE "VERSION [^)]*"|awk '{print $2}'|sed 's/\./_/g'`
if [[ $notag -eq 1 ]] && [[ $opt_develop -eq 0 ]]; then
  tag=$demo_version

  # 06-Mar-2017, KAB: re-fetch the product_deps file based on the tag
  mv product_deps product_deps.orig
wget  https://raw.githubusercontent.com/Mu2e/otsdaq_mu2e/$tag/ups/product_deps
wget  https://raw.githubusercontent.com/Mu2e/otsdaq_mu2e/$tag/CMakeLists.txt
  demo_version=v`grep "project" $Base/download/CMakeLists.txt|grep -oE "VERSION [^)]*"|awk '{print $2}'|sed 's/\./_/g'`
  tag=$demo_version
fi
otsdaq_version=`grep "^otsdaq\s" $Base/download/product_deps | awk '{print $2}'`
utilities_version=`grep "^otsdaq_utilities\s" $Base/download/product_deps | awk '{print $2}'`
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

wget  http://scisoft.fnal.gov/scisoft/bundles/tools/pullProducts
chmod +x pullProducts
./pullProducts $Base/products ${os} otsdaq-${otsdaq_version} ${squalifier}-${equalifier} ${build_type}
    if [ $? -ne 0 ]; then
	echo "Error in pullProducts."
	exit 1
    fi

mrbversion=`grep mrb *_MANIFEST.txt|sort|tail -1|awk '{print $2}'`
rm -rf *.bz2 *.txt
export PRODUCTS=$PRODUCTS_SET
source $Base/products/setup
PRODUCTS_SET=$PRODUCTS
echo PRODUCTS after source products/setup: $PRODUCTS
detectAndPull mrb noarch nq $mrbversion
setup mrb $mrbversion
setup git
setup gitflow

export MRB_PROJECT=otsdaq_demo
cd $Base
mrb newDev -f -v $demo_version -q ${equalifier}:${squalifier}:${build_type}
source $Base/localProducts_otsdaq_demo_${demo_version}_${equalifier}_${squalifier}_${build_type}/setup

cd $MRB_SOURCE
if [[ $opt_develop -eq 1 ]]; then
mrb gitCheckout https://github.com/Mu2e/artdaq_core_mu2e
mrb gitCheckout https://github.com/Mu2e/artdaq_mu2e
mrb gitCheckout https://github.com/Mu2e/otsdaq_mu2e
mrb gitCheckout https://github.com/Mu2e/mu2e_pcie_utils
else
mrb gitCheckout -t ${demo_version} https://github.com/Mu2e/otsdaq_mu2e
fi

git clone git@github.com:Mu2e/otsdaq_mu2e_config

mrb uc


cd $Base
  
ln -s srcs/otsdaq_mu2e_config/setup_ots.sh

# Build artdaq_demo
cd $MRB_BUILDDIR
mrbsetenv
export CETPKG_J=$((`cat /proc/cpuinfo|grep processor|tail -1|awk '{print $3}'` + 1))
mrb build    # VERBOSE=1
installStatus=$?
		
echo
echo

if [ $installStatus -eq 0 ]; then
    echo "otsdaq-demo has been installed correctly. Use 'source setup_ots.sh' to setup your otsdaq software, then follow the instructions or visit the project redmine page for more info: https://github.com/art-daq/otsdaq/wiki"
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

