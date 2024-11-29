#!/usr/bin/env bash

export FADCEXE=./bin/nkfadc500s_run
export RUNNO=`cat RUNNUMBER`
export NEVT=${1:-0}

$FADCEXE $RUNNO $NEVT

echo "///////////////////"
echo "RUNNUMBER: ${RUNNO}"
echo "///////////////////"

if [ $NEVT -ne 0 ]; then
	source ./STOP.sh
fi
