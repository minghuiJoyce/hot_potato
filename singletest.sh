#! /bin/bash
pkill -9 ringmaster
pkill -9 player
make clean
make

RINGMASTER_HOSTNAME=vcm-8248.vm.duke.edu
RINGMASTER_PORT=4444
NUM_PLAYERS=1021
NUM_HOPS=512

./ringmaster $RINGMASTER_PORT $NUM_PLAYERS $NUM_HOPS &
# ./ringmaster 4444 2 512 &

sleep 2

for (( i=0; i<$NUM_PLAYERS; i++ ))
do
    ./player $RINGMASTER_HOSTNAME $RINGMASTER_PORT &
done

wait