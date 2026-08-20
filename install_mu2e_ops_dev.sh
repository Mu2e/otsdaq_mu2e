#!/bin/bash
#
# author:           rrivera at fnal dot gov
# last modified:    26-Feb-2026
# last modified by: rrivera at fnal dot gov
#
# change log:
#   -- 26-Feb-2026: Initial version of the script. It is a wrapper to the mu2e-quick-spack-start.sh script, which is used to install the otsdaq
#
# In new terminal, should source this script on the NFS host (mu2e-mgr-01 in MC2, mu2edaq13 in HEERC)
#	to create a new development area for otsdaq-mu2e.
# 	It will copy the srcs/otsdaq* repos from an existing area (default: /home/mu2eshift/ots_ops_dev)
#	to the new area.
#
# Example execution from new terminal at PWD = ~mu2etrk on mu2e-mgr-01 in MC2:
# 		baseurl="https://raw.githubusercontent.com/Mu2e/otsdaq_mu2e/refs/heads/develop"
# 		curl "$baseurl/install_mu2e_ops_dev.sh" -o install_mu2e_ops_dev.sh
# 		source install_mu2e_ops_dev.sh <dev area folder name> [optional area to copy from]
#
# This install worked on 20-Aug-2026 in MC2 on mgr-01 from /home/mu2etrk/ots_tmp
#		> source install_mu2e_ops_dev.sh myots
#   - Started at 14:02
#	- 15:52: done with install, copy, spack setup.. now concretizing...
#   - 15:55: starting mz_uc for a clean build, have to click Y and enter cores
#	- 16:06: done with mz_uc.
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
# $2 (optional): override OTS_OPS_DEV_PATH with an existing directory to build from another area's work
if [[ -n "$2" && -d "$2" ]]; then
	OTS_OPS_DEV_PATH="$2"
fi

echo -e "$(date +%d%b%y.%T) install_mu2e_ops_dev.sh:${LINENO} \t This script should be sourced on mu2e-mgr-01 in MC2. Will copy srcs/otsdaq* from ${OTS_OPS_DEV_PATH}"

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
	echo -e "$(date +%d%b%y.%T) install_mu2e_ops_dev.sh:${LINENO} \t Error: This script must be sourced, not executed"
	exit 1
fi

if [[ ! -d "$OTS_OPS_DEV_PATH" ]]; then
	echo -e "$(date +%d%b%y.%T) install_mu2e_ops_dev.sh:${LINENO} \t Error: OTS_OPS_DEV_PATH directory does not exist: $OTS_OPS_DEV_PATH"
	return 1
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
./mu2e-quick-spack-start.sh --trigger --develop #--dev-only # Omit --trigger if you are not developing any code that depends on data overlays or Offline code

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
rm setup_ots_rte.sh
rm setup_ots.sh
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

echo -e "$(date +%d%b%y.%T) install_mu2e_ops_dev.sh:${LINENO} \t Install complete!"
echo -e "$(date +%d%b%y.%T) install_mu2e_ops_dev.sh:${LINENO} \t    Note: the otsdaq* repos are all on the develop branch, but not fetched nor pulled."
echo -e "$(date +%d%b%y.%T) install_mu2e_ops_dev.sh:${LINENO} \t    They will be at whatever hash has been fetched in ${OTS_OPS_DEV_PATH} (which should be known to be compilable as a set)."
echo -e "$(date +%d%b%y.%T) install_mu2e_ops_dev.sh:${LINENO} \t    If you want to pull the latest of all repos in srcs/, do the following:"
echo -e "$(date +%d%b%y.%T) install_mu2e_ops_dev.sh:${LINENO} \t        UpdateOTS.sh --pullall"
