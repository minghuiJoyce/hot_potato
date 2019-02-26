all: player ringmaster
player: potato.h player.c
	gcc -g player.c -o player
ringmaster: potato.h ringmaster.c
	gcc -g ringmaster.c -o ringmaster

clean:
	rm player ringmaster
 
