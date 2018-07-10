#!/bin/bash
if (( $# < 3)); then
    echo "ERROR: At least three parameters are required"
    echo "Usage: { constant | increasing | random } { single | rr | random } application [application args...]"
    exit 1
fi

mpirun -np 4 ${@:3} &

sleep 2
pids=($(pgrep PerfSimulator))

iteration=1

while true; do
    rank=-1
    if [ $2 = "single" ]; then
        rank=0
    fi

    if [ $2 = "rr" ]; then
        let "rank = ($iteration - 1) % 4"
    fi

    if [ $2 = "random" ]; then
        rank=`python3 -c "from random import randint; print(randint(0,3))"`
    fi
    
    if  kill -0 ${pids[0]} ; then
        kill -USR1  ${pids[$rank]}
    else 
        exit 1
    fi

    if [ $1 = "constant" ]; then
        sleep 5
    fi

    if [ $1 = "increasing" ]; then
        sleep $(python -c "print(max(2,20.0/$iteration))")
    fi

    if [ $1 = "random" ]; then
        sleep `python3 -c "from random import uniform; print(uniform(2,10))"`
    fi
    ((iteration++))
done




