#!/bin/bash

export TEAMS=2
for interval in constant increasing random; do
    for select in single rr  random; do 
        export TMPI_FILE="$interval-$select-timings"
        ./rank-performance-latency.sh random random ./bin/Latency 100000 100000 1
    done
done