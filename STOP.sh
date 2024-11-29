#!/usr/bin/env bash

export FADCPROC=nkfadc500s_run
export FADCDECODE=./bin/decode_nkfadc500s
export RUNNO=`cat RUNNUMBER`

# Stop current run (FADC) and minitcb
PIDList=`pidof $FADCPROC`
for i in $(echo $PIDList)
do
    kill -s SIGINT $i
done
echo "RUN: $RUNNO finished"

# Decode dat file if it exists, ckim
if compgen -G ./FADCData_${RUNNO}*.dat > /dev/null; then
	mkdir -p ./data
	mv ./log*.txt ./FADCData*.dat ./data
	$FADCDECODE $RUNNO
fi

# Increase Run number by 1 for next run
RUNNO=`expr $RUNNO + 1`
echo "$RUNNO" > RUNNUMBER
