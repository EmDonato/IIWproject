#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "packet.h"
#include <sys/shm.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <signal.h>
#include <stdbool.h>
#define M 1024
#define X 10

char buffer[X][M];

int keysem = 1243;

void *write_thread(void *arg){
	
	int byteread;
	int file_descriptor = open((char *)arg, O_READ);
	int i = 0;
	while((byteread = read(file_descriptor, buffer[i],M)) != 0 || i != X-1 ){
		if(byteread == 0){
			printf("meno del buffer\n\n");
			int y = 0;
			while(y == i){
				printf("%s\n", buffer[y]);
			}
			exit_thread(0);
		}
		i++;
	}
	printf("piu del buffer\n\n");
	int i=0;
	while( i== X-1){
		printf("%s\n", buffer[i]);
	}
	exit_thread(0);
	
}

int sendReliable(int sockID, char *file_name){
	int tid1,tid2,tid3;
	void *status;
	if (pthread_create(&tid1,NULL,mylisten,NULL) != 0 ) {
		perror("errore in mylisten");
		exit(1);
	}
	pthread_join(tid1,&status);
	exit_thread(0);
}


int main(){
	char *file_name="a.txt";
	sendReliable(654763, char *file_name);
	return 0;
	
}