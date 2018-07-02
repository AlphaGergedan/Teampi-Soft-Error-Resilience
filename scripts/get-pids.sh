#!/bin/bash

pids=($(pgrep $1))
for i in ${pids[*]}; do
    echo $i
done
