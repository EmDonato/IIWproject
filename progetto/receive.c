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
#include "packet.h"

int makePac(packet *pac,int seqn,int ackn,int sizedata){
	
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
	
	if(sizedata == 0){
		pac->seqnumb = seqn;
		pac->acknumb = ackn;
		pac->flags.ack = 1;
		pac->last = true;
	}
	else{
		pac->seqnumb = seqn;
		pac->acknumb = ackn + sizedata + 1;
		pac->flags.ack = 1;
		pac->last = true;
	}
	return 0;
}


int rcv(int socketID, struct sockaddr_in *add,int fileID,int32_t ackn,int32_t seqn){
	int32_t actualack = ackn;
	int32_t actualseqn = seqn;
	
	int len = sizeof(struct sockaddr_in);
	packet pacchetto,pacToSend;
	int32_t expectedack = ackn;
	makePac(&pacToSend,actualseqn,actualack,0);
	while(1){
		if((recvfrom(socketID,(void *)&pacchetto, sizeof(packet), 0 ,(struct sockaddr *) add,&len )) < 0) {
			perror("errore in recvfrom");
			exit(1);
		}	
		printf("il pacchetto arrivato contiene:\n %d \n %d \n %d \n %d\n %d\n\n\n %s\n ",pacchetto.seqnumb,pacchetto.acknumb,pacchetto.flags.ack,pacchetto.flags.syn,pacchetto.last,pacchetto.data);
		printf("\n\nexpectedack %d,pacchetto.seqnumb %d\n\n",expectedack,pacchetto.seqnumb);
		
		if(pacchetto.flags.ack == 0 && expectedack == pacchetto.seqnumb){
			printf("superato il controllo del ack\n\n");
			// e andato a buon fine 
			if((write(fileID, (const void *)pacchetto.data, pacchetto.data_size)) == -1){
				perror("errore in recvfrom");
				exit(1);
			}
			printf("superato il controllo del write\n\n");
			// aggiornare gli ack e seq numb
			makePac(&pacToSend,actualseqn,actualack,pacchetto.data_size);
			actualack = pacToSend.acknumb;
			expectedack = actualack;
			printf("creato il nuovo pacchetto\n\n");
			if(pacchetto.last == true){
				printf("dentro il last\n\n");
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
			printf("nell invio ack di nuovo\n\n");			
		}
		memset((void *)&pacchetto,0,sizeof(packet));
	}
	return 0;
}
