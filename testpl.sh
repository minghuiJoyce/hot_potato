pkill -9 ringmaster
pkill -9 player

make clean
make

RINGMASTER_HOSTNAME=vcm-8250.vm.duke.edu
RINGMASTER_PORT=8862
NUM_PLAYERS=200


sleep 2

for (( i=0; i<$NUM_PLAYERS; i++ ))
do
    ./player $RINGMASTER_HOSTNAME $RINGMASTER_PORT &
done

wait
