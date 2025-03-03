#!/bin/sh

# "$@" to pass all args to child call
# use grep to show only single lines from DCS_test source
# use sed to hide TRACE preamble decoration
DCS_test "$@" 2>&1 | grep "DCS_test.cc:" | grep '^|' | sed -E 's/\|.*\|(.*)/\1/'

