#!/usr/bin/env bash

export FADCINIT=./bin/nkfadc500s_setup
export RUNNO=`cat RUNNUMBER`

$FADCINIT -r  > log_${RUNNO}.txt; cat log_${RUNNO}.txt

echo "/////////////////"
echo "RUNNUMBER: $RUNNO"
echo "/////////////////"
