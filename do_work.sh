#!/bin/bash
if [ "$#" -ne 4 ]; then
    echo "usage: do_work.sh START STEP END CPUS"
    exit 1
fi
START=$1
STEP=$2
END=$3
NUMCPU=$4

# default to 11, the number to beat
MAX=11

# kill child processes if script cancelled
#trap "trap - SIGTERM && kill -- -$$" SIGINT SIGTERM EXIT

active=0
for (( n=$START; n <= $END-$STEP; n += $STEP )); do
    if (( active++ >= $NUMCPU )); then
        wait -n
        (( active-- ))
    fi
    ./multper "$n" "$((n + $STEP))" "$MAX"&
done
wait
