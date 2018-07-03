if (( $# < 4)); then
    echo "ERROR: At least four parameters are required"
    echo "Usage: { constant | increasing | random } { single | rr | random } application [application args...]"
    exit 1
fi

mpirun -np 4 ${@:3} &
sleep 5
pids=($(pgrep $3))

iteration = 1

while kill -0 ${pids[0]}; do
    rank=-1
    if [ $2 = "single" ]; then
        rank=0
    fi

    if [ $2 = "rr" ]; then
        let "rank = ($rank + 1) % 4"
    fi

    if [ $2 = "random" ]; then
        rank = `python3 -c "from random import randint; print(randint(0,3))"`
    fi

    kill -USR1 $rank

    if [ $1 = "constant" ]; then
        sleep 5
    fi

    if [ $1 = "increasing" ]; then
        sleep $((50.0 / iteration))
    fi

    if [ $1 = "random"]; then
        sleep `python3 -c "from random import randint; print(randint(1,50))"`
    fi
    ((iteration++))
done




