#!/bin/bash

# Opens a remote file somewhat transparently using tail -f

echo "Opening remove file in less from mu2edaq${1}.fnal.gov: $2"

ssh mu2edaq${1}.fnal.gov tail -f $2


