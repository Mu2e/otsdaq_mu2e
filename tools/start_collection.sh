#!/bin/bash

# This is temporary and needs to be replaced by actually pulling the contexts from the DB
CONTEXTS=(05 06 14 17 16)
CONTEXT_PREFIX="mu2edaq"

OTS_PATH=test_stand/ots

CMD="~/test_stand/ots/collect.sh"

#echo ${CONTEXT_PREFIX}${CONTEXTS[@]}

# Start each collection simultaneously

for i in ${CONTEXTS[@]}; do
	ssh "${CONTEXT_PREFIX}$i" "$CMD" &
done

wait

echo "Collection complete"

