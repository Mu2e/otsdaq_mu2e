#!/bin/bash
#
# author:           rrivera at fnal dot gov
# last modified:    26-Feb-2026
# last modified by: rrivera at fnal dot gov
#
# change log:
#   -- 26-Feb-2026: Initial version of the script. It is a wrapper to the mu2e-quick-spack-start.sh script, which is used to install the otsdaq
#
# This install worked on 25-Feb-2026 in MC2 on dl-01
#   - it took ~30 minutes for quick spack start
#   - plus it took ~30 minutes for the mz_uc clean build
#   - resulting srcs directory (all on develop branch, not fetched nor pulled from remote):
#       artdaq-core-mu2e
#       artdaq-mu2e
#       mu2e-pcie-utils
#       mu2e-tdaq-suite
#       mu2e-trig-config    -- on main branch
#       Offline             -- on main branch
#       otsdaq
#       otsdaq-components
#       otsdaq-epics
#       otsdaq-mu2e
#       otsdaq-mu2e-calorimeter
#       otsdaq-mu2e-crv
#       otsdaq-mu2e-dqm
#       otsdaq-mu2e-extmon
#       otsdaq-mu2e-stm
#       otsdaq-mu2e-tracker
#       otsdaq-mu2e-trigger
#       otsdaq-suite
#       otsdaq-utilities
#
#   - Note: that the otsdaq* repos are all on the develop branch, but not fetched nor pulled.
#       They will be at whatever hash has been fetched in ots_ops_dev (which should be known to be compilable as a set).
#   - If you want to pull the latest of all repos in srcs/, do the following:
#       UpdateOTS.sh --pullall
#

OTS_OPS_DEV_PATH="/home/mu2eshift/ots_ops_dev"

echo -e "$(date +%d%b%y.%T) install_mu2e_ops_dev.sh:${LINENO} \t This script should be sourced on mu2e-dl-01 in MC2. Will copy srcs/otsdaq* from ${OTS_OPS_DEV_PATH}"

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
	echo -e "$(date +%d%b%y.%T) install_mu2e_ops_dev.sh:${LINENO} \t Error: This script must be sourced, not executed"
	exit 1
fi

if [[ -z "$1" ]]; then
	echo -e "$(date +%d%b%y.%T) install_mu2e_ops_dev.sh:${LINENO} \t Error: Missing argument: please provide a target directory name"
	return 1
fi
if [[ -d "$1" ]]; then
	echo -e "$(date +%d%b%y.%T) install_mu2e_ops_dev.sh:${LINENO} \t Error: Directory $1 already exists. Please provide a new directory name."
	return 1
fi
if ! mkdir -- "$1"; then
	echo -e "$(date +%d%b%y.%T) install_mu2e_ops_dev.sh:${LINENO} \t Error: Failed to create directory $1"
	return 1
fi
cd -- "$1"

# Get the latest mu2e-quick-spack-start.sh
baseurl="https://raw.githubusercontent.com/Mu2e/otsdaq_mu2e/refs/heads/develop/tools"
latestscript=$(curl -s "$baseurl/mu2e-quick-spack-start.sh")
curl "$baseurl/$latestscript" -o mu2e-quick-spack-start.sh
chmod +x mu2e-quick-spack-start.sh
unset SPACK_ROOT # just in case there is another installation hanging around in your bash environment
# Specify --tag (e.g. --tag v3_04_00) to build your area against a fixed tag of the software. (Defaults to the latest tag in otsdaq-mu2e)
./mu2e-quick-spack-start.sh --trigger --develop --dev-only # Omit --trigger if you are not developing any code that depends on data overlays or Offline code

ll srcs/
if [ ! -d "srcs" ]; then
	echo -e "$(date +%d%b%y.%T) install_mu2e_ops_dev.sh:${LINENO} \t Error: Install failed to create srcs directory"
	return 1
fi

#copy dev sources over
rm -rf srcs/otsdaq*
cp -r "$OTS_OPS_DEV_PATH/srcs/otsdaq"* srcs/.

#cleanup vestiges of the install
rm setup-env.sh
rm setup_spack_build_system_v0.28.sh
rm fonts.*
rm mu2e-quick-spack-start.sh
ll

#get scripts and make links
cp -r "$OTS_OPS_DEV_PATH/otsdaq-mu2e-config" .
ln -s otsdaq-mu2e-config/setup_kinit.sh kinit_setup.sh
ln -s otsdaq-mu2e-config/hwdev_spack_fast_setup_ots.sh setup_ots.sh
ll
cp "$OTS_OPS_DEV_PATH/MacroMakerMode_*" .
cp "$OTS_OPS_DEV_PATH/mongodb_setup.sh" .
cp "$OTS_OPS_DEV_PATH/ecl_setup_ots.sh" .
cp "$OTS_OPS_DEV_PATH/db_setup_ots.sh" .
ll
#get USER_DATA areas
cp -r "$OTS_OPS_DEV_PATH/Data_"* .
ll

#clean build with new srcs
. setup_ots.sh cfo
UpdateOTS.sh --develop #move all srcs repos to develop branch
mz_uc
