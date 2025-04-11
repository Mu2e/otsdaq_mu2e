#! /bin/bash
# quick-mrb-start.sh - Eric Flumerfelt, May 20, 2016
# Downloads, installs, and runs the artdaq_demo as an MRB-controlled repository

git_status=`git status 2>/dev/null`
git_sts=$?
if [ $git_sts -eq 0 ];then
    echo "This script is designed to be run in a fresh install directory!"
    exit 1
fi

starttime=`date`
Base=$PWD
test -d log || mkdir log
test -d Data && rmdir Data
test -d databases && rmdir databases

env_opts_var=`basename $0 | sed 's/\.sh$//' | tr 'a-z-' 'A-Z_'`_OPTS
USAGE="\
   usage: `basename $0` [options] [demo_root]
examples: `basename $0` .
          `basename $0` --debug
          `basename $0` --tag v2_08_04
If the \"demo_root\" optional parameter is not supplied, the user will be
prompted for this location.
--debug       perform a debug build
--develop     Install the develop version of the software (may be unstable!)
--dev-only    Do not install the suite in an environment (use with upstreams!)
--tag         Install a specific tag of mu2e-tdaq-suite
--spackdir    Install Spack in this directory (or use existing installation)
--all-packages Install all packages including Offline and otsdaq-mu2e-trigger
--trigger     Synonym for --all-packages
-a            Artdaq version number (e.g. 31300 for v3_13_00)
-o            Otsdaq version number (e.g. 20800 for v2_08_00)
-s            Use specific qualifiers when building ots
-v            Be more verbose
-x            set -x this script
-w            Check out repositories read/write
--no-extra-products  Skip the automatic use of central product areas, such as CVMFS
--upstream    Use <dir> as a Spack upstream (repeatable)
--padding     Pad directories to 255 characters for relocatability
--arch        Set architechture for build (ex. linux-almalinux9-x86_64_v3)
--no-kmod     Do not build TRACE kernel module (for Docker builds)
--no-view     Do not create a Spack environment view
--no-emacs    Do not attempt to install emacs
--no-auto-upstream Do not search /mu2e/spack_areas for upstreams
--all-packages Used with --develop, will fetch all subdetector repos
"

# Process script arguments and options
eval env_opts=\${$env_opts_var-} # can be args too

spackdir="${SPACK_ROOT:-$Base/spack}"
upstreams=()
installStatus=0
eval "set -- $env_opts \"\$@\""
op1chr='rest=`expr "$op" : "[^-]\(.*\)"`   && set -- "-$rest" "$@"'
op1arg='rest=`expr "$op" : "[^-]\(.*\)"`   && set --  "$rest" "$@"'
reqarg="$op1arg;"'test -z "${1+1}" &&echo opt -$op requires arg. &&echo "$USAGE" &&exit'
args= do_help= opt_v=0; opt_w=0; opt_develop=0; opt_skip_extra_products=0; opt_no_pull=0; opt_padding=0; opt_no_kmod=0; opt_all_packages=0; opt_no_view=0; opt_no_emacs=0; opt_dev_only=0; opt_no_auto_upstream=0;
while [ -n "${1-}" ];do
    if expr "x${1-}" : 'x-' >/dev/null;then
        op=`expr "x$1" : 'x-\(.*\)'`; shift   # done with $1
        leq=`expr "x$op" : 'x-[^=]*\(=\)'` lev=`expr "x$op" : 'x-[^=]*=\(.*\)'`
        test -n "$leq"&&eval "set -- \"\$lev\" \"\$@\""&&op=`expr "x$op" : 'x\([^=]*\)'`
        case "$op" in
            \?*|h*)     eval $op1chr; do_help=1;;
            v*)         eval $op1chr; opt_v=`expr $opt_v + 1`;;
            x*)         eval $op1chr; set -x;;
            a*)         eval $op1arg; aqualifier=$1; shift;;
            o*)         eval $op1arg; oqualifier=$1; shift;;
            s*)         eval $op1arg; squalifier=$1; shift;;
            w*)         eval $op1chr; opt_w=`expr $opt_w + 1`;;
            -debug)     opt_debug=--debug;;
            -develop) opt_develop=1;;
            -dev-only)   opt_dev_only=1;;
            -tag)       eval $reqarg; tag=$1; shift;;
            -spackdir)  eval $op1arg; spackdir=$1; shift;;
            -no-extra-products)  opt_skip_extra_products=1;;
            -no-pull)   opt_no_pull=1;;
            -upstream)  eval $op1arg; upstreams+=($1); shift;;
            -padding)   opt_padding=1;;
            -arch)      eval $op1arg; arch=$1; shift;;
            -no-kmod)   opt_no_kmod=1;;
            -no-emacs)  opt_no_emacs=1;;
                        -no-auto-upstream) opt_no_auto_upstream=1;;
            -all-packages) opt_all_packages=1;;
        -trigger)   opt_all_packages=1;;
            -no-view)   opt_no_view=1;;
            *)          echo "Unknown option -$op"; do_help=1;;
        esac
    else
        aa=`echo "$1" | sed -e"s/'/'\"'\"'/g"` args="$args '$aa'"; shift
    fi
done
eval "set -- $args \"\$@\""; unset args aa

test -n "${do_help-}" -o $# -ge 2 && echo "$USAGE" && exit

if [ "x$SPACK_ROOT" == "x$spackdir" ]; then
  echo "Using pre-existing Spack installation $SPACK_ROOT.\nIf this is not correct, hit Ctrl-C and run 'unset SPACK_ROOT'."
  sleep 5
  spack env deactivate
fi

# Save all output from this script (stdout + stderr) in a file with a
# name that looks like "quick-start.sh_Fri_Jan_16_13:58:27.script" as
# well as all stderr in a file with a name that looks like
# "quick-start.sh_Fri_Jan_16_13:58:27_stderr.script"
alloutput_file=$( date | awk -v "SCRIPTNAME=$(basename $0)" '{print SCRIPTNAME"_"$1"_"$2"_"$3"_"$4".script"}' )
stderr_file=$( date | awk -v "SCRIPTNAME=$(basename $0)" '{print SCRIPTNAME"_"$1"_"$2"_"$3"_"$4"_stderr.script"}' )
exec  > >(tee "$Base/log/$alloutput_file")
exec 2> >(tee "$Base/log/$stderr_file")

# Get all the information we'll need to decide which exact flavor of the software to install
notag=0
if [ -z "${tag:-}" ]; then
  tag=develop;
  notag=1;
fi

rm CMakeLists.txt*
wget https://raw.githubusercontent.com/Mu2e/otsdaq-mu2e/$tag/CMakeLists.txt
demo_version=v`grep "project" $Base/CMakeLists.txt|grep -oE "VERSION [^)]*"|awk '{print $2}'|sed 's/\./_/g'`
echo "Mu2e TDAQ Version is $demo_version"
if [[ $notag -eq 1 ]] && [[ $opt_develop -eq 0 ]]; then
  tag=$demo_version

  # 06-Mar-2017, KAB: re-fetch the product_deps file based on the tag
  mv CMakeLists.txt CMakeLists.txt.orig
  wget https://raw.githubusercontent.com/Mu2e/otsdaq-mu2e/$tag/CMakeLists.txt
  demo_version=v`grep "project" $Base/CMakeLists.txt|grep -oE "VERSION [^)]*"|awk '{print $2}'|sed 's/\./_/g'`
  tag=$demo_version
fi

svariant=""
avariant=""
ovariant=""

if [ -n "${squalifier-}" ]; then
    svariant="s=${squalifier}"
fi
if [ -n "${aqualifier-}" ]; then
    avariant="artdaq=${aqualifier}"
fi
if [ -n "${oqualifier-}" ]; then
    ovariant="otsdaq=${oqualifier}"
fi
compiler_info="" # Maybe do e- and c- qualifiers?

arch_opt=""
if [ "x$arch" != "x" ]; then
   arch_opt="arch=$arch"
fi

view_opt=""
if [ $opt_no_view -eq 1 ];then
    view_opt="--without-view"
fi

if ! [ -d $spackdir ];then
    $(
    cd ${spackdir%/spack}
    git clone https://github.com/FNALssi/spack.git -b fnal-develop
    cd $spackdir && git checkout e18ecaaa780b863b2104e2971d3320c97ebf3b65
        )
else
    #cd $spackdir && git pull && cd $Base
    cd $spackdir && git fetch -a && git checkout e18ecaaa780b863b2104e2971d3320c97ebf3b65 && cd $Base
fi

cat >setup-env.sh <<-EOF
export SPACK_DISABLE_LOCAL_CONFIG=true
source $spackdir/share/spack/setup-env.sh
EOF
source setup-env.sh

if ! [ -d fermi-spack-tools ]; then
    git clone https://github.com/FNALssi/fermi-spack-tools.git # Upstream
    #git clone https://github.com/eflumerf/fermi-spack-tools.git # Fork
else
    cd fermi-spack-tools && git pull && cd ..
fi
if ! [ -d spack-mpd ]; then
    # git clone https://github.com/FNALssi/spack-mpd.git # Upstream
    git clone https://github.com/eflumerf/spack-mpd.git # Fork
else
    cd spack-mpd && git pull && cd ..
fi

sed -i '/perl/d' fermi-spack-tools/templates/packagelist
if [ -f $spackdir/etc/spack/`uname -s | tr [A-Z] [a-z]`/almalinux9/packages.yaml ];then
    echo "Skipping ./fermi-spack-tools/bin/make_packages_yaml $spackdir almalinux9"
    echo "... $spackdir/etc/spack/`uname -s | tr [A-Z] [a-z]`/almalinux9/packages.yaml already exists"
else
    echo "executing ./fermi-spack-tools/bin/make_packages_yaml $spackdir almalinux9"
    echo "... to produce $spackdir/etc/spack/`uname -s | tr [A-Z] [a-z]`/almalinux9/packages.yaml"
    ./fermi-spack-tools/bin/make_packages_yaml $spackdir almalinux9
fi

repo_found=`spack repo list|grep -c fnal_art`
if [ $repo_found -eq 0 ]; then
    echo "Adding repos: fnal_art scd_recipes artdaq-spack mu2e-spack"
    mkdir spack-repos;cd spack-repos
    git clone https://github.com/FNALssi/fnal_art.git
    spack repo add ./fnal_art
    git clone https://github.com/marcmengel/scd_recipes.git
    spack repo add ./scd_recipes
    git clone https://github.com/art-daq/artdaq-spack.git
    spack repo add ./artdaq-spack
    git clone https://github.com/Mu2e/mu2e-spack.git
    spack repo add ./mu2e-spack
    cd $Base
else
    echo "Repo's previously added -- pull any updates"
    for dir in `spack repo list|awk '{print $2}'`;do
        cd $dir
        git pull
    done
    cd $Base
fi

spack config --scope=site add "config:extensions:- $Base/spack-mpd"

if [ $opt_padding -eq 1 ];then
  spack config --scope=site add config:install_tree:padded_length:255
fi

concrete_include_cmd=

# Auto-add upstreams from /mu2e
if [ $opt_no_auto_upstream -eq 0 ] && [ -d /mu2e/spack_areas ];then
  art=`ls -d /mu2e/spack_areas/art-suite-*|tail -1`
  artdaq=`ls -d /mu2e/spack_areas/artdaq-*|tail -1`
  ots=`ls -d /mu2e/spack_areas/ots-*|tail -1`
  mu2e=`ls -d /mu2e/spack_areas/mu2e-tdaq-*|tail -1`

  upstreams+=($mu2e $ots $artdaq $art)
fi

for upstream in ${upstreams[@]}; do
    for upstreamdir in `find $upstream -type f -wholename '*/.spack-db/index.json' 2>/dev/null`; do
        echo "Getting real directory for upstream database $upstreamdir"
        upstreamdir=`dirname $upstreamdir`
        upstreamdir=`dirname $upstreamdir`
        upstreamdir=`realpath $upstreamdir`
        upstreamname=`echo $upstreamdir|sed 's|/__spack[^/]*||g;s|/spack/opt/spack||g'`

        if ! [ -d $upstreamdir/.spack-db ]; then
            echo "No Spack instance found at $upstream!"
            continue
        fi

        if ! [ -f $spackdir/etc/spack/upstreams.yaml ]; then
            echo "upstreams:" > $spackdir/etc/spack/upstreams.yaml
        fi

        if [ `grep -c $upstreamdir $spackdir/etc/spack/upstreams.yaml` -eq 0 ]; then
            # Only add upstream if not already present
            echo "  upstream${upstreamname//\//-}:" >>$spackdir/etc/spack/upstreams.yaml
            echo "    install_tree: $upstreamdir" >>$spackdir/etc/spack/upstreams.yaml
        fi
    done

    for envdir in `find $upstream -type d -wholename '*/var/spack/environments' 2>/dev/null`; do
        echo "Looking for mu2e environments in $envdir"
        environment="tdaq-${demo_version}"
        if ! [ -d $environment ]; then continue; fi
        environment_dir=`realpath $environment`
        echo "Adding environment $environment_dir to include-concrete list"
        concrete_include_cmd="$concrete_include_cmd --include-concrete $environment_dir"
    done

done

spack reindex

cd $Base

BUILD_J=$((`cat /proc/cpuinfo|grep processor|tail -1|awk '{print $3}'` + 1))
spack load --first gcc@13.1.0 >/dev/null 2>&1
if [ $? -ne 0 ];then
  spack install --deprecated -j $BUILD_J gcc@13.1.0 ${arch_opt} +binutils
  installStatus=$?
  spack load gcc@13.1.0
fi
spack compiler find

if [ ${opt_dev_only:-0} -eq 0 ];then
    spack env create ${concrete_include_cmd} $view_opt tdaq-${demo_version}
    spack env activate tdaq-${demo_version}
    env_to_activate="tdaq-${demo_version}"
    ln -s $spackdir/var/spack/environments/tdaq-${demo_version}

    if ! [ -d srcs ];then
        rm srcs >/dev/null 2>&1
        ln -s $spackdir/var/spack/environments/tdaq-${demo_version} srcs
    fi

    if [ $opt_no_kmod -eq 1 ];then
        spack add trace~kmod
    else
        spack add trace+kmod
    fi

    spack add mu2e-tdaq-suite@${demo_version}${compiler_info} ${svariant} ${avariant} ${ovariant} ${arch_opt} ~g4 %gcc@13.1.0

    # Add EMACS
    if [ $opt_no_emacs -eq 0 ]; then
        spack add cairo+X+fc+ft ${arch_opt}
        spack add emacs@29.3%gcc@13.1.0+X toolkit=athena ${arch_opt}
    fi
fi

function checkout_package()
{
    pkg=$1
    if ! [ -d $pkg ]; then
        if [ $opt_w -eq 0 ];then
            git clone https://github.com/Mu2e/$pkg.git $pkg
        else
            git clone git@github.com:Mu2e/$pkg.git $pkg
        fi
    else
        cd $pkg
        git pull
        cd ..
    fi
}

if [[ ${opt_develop:-0} -eq 1 ]];then
    env_to_activate="tdaq-develop"
    cd $Base
    rm srcs
    mkdir srcs
    cd srcs
    for pkg in artdaq-core-mu2e mu2e-pcie-utils artdaq-mu2e otsdaq-mu2e;do # Add more packages?
        checkout_package $pkg
    done
    if [[ ${opt_all_packages:-0} -eq 1 ]]; then
        for pkg in Offline mu2e-trig-config otsdaq-mu2e-calorimeter otsdaq-mu2e-crv otsdaq-mu2e-dqm otsdaq-mu2e-extmon otsdaq-mu2e-stm otsdaq-mu2e-tracker otsdaq-mu2e-trigger;do
            checkout_package $pkg
        done
    fi
    cd $Base
fi

if ! [ -f setup_ots.sh ]; then
    cat >setup_ots.sh <<-EOF
echo # This script is intended to be sourced.

sh -c "[ \`ps \$\$ | grep bash | wc -l\` -gt 0 ] || { echo 'Please switch to the bash shell before running ots.'; exit; }" || exit
export SPACK_DISABLE_LOCAL_CONFIG=true
source $spackdir/share/spack/setup-env.sh

spack load --first gcc@13.1.0
spack compiler find

spack env activate ${env_to_activate}
if [ -d $Base/local/install ]; then
  export PATH=$Base/local/install/bin:\$PATH
  export LD_LIBRARY_PATH=$Base/local/install/lib:\$LD_LIBRARY_PATH
  export CET_PLUGIN_PATH=$Base/local/install/lib:\$CET_PLUGIN_PATH
  export FHICL_FILE_PATH=$Base/local/install/fcl:$FHICL_FILE_PATH
  export MU2E_SEARCH_PATH=$MU2E_SEARCH_PATH:/cvmfs/mu2e.opensciencegrid.org/DataFiles
  export MU2E_SEARCH_PATH=$Base/local/install/fcl:$MU2E_SEARCH_PATH
  export MU2E_SEARCH_PATH=$Base/local/install/share/:$MU2E_SEARCH_PATH
  export ROOT_INCLUDE_PATH=$Base/srcs:$ROOT_INCLUDE_PATH
fi

k5user=\`klist|grep "Default principal"|cut -d: -f2|sed 's/@.*//;s/ //'\`
export TRACE_FILE=/tmp/trace_buffer_\$USER.\$k5user

export OTS_MAIN_PORT=2015

export USER_DATA="$Base/Data"
export ARTDAQ_DATABASE_URI="filesystemdb://$Base/databases/filesystemdb/test_db"
export OTSDAQ_DATA="$Base/Data/OutputData"
export OTS_SOURCE=$Base/srcs

echo -e "setup_ots.sh:\${LINENO} |  \t  Now your user data path is USER_DATA \t\t = \${USER_DATA}"
echo -e "setup_ots.sh:\${LINENO} |  \t  Now your database path is ARTDAQ_DATABASE_URI \t = \${ARTDAQ_DATABASE_URI}"
echo -e "setup_ots.sh:\${LINENO} |  \t  Now your output data path is OTSDAQ_DATA \t = \${OTSDAQ_DATA}"
echo

#make the number of build threads dependent on the number of cores on the machine:
export CETPKG_J=\$((`cat /proc/cpuinfo|grep processor|tail -1|awk '{print $3}'` + 1))

alias  kx='ots -k'
# When using upstream spack-mpd
#alias  mb='date; start_time=\$(date +%s); spack find | grep gcc; spack mpd build -j\$CETPKG_J 2>&1 | sed s/__spack_path_placeholder__//g | sed s/\\\[padded-to-255-chars\\\]//g | sed s/\\\/tdaq-v......../\\\/tdaq-v_\ \ \ /g; end_time=\$(date +%s); pushd $Base/build; ninja install; popd; date; delta_time=\$((end_time - start_time)); fractional_minutes=\$(echo "scale=1; \$delta_time / 60" | bc); echo "Full time: \$delta_time seconds or \$fractional_minutes minutes"'
#alias  ml='date; start_time=\$(date +%s); spack find | grep gcc; spack mpd build -j\$CETPKG_J 2>&1 | sed s/__spack_path_placeholder__//g | sed s/\\\[padded-to-255-chars\\\]//g | sed s/\\\/tdaq-v......../\\\/tdaq-v_\ \ \ /g | tee m.txt; end_time=\$(date +%s); pushd $Base/build; ninja install; popd; date; delta_time=$((end_time - start_time)); fractional_minutes=\$(echo "scale=1; \$delta_time / 60" | bc); echo "Full time: \$delta_time seconds or \$fractional_minutes minutes"; less m.txt'
#alias  mz='date; start_time=\$(date +%s); spack concretize --force --deprecated; spack mpd build --clean -j\$CETPKG_J 2>&1 | sed s/__spack_path_placeholder__//g; end_time=\$(date +%s); pushd $Base/build; ninja install; popd; date; delta_time=\$((end_time - start_time)); fractional_minutes=\$(echo "scale=1; \$delta_time / 60" | bc); echo "Full time: \$delta_time seconds or \$fractional_minutes minutes"'
# When using the fork of spack-mpd
alias  mb='date; start_time=\$(date +%s); spack find | grep gcc; spack mpd build -G Ninja -j\$CETPKG_J 2>&1 | sed s/__spack_path_placeholder__//g | sed s/\\\[padded-to-255-chars\\\]//g | sed s/\\\/tdaq-v......../\\\/tdaq-v_\ \ \ /g; end_time=\$(date +%s); pushd $Base/build; ninja install; popd; date; delta_time=\$((end_time - start_time)); fractional_minutes=\$(echo "scale=1; \$delta_time / 60" | bc); echo "Full time: \$delta_time seconds or \$fractional_minutes minutes"'
alias  ml='date; start_time=\$(date +%s); spack find | grep gcc; spack mpd build -G Ninja -j\$CETPKG_J 2>&1 | sed s/__spack_path_placeholder__//g | sed s/\\\[padded-to-255-chars\\\]//g | sed s/\\\/tdaq-v......../\\\/tdaq-v_\ \ \ /g | tee m.txt; end_time=\$(date +%s); pushd $Base/build; ninja install; popd; date; delta_time=$((end_time - start_time)); fractional_minutes=\$(echo "scale=1; \$delta_time / 60" | bc); echo "Full time: \$delta_time seconds or \$fractional_minutes minutes"; less m.txt'
alias  mz='date; start_time=\$(date +%s); spack concretize --force --deprecated; spack mpd build -G Ninja --clean -j\$CETPKG_J 2>&1 | sed s/__spack_path_placeholder__//g; end_time=\$(date +%s); pushd $Base/build; ninja install; popd; date; delta_time=\$((end_time - start_time)); fractional_minutes=\$(echo "scale=1; \$delta_time / 60" | bc); echo "Full time: \$delta_time seconds or \$fractional_minutes minutes"'


echo
echo -e "setup_ots.sh:\${LINENO} |  \t  Now use 'ots --wiz' to configure otsdaq"
echo -e "setup_ots.sh:\${LINENO} |  \t   	Then use 'ots' to start otsdaq"
echo -e "setup_ots.sh:\${LINENO} |  \t   	Or use 'ots --help' for more options"
echo
echo -e "setup_ots.sh:\${LINENO} |  \t      use 'kx' to kill otsdaq processes"
echo

echo -e "setup_ots.sh:\${LINENO} |  \t  "
echo -e "setup_ots.sh:\${LINENO} |  \t      setup_ots.sh creates some compiling aliases for you:"
echo -e "setup_ots.sh:\${LINENO} |  \t     ---------------"
echo -e "setup_ots.sh:\${LINENO} |  \t            mb                             ### for incremental build"
echo -e "setup_ots.sh:\${LINENO} |  \t            mz                             ### for clean build"
echo -e "setup_ots.sh:\${LINENO} |  \t     ---------------"
echo -e "setup_ots.sh:\${LINENO} |  \t  "
echo -e "setup_ots.sh:\${LINENO} |  \t  "

EOF
#
fi

########################################
########################################
## Setup USER_DATA and databases
########################################
########################################
cd $Base

export USER_DATA="$Base/Data"
export ARTDAQ_DATABASE_URI="filesystemdb://$Base/databases/filesystemdb/test_db"


########################################
########################################
## END Setup USER_DATA and databases
########################################
########################################

if [ ${opt_dev_only:-0} -eq 0 ];then
    spack concretize --force --deprecated && spack install --deprecated -j $BUILD_J
    installStatus=$?
fi

if [[ ${opt_develop:-0} -eq 1 ]];then
    spack env deactivate
    # spack mpd init # Upstream
    spack mpd init -r site -u $Base/spack-repos/mpd # Fork
    if [ ${opt_dev_only:-0} -eq 0 ];then
        # spack mpd new-project --force -y --name tdaq-develop -E tdaq-${demo_version} cxxstd=20 %gcc@13.1.0 generator=ninja # Upstream
        spack mpd new-project --force -y --name tdaq-develop -E tdaq-${demo_version} cxxstd=20 %gcc@13.1.0 # Fork
    else
        # spack mpd new-project --force -y --name tdaq-develop cxxstd=20 %gcc@13.1.0 generator=ninja # Upstream
        spack mpd new-project --force -y --name tdaq-develop cxxstd=20 %gcc@13.1.0 # Fork
    fi
    spack env activate tdaq-develop
    spack concretize --force --deprecated && spack install --deprecated
    # spack mpd build # Upstream
    spack mpd build -G Ninja # Fork
    cd $Base/build
    ninja install
    installStatus=$?
    cd $Base
fi

if [ $installStatus -eq 0 ]; then
    echo "mu2e-tdaq-suite has been installed correctly. Use 'source setup_ots.sh' to setup your otsdaq software, then follow the instructions or visit the project redmine page for more info: https://github.com/art-daq/otsdaq/wiki"
    echo
    echo "In the future, when you open a new terminal, just use 'source setup_ots.sh' to setup your ots installation."
    echo
else
    echo "BUILD ERROR!!! SOMETHING IS VERY WRONG!!!"
    echo
    echo
fi

endtime=`date`

echo "Build start time: $starttime"
echo "Build end time:   $endtime"

exit $installStatus
