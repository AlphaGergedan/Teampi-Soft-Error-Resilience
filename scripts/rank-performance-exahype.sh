if (( $# < 4)); then
    echo "ERROR: At least four parameters are required"
    echo "Usage: { constant | increasing | random } { single | rr | random } application [application args...]"
    exit 1
fi

echo "Execute ${@:3} &"
sleep 5
pids=($(pgrep $3))

while kill -0 ${pids[0]}; do
    if [ $2 = "single" ]; then
    # Select same rank each time
    fi

    if [ $2 = "rr" ]; then
    #select next rank
    fi

    if [ $2 = "random" ]; then
    #select a random rank
    fi

    kill -USR1 $rank

    if [ $1 = "constant" ]; then
    # wait 5s
    fi

    if [ $1 = "increasing" ]; then
    #wait 50s, 25s, 12.5s...
    fi

    if [ $1 = "random"]; then
    #wait rand(0.1, 100)
    fi
done




