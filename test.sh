pkill -9 ringmaster
pkill -9 player


make clean
make

RINGMASTER_HOSTNAME=vcm-8250.vm.duke.edu
RINGMASTER_PORT=4444
NUM_PLAYERS=800
NUM_HOPS=512

NUM_PLAYERS_IN=600

./ringmaster $RINGMASTER_PORT $NUM_PLAYERS $NUM_HOPS &

sleep 2

for (( i=0; i<$NUM_PLAYERS_IN; i++ ))
do
    ./player $RINGMASTER_HOSTNAME $RINGMASTER_PORT &
done
wait