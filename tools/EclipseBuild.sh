#!/bin/bash
cd $OTSDAQDEMO_BUILD/..
source setupARTDAQOTS
cd $OTSDAQDEMO_BUILD
source $OTSDAQDEMO_REPO/ups/setup_for_development -p
buildtool $@

