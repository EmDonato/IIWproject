
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/shm.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <signal.h>
#include <stdbool.h>
#include "packet.h"
#define SERV_PORT   5193 
#define SIZE  128 
#define T  1

packet pacReserve;
int socketReserve;
struct sockaddr_in addrReserve;

void retrasmition_cmd(int sig){

	
	
 	if (sendto(socketReserve, (const void *)&pacReserve, sizeof(packet), 0, (struct sockaddr *)&addrReserve, sizeof(struct sockaddr)) < 0) {
		perror("errore in sendto 1");
		exit(1);
	}  
	printf("il pacchetto da rimandare:\n %d \n %d \n %d \n %d\n %s ",pacReserve.seqnumb,pacReserve.acknumb,pacReserve.flags.ack,pacReserve.flags.syn,pacReserve.data);
	fflush(stdout);
	printf("\n\n**********************************nella gestione del segnale*******************************\n\n"); 
	fflush(stdout);
	alarm(T);  //imposta timer
	
	
}















int strswitch(char *str){
	int i;
	if(strcmp(str,"list") == 0){
		i = 0;
	}
	else if(strcmp(str,"delete") == 0){
		i = 1;
	}	
	else if(strcmp(str,"put") == 0){
		i = 2;
	}
	else if(strcmp(str,"get") == 0){
		i = 3;
	}	
	return i;
}


void createFirstPac(packet *pac,int32_t sn,int32_t an,char *data1){
	
	
		memset((void*)pac,0,sizeof(packet));
	
		pac->seqnumb = sn;
		pac->acknumb = an;
		strcpy(pac->data,data1);
	

	return;
	
	
	
}



int sendRequest(int socketID,struct sockaddr_in *add,packet *pac){
	
	int len =  sizeof(struct sockaddr);
	int32_t ackatteso = (pac->seqnumb + 1 + strlen(pac->data));
	int32_t sqn = pac->seqnumb;
	int32_t acn = pac->seqnumb;
	
	
	
struct sigaction act;


	// gestione timer
	sigset_t set ;
	sigfillset(&set);
	memset((void *)&act,0,sizeof(struct sigaction));
	if((sigprocmask(SIG_BLOCK,&set,NULL))==-1){
		printf("errore nel settare i segnali");
		exit(1);
	}
	act.sa_mask = set;
	act.sa_handler = retrasmition_cmd;
	if((sigaction(SIGALRM,&act,NULL))==-1){
		printf("errore nel settare i segnali");
		exit(1);
	}
	if(sigemptyset(&set)) exit(1);
	if(sigaddset(&set,SIGALRM)) exit(1);
	if(sigaddset(&set, SIGINT)) exit(1);
	if((sigprocmask(SIG_UNBLOCK,&set,NULL))==-1){
		printf("errore nel settare i segnali\n");
		exit(1);
	}
	
	
	memcpy((void *)&pacReserve,(void *)pac,sizeof(packet));
	socketReserve = socketID;
	memcpy((void *)&addrReserve,(void *)add,sizeof(struct sockaddr_in));
	
	if (sendto(socketID, (const void *)pac, sizeof(packet), 0, (struct sockaddr *)add, sizeof(struct sockaddr)) < 0) {
		perror("errore in sendto 1");
		exit(1);
	}	
	alarm(T);  //imposta timer
	while(1){
	
		//printf("in loop \n");
		memset((void *)pac,0,sizeof(packet));

		if((recvfrom(socketID,(void *)pac, sizeof(packet), 0 ,(struct sockaddr *)add,&len )) < 0) {
			perror("errore in recvfrom");
			exit(1);
		}	
		
		printf("il pacchetto arrivato:\n %d \n %d \n %d \n %d\n %s ",pac->seqnumb,pac->acknumb,pac->flags.ack,pac->flags.syn,pac->data);
		printf("ackatteso: %dpac->ack:%d\n\n",ackatteso,pac->acknumb);
		if(ackatteso == pac->acknumb && pac->flags.ack == 1){
			printf("\nsuperato i controlli\n");
			alarm(0); //azzero timer
			if(strcmp( pac->data,"OK")==0)
				return 0;
			else
				return 1;
		}
		
	}
	
	

	
	return 1;
	
	
	
	
	
	
}

