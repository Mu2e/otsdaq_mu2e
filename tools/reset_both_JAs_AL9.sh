#!/bin/bash

# Call this script reset both JAs on this node
# Usage: ./reset_both_JAs_AL9.sh
#
# Uses the mu2eshift version of JA setup

if [[ "${BASH_SOURCE[0]}" != "${0}" ]]; then
    echo "Error: this script must be executed, not sourced." >&2
    return 1 2>/dev/null || exit 1
fi

SCRIPT_DIR="$(
 cd "$(dirname "$(readlink "$0" || printf %s "$0")")"
 pwd -P
)"

ssh mu2eshift@${HOSTNAME} bash  /home/mu2eshift/JA_ots_setup.sh

# Print a summary result
echo -e "$(date +%d%b%y.%T) reset_both_JAs_AL9.sh:${LINENO} |  \t ===> Done with JA setup on ${HOSTNAME}!"
