#!/bin/bash
set -x

clients_no=${1:-10}

for i in $(seq 1 $clients_no); do
    echo Launching client $i
    ./client $i &
done

wait # wait for all background tasks

exit 0
