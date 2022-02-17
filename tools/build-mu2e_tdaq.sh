#!/bin/bash

# build in $WORKSPACE/build
# copyback directory is $WORKSPACE/copyBack

usage()
{
  cat 1>&2 <<EOF
Usage: $(basename ${0}) [-h]

Options:

  -h    This help.

EOF
}

working_dir=${WORKSPACE}
qual_set="${QUAL}"
build_type=${BUILDTYPE}
copyback_deps=${COPYBACK_DEPS}

IFS_save=$IFS
IFS=":"
read -a qualarray <<<"$qual_set"
IFS=$IFS_save
basequal=
squal=
pyflag=
build_db=1

# Remove shared memory segments which have 0 nattach
killall art && sleep 5 && killall -9 art
killall transfer_driver
for key in `ipcs|grep " $USER "|grep " 0 "|awk '{print $1}'`;do ipcrm -M $key;done

for qual in ${qualarray[@]};do
	case ${qual} in
		e*)		basequal=${qual};;
		c*)		basequal=${qual};;
		nodb)	build_db=0;;
		s*)     squal=${qual};;
		esac
done

if [[ "x$squal" == "x" ]] || [[ "x$basequal" == "x" ]]; then
	echo "unexpected qualifier set ${qual_set}"
	usage
	exit 1
fi

case ${build_type} in
    debug) ;;
    prof) ;;
    *)
	echo "ERROR: build type must be debug or prof"
	usage
	exit 1
esac

OS=`uname`
if [ "${OS}" = "Linux" ]
then
    flvr=slf`lsb_release -r | sed -e 's/[[:space:]]//g' | cut -f2 -d":" | cut -f1 -d"."`
else 
  echo "ERROR: unrecognized operating system ${OS}"
  exit 1
fi
echo "build flavor is ${flvr}"
echo ""

qualdir=`echo ${qual_set} | sed -e 's%:%-%'`

set -x

srcdir=${working_dir}/source
blddir=${working_dir}/build
# start with clean directories
rm -rf ${blddir}
rm -rf ${srcdir}
rm -rf $WORKSPACE/copyBack 
# now make the dfirectories
mkdir -p ${srcdir} || exit 1
mkdir -p ${blddir} || exit 1
mkdir -p $WORKSPACE/copyBack || exit 1

cd ${blddir} || exit 1

curl --fail --silent --location --insecure -O http://scisoft.fnal.gov/scisoft/bundles/tools/buildFW || exit 1
chmod +x buildFW

mv ${blddir}/*source* ${srcdir}/

cd ${blddir} || exit 1

echo
echo "begin build"
echo
./buildFW -t -b ${basequal} -s ${squal} ${blddir} ${build_type} mu2e_tdaq_software-1.00.00 || \
 { mv ${blddir}/*.log  $WORKSPACE/copyBack/
   exit 1 
 }

source ${blddir}/setups

if [ $copyback_deps == "false" ]; then
  echo "Removing art, artdaq and otsdaq bundle products"
  for file in ${blddir}/*.bz2;do
    filebase=`basename $file`
    if [[ "${filebase}" =~ "otsdaq_mu2e" ]]; then
        echo "Not deleting ${filebase}"
    elif [[ "${filebase}" =~ "mu2e_artdaq" ]]; then
        echo "Not deleting ${filebase}"
    elif [[ "${filebase}" =~ "pcie_linux" ]]; then
        echo "Not deleting ${filebase}"
    else
        echo "Deleting ${filebase}"
	    rm -f $file
    fi
  done
  rm -f ${blddir}/art-*_MANIFEST.txt
fi

echo
echo "move files"
echo
mv ${blddir}/*.bz2  $WORKSPACE/copyBack/
mv ${blddir}/*.txt  $WORKSPACE/copyBack/
mv ${blddir}/*.log  $WORKSPACE/copyBack/

echo
echo "cleanup"
echo
rm -rf ${blddir}
rm -rf ${srcdir}

exit 0
