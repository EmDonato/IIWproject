#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <pthread.h>
#include <stdbool.h>
#include <signal.h>
#include <semaphore.h>

#include<fcntl.h>
#include<sys/sem.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<pthread.h>

#include "client.h"

char *filename ="picture.jpeg";

void main()
{
	int fd = open(filename,O_RDWR|0666);
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	int32_t ackn = 0;
	int32_t seqn = 0;
	struct sockaddr_in servaddr;

	memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    TCPfunction(fd, seqn, ackn, sockfd, servaddr);
}	
