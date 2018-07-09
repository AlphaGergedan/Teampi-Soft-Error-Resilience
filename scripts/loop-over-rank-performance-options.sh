#!/bin/bash

export TEAMS=2
for interval in constant increasing random; do
    for select in single rr random; do 
        export TMPI_FILE="$interval-$select-timings"
        ./rank-performance-latency.sh $interval $select ./bin/PerfSimulator
    done
done