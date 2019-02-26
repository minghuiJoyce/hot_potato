#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <time.h>
#include <arpa/inet.h>
#include "potato.h"

int main(int argc, char* argv[]){
	if(argc != 3){
		printf("Usage: player <machine_name> <port_num>\n");
		return EXIT_FAILURE;
	}
	int portNum = atoi(argv[2]);
	char* machine_name = argv[1];//the machine that ringmaster process is running
  
	int playerClient, status;
  struct addrinfo playerInfo;
  struct addrinfo *playerInfoList;
  const char *mastername = argv[1];
  const char *portnum = argv[2];
	//struct sockaddr_in masterAddr;
	int mysockets[3];
	//connect with ringmaster
  memset(&playerInfo, 0, sizeof(playerInfo));
  playerInfo.ai_family   = AF_UNSPEC;
  playerInfo.ai_socktype = SOCK_STREAM;
  
  status = getaddrinfo(mastername, portnum, &playerInfo, &playerInfoList);
  if (status != 0) {
    fprintf(stderr, "Error: cannot get address info for host\n");
		exit(EXIT_FAILURE);
  }
  
	//playerClient = socket(AF_INET, SOCK_STREAM, 0);
  playerClient = socket(playerInfoList->ai_family, 
		     playerInfoList->ai_socktype, 
		     playerInfoList->ai_protocol);
              
	if(playerClient == -1){
		fprintf(stderr, "Error: Fail to create client socket\n");
		exit(EXIT_FAILURE);
	}
	/*memset(&masterAddr, 0, sizeof(masterAddr));
	masterAddr.sin_family = AF_INET;
  masterAddr.sin_addr.s_addr = inet_addr(machine_name);
	masterAddr.sin_port = htons(portNum);*/
	
	//status = connect(playerClient, (struct sockaddr*)&masterAddr, sizeof(masterAddr));
		status = connect(playerClient, playerInfoList->ai_addr, playerInfoList->ai_addrlen);
  
  if(status == -1){
		fprintf(stderr, "Error: Fail to connect to ringmaster\n");
		exit(EXIT_FAILURE);
	}
 
	//recv playNum and send ip+port to ringmaster
	int number;
	recv(playerClient, &number, sizeof(int), 0);
	int totalNum;
	recv(playerClient, &totalNum, sizeof(int), 0);
 	printf("Connected as player %d out of %d total players\n", number, totalNum);
  //confirm msg to ensure two way communication between master and player
	const char* confirm = "roger";
	send(playerClient, confirm, strlen(confirm), 0);
	//pack ip+port
	char hostname[128];//my ip address
	gethostname(hostname, sizeof(hostname));
 // printf("%s\n", hostname);
	struct hostent * he = gethostbyname(hostname);
	struct sockaddr_in myaddress;
	memset(&myaddress, 0, sizeof(myaddress));
	myaddress.sin_family = AF_INET;
	myaddress.sin_addr = *((struct in_addr *)he->h_addr);
	//search for an available port by binding
	int rightHost_fd;
	rightHost_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(rightHost_fd == 0){
		fprintf(stderr, "Error: Fail to create rightHost socket\n");
		exit(EXIT_FAILURE);
	}
	int yes = 1;
	status = setsockopt(rightHost_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
 
	struct sockaddr_in rightAddr;
  memset(&rightAddr, 0, sizeof(rightAddr));
  rightAddr.sin_family = AF_INET;   
  rightAddr.sin_addr.s_addr = INADDR_ANY;
 
	//50000+
	int availablePort = 50000;
	while(availablePort<=62204){
		rightAddr.sin_port = htons(availablePort);
		status = bind(rightHost_fd, (struct sockaddr*)&rightAddr, sizeof(rightAddr));
		if(status == 0){
			break;
		}
		if(availablePort == 62204){
			fprintf(stderr, "Error: Fail to find available port\n");
			exit(EXIT_FAILURE);
		}
		availablePort++;
	}
 	//listen on right neighbor
	status = listen(rightHost_fd, 10);//should I use numPlayer?
	 if(status == -1){
	  fprintf(stderr, "Error: cannot listen on right neighbor\n");
	  exit(EXIT_FAILURE);
	}
  
  myaddress.sin_port =  htons(availablePort);
  //printf("player %d's ip : %s , port : %d  \n" , number, inet_ntoa(myaddress.sin_addr),ntohs(myaddress.sin_port));
	//send my IP & port
	send(playerClient, &myaddress, sizeof(myaddress), 0);
	
 
 
	//receive my left neighbor IP & port
	struct sockaddr_in leftAddr;
	status = recv(playerClient, &leftAddr, sizeof(leftAddr), 0);
	//error checking leftAddr ...
	if(status == -1){
    printf("Error: left recv wrong from ringmaster\n");
    exit(EXIT_FAILURE);
  }
	
  //printf("[recv] my left's ip : %s , port : %d  \n" , inet_ntoa(leftAddr.sin_addr),ntohs(leftAddr.sin_port));
 
	//connect to left neighbor
	int leftClient_fd;
	leftClient_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(leftClient_fd == 0){
		fprintf(stderr, "Error: Fail to create leftClient socket\n");
		exit(EXIT_FAILURE);
	}
	status = connect(leftClient_fd, (struct sockaddr*)&leftAddr, sizeof(leftAddr));
	if(status == -1){
		//fprintf(stderr, "Error: Fail to connect to left neighbor\n");
   perror("connection");
		exit(EXIT_FAILURE);
	}
	
 	int rightaddrLen = sizeof(rightAddr);
	int rightClient_fd;
	rightClient_fd = accept(rightHost_fd, (struct sockaddr*)&rightAddr,(socklen_t*)&rightaddrLen);
	
	potato hotPotato;
	mysockets[0] = leftClient_fd;
	mysockets[1] = rightClient_fd;
	mysockets[2] = playerClient;
	int highfd = mysockets[0];

	
	srand((unsigned int)time(NULL) + 0);
	int in_game = 1;
  int randPlyNum = 0;
  int readbyte = 0;
	while(in_game){
 		//add to read list
	fd_set readfds;
	FD_ZERO(&readfds);
	for(int i = 0; i < 3; i++){
		FD_SET(mysockets[i], &readfds);
		if(mysockets[i]>highfd){
			highfd = mysockets[i];
		}
	}
 	int active = select(highfd+1, &readfds, NULL, NULL, NULL);
	if(active<0){
		fprintf(stderr,"Error: select\n");
		exit(EXIT_FAILURE);
	}
 
	for(int i = 0; i<3;i++){
		if(FD_ISSET(mysockets[i],&readfds)){
			//receive the potato from master to the chosen player
      memset(&hotPotato, 0, sizeof(hotPotato));
			readbyte = recv(mysockets[i], &hotPotato, sizeof(hotPotato), MSG_WAITALL);
      if(readbyte == 0){//master closed
          in_game = 0;
          break;
      }
     //printf("player %d recvs the potato\n", number);
			hotPotato.hops--;
			if(hotPotato.hops>0){
				//addtoTrace(&hotPotato,number);
				hotPotato.id = number;
        hotPotato.trace[hotPotato.hops] = number;
				int randDirection = rand()%2;
        //calculate player id
        randPlyNum = (randDirection == 0)? number -1:number + 1;
        if(randPlyNum<0){
            randPlyNum = totalNum-1;
        }
        if(randPlyNum == totalNum){
            randPlyNum = 0;
        }
				send(mysockets[randDirection], &(hotPotato), sizeof(hotPotato), 0);
				printf("Sending potato to %d\n", randPlyNum);
        //break;
			}
			if(hotPotato.hops == 0){
        hotPotato.id = number;
        hotPotato.trace[hotPotato.hops] = number;
				printf("I'm it\n");
				send(playerClient, &(hotPotato), sizeof(hotPotato), 0);
				break;
			}
		}
	}
 
	}

/*	char shutDown[512];
	recv(playerClient, shutDown, 512, 0);
	shutDown[511] = 0;
	if(strcmp(shutDown, "close")==0){*/
 freeaddrinfo(playerInfoList);
		close(leftClient_fd);
		close(rightHost_fd);
		close(playerClient);
//	}
	return EXIT_SUCCESS;
}