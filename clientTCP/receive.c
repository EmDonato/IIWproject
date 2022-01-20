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
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include "packet.h"
int makePac(packet *pac,int seqn,int ackn,int sizedata){
	
	//funzione che crea un pacchetto
	
	if(sizedata == 0){
		pac->seqnumb = seqn;
		pac->acknumb = ackn;
		pac->flags.ack = 1;
		pac->last = false;
	}
	else{
		pac->seqnumb = seqn;
		pac->acknumb = ackn + sizedata + 1;
		pac->flags.ack = 1;
		pac->last = false;
	}
	return 0;
}

int makeLastPac(packet *pac,int seqn,int ackn,int sizedata){
	
	//funzione che crea l ultimo pacchetto pacchetto
	
	if(sizedata == 0){
		pac->seqnumb = seqn;
		pac->acknumb = ackn;
		pac->flags.ack = 1;
		pac->last = true;
	}
	else{
		pac->seqnumb = seqn;
		pac->acknumb = ackn ;
		pac->flags.ack = 1;
		pac->last = true;
	}
	return 0;
}


int rcv_prob(int socketID, struct sockaddr_in *add,int fileID,int32_t ackn,int32_t seqn,float prob){
	
	// funzione che si occupa della ricezione sicura del file con probabilita di perdita prob, scrive sul file solamente se ack arrivato = ack atteso
	
	
	int32_t actualack = ackn;
	int32_t actualseqn = seqn;
	int numb;
	double probaus;
	srand (time(NULL));	
	int len = sizeof(struct sockaddr_in);
	packet pacchetto,pacToSend;
	int32_t expectedack = ackn;
	makePac(&pacToSend,actualseqn,actualack,0);
	while(1){
		numb = rand();
		probaus = (double)numb /(double)RAND_MAX;
		if((recvfrom(socketID,(void *)&pacchetto, sizeof(packet), 0 ,(struct sockaddr *) add,&len )) < 0) {
			perror("errore in recvfrom");
			exit(1);
		}	

		if(probaus<=prob){

		}
		else{


			if(pacchetto.flags.ack == 0 && expectedack == pacchetto.seqnumb){ 
			
				if((write(fileID, (const void *)pacchetto.data, pacchetto.data_size)) == -1){
					perror("errore in recvfrom");
					exit(1);
				}

				makePac(&pacToSend,actualseqn,actualack,pacchetto.data_size);
				actualack = pacToSend.acknumb;
				expectedack = actualack;
				if(pacchetto.last == true){					
					makeLastPac(&pacToSend,actualseqn,actualack,pacchetto.data_size);
					if (sendto(socketID, (const void *)&pacToSend, sizeof(packet), 0, (struct sockaddr *)add, sizeof(struct sockaddr_in)) < 0) {
						perror("errore in sendto 1");
						exit(1);
					}		
					close(fileID);
					return 0;
						
				}
				if (sendto(socketID, (const void *)&pacToSend, sizeof(packet), 0, (struct sockaddr *)add, sizeof(struct sockaddr_in)) < 0) {
					perror("errore in sendto 1");
					exit(1);
				}			
			}
			else if(pacchetto.flags.ack == 0 && expectedack != pacchetto.seqnumb){
				//rimada ultimo ack
				if (sendto(socketID, (const void *)&pacToSend, sizeof(packet), 0, (struct sockaddr *)add, sizeof(struct sockaddr_in)) < 0) {
					perror("errore in sendto 1");
					exit(1);
				}
			}
			memset((void *)&pacchetto,0,sizeof(packet));
		}
	}
	return 0;
}
