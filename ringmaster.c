#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "potato.h"

potato hot_potato;


void processInput(int argc, char* argv[]){
    if(argc != 4){
    printf("Usage: ringmaster <port_num> <num_players> <num_hops>\n");
    exit(EXIT_FAILURE);
  }
  int numPlyers = atoi(argv[2]);
  if(numPlyers<=1){
    printf("Number of players must be greater than 1.\n");
    exit(EXIT_FAILURE);
  }
  int numHops = atoi(argv[3]);
  if(numHops<0 || numHops > 512){
    printf("Number of hops must: 0<= hops <= 512\n");
    exit(EXIT_FAILURE);
  }
  hot_potato.hops = numHops;

}
void printTrace(potato* hpotato, int numHops){
	printf("Trace of potato:\n");
  for(int i = numHops-1; i>=0; i--){
    if(i == 0){
      printf("%d\n",hot_potato.trace[i]);
    }
    else{
      printf("%d,",hot_potato.trace[i]);
    }
    
  }
}


int main(int argc, char* argv[]){
  processInput(argc, argv);
  int port = atoi(argv[1]);
  int numPlyers = atoi(argv[2]);
  int numHops = atoi(argv[3]);
  printf("Potato Ringmaster\nPlayers = %d\nHops = %d\n", numPlyers, hot_potato.hops);
//  printf("empty array, size of struct:%ld\n", sizeof(hot_potato));
  for(int i = 0;i<numHops;i++){
    hot_potato.trace[i] = 0;
  }
//  printf("full array, size of struct:%ld\n", sizeof(hot_potato));
  //connect to all players
  int status;
  int master_fd; 
  int hot[numPlyers];
  int player_fd[numPlyers];
  struct sockaddr_in playerAddress[numPlyers];//store all players' address
  struct addrinfo master_info;
  struct addrinfo *master_info_list;
  const char *mastername = NULL;
  const char *portNumber = argv[1];
  memset(&master_info, 0, sizeof(master_info));

  master_info.ai_family   = AF_UNSPEC;
  master_info.ai_socktype = SOCK_STREAM;
  master_info.ai_flags    = AI_PASSIVE;
  
    status = getaddrinfo(mastername, portNumber, &master_info, &master_info_list);
  if (status != 0) {
	fprintf(stderr, "Error: cannot get address info for host\n");
	exit(EXIT_FAILURE);
  }
  
  master_fd = socket(master_info_list->ai_family, 
		     master_info_list->ai_socktype, 
		     master_info_list->ai_protocol);
  if(master_fd == 0){
	  fprintf(stderr, "Error: Fail to create master socket\n");
	  exit(EXIT_FAILURE);
  }
  
  int yes = 1;
  status = setsockopt(master_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status = bind(master_fd, master_info_list->ai_addr, master_info_list->ai_addrlen);
  if(status == -1){
	  fprintf(stderr, "Error: cannot bind socket\n");
	  exit(EXIT_FAILURE);
  }
  //set max # of pending connections on master socket
  status = listen(master_fd, numPlyers);
  if(status == -1){
	  fprintf(stderr, "Error: cannot listen on socket\n");
	  exit(EXIT_FAILURE);
  }

  struct sockaddr_storage sockAddress;
  socklen_t addrLen = sizeof(sockAddress);
  int maxfd = 0;
  //accept player sockets one by one
	fd_set readfds;
	FD_ZERO(&readfds);
	for(int i= 0; i<numPlyers; i++){
		player_fd[i] = accept(master_fd, (struct sockaddr*)&sockAddress, &addrLen);
		if(player_fd[i] == -1){
			perror("cannot accept");
			//fprintf(stderr, "Error: cannot accept connection on socket\n");
			exit(EXIT_FAILURE);
		}
		//add player_fd to read list
		FD_SET(player_fd[i], &readfds);
		if(player_fd[i]>maxfd){
			maxfd = player_fd[i];
		}
		send(player_fd[i], &i, sizeof(int), 0);
		send(player_fd[i], &numPlyers, sizeof(int), 0);
		printf("Player %d is ready to play\n", i);
	//reveive confirm msg from each player as the sign of "player is ready"
		char buffer[6];
		memset(buffer, '\0', 6);
		recv(player_fd[i], buffer, 6, 0);
		buffer[5] = '\0';
		if(strcmp(buffer, "roger")==0){
			printf("Player %d is ready to play\n", i);
		}
		else{
			fprintf(stderr, "Error: Player %d  fails to connect\n", i);
			exit(EXIT_FAILURE);
		}
   
     //recv everyone's ip+port
	  recv(player_fd[i], &(playerAddress[i]), sizeof(playerAddress[i]), 0);
	}
 //delegate ip+port
 	for(int i= 0; i<numPlyers; i++){
      if(i == 0){//send [N-1] to player 0 through fd[0]
        send(player_fd[i], &(playerAddress[numPlyers-1]), sizeof(playerAddress[numPlyers-1]), 0);
      }
      else{
        //send curr's left neighbor
        send(player_fd[i], &(playerAddress[i-1]), sizeof(playerAddress[i-1]), 0);
      }
   }
   //corner case: hop = 0;
   if(hot_potato.hops == 0){
	 freeaddrinfo(master_info_list);
     for(int i = 0;i<numPlyers;i++){
	     close(player_fd[i]);
     }
     return EXIT_SUCCESS;
   }
   //hop > 0
   srand((unsigned int)time(NULL) + 0);
   int in_game = 1;
   int randPlayer = rand()%numPlyers;
   send(player_fd[randPlayer], &(hot_potato), sizeof(hot_potato), 0);
   printf("Ready to start the game, sending potato to player %d\n", randPlayer);
   memset(&hot_potato, 0, sizeof(hot_potato));
   while(in_game){
    int active = select(maxfd+1, &readfds, NULL, NULL, NULL);
	  if(active<0){
		  fprintf(stderr,"Error: select\n");
		  exit(EXIT_FAILURE);
	  }
     for(int i = 0;i<numPlyers;i++){
	     if(FD_ISSET(player_fd[i], &readfds)){
         memset(&hot_potato, 0, sizeof(hot_potato));
         //it
		     recv(player_fd[i], &hot_potato, sizeof(hot_potato), MSG_WAITALL);
		     printTrace(&hot_potato,numHops);
		     //printf("last play is %d\n", hot_potato.id);
         in_game = 0;
         break;
	     }
     }
   }
   
   freeaddrinfo(master_info_list);
   //close every player
   for(int i = 0;i<numPlyers;i++){
	   close(player_fd[i]);
   }
  return EXIT_SUCCESS;
}
