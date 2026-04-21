#!/bin/bash

# Call this logbook posting script with a message (in quotes)
# Usage: ./post_to_ecl.sh "message"
#
# This script will decorate your post with your username
#
# Uses the mu2eshift area to post

if [[ "${BASH_SOURCE[0]}" != "${0}" ]]; then
    echo "Error: this script must be executed, not sourced." >&2
    return 1 2>/dev/null || exit 1
fi

SCRIPT_DIR="$(
 cd "$(dirname "$(readlink "$0" || printf %s "$0")")"
 pwd -P
)"


if [[ "x$1" == "x" ]]; then
    echo -e "$(date +%d%b%y.%T) otsdaq-mu2e/tools/post_to_ecl.sh:${LINENO} |  \t Missing title and message! Usage: ./post_to_ecl.sh \"title\" \"message\""
    exit 1
fi
TITLE=$1
shift
echo -e "$(date +%d%b%y.%T) otsdaq-mu2e/tools/post_to_ecl.sh:${LINENO} |  \t TITLE = $TITLE"

if [[ "x$1" == "x" ]]; then
    echo -e "$(date +%d%b%y.%T) otsdaq-mu2e/tools/post_to_ecl.sh:${LINENO} |  \t Missing message! Usage: ./post_to_ecl.sh \"title\" \"message\""
    exit 1
fi
MESSAGE=$1
shift
echo -e "$(date +%d%b%y.%T) otsdaq-mu2e/tools/post_to_ecl.sh:${LINENO} |  \t MESSAGE = $MESSAGE"


ssh mu2eshift@${HOSTNAME} bash  /home/mu2eshift/ecl_post.sh \"$TITLE\" \"$MESSAGE\"

# Print a summary result
echo -e "$(date +%d%b%y.%T) otsdaq-mu2e/tools/post_to_ecl.sh:${LINENO} |  \t ===> Done with ecl post from ${HOSTNAME}!"
