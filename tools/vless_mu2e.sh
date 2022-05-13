#!/bin/bash

# Opens a remote file somewhat transparently using less

echo "Opening remove file in less from mu2e${1}.fnal.gov: $2"

scp mu2e${1}.fnal.gov:$2 .tmpLogFile && less .tmpLogFile && rm .tmpLogFile



