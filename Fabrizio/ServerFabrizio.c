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
#include <fcntl.h>


// ******************************************************manda un  pacchetto per debug**********************************

key_t semkey = 12345;
int sockfd, connsd;
int IDsem;
struct sockaddr_in accepted[BACKLOG];
struct sockaddr_in syn_rcvd[N];
pthread_t tidglb;
bool ausiliarAccepted[BACKLOG] = { false };
bool ausiliarSyn_rcvd[N] = { false };
int semID;




int main(int argc, char **argv){
	
	int i;
	int descriptor;
	
	char *token;
	pid_t pid;
	pthread_t tid;
	void *value;
	struct sockaddr_in addr, client;;
	int n ,len=sizeof(struct sockaddr_in);
	socklen_t clientlen;
	key_t keysem = 32145;
	char cmd_rcvd[10] ;  // vettore per memorizzare comando ricevuto che prendo dal campo data
	int lenLissen = sizeof(struct sockaddr_in);
	sigset_t set;
	packet pacchetto , pacrcv;
	char *strstr = "HELLO WORLD\n";
	if ((semID = semget(semkey, 1, IPC_CREAT|0666)) == -1)
		goto exit_process_1;
	
	if (semctl(semID, 0, SETVAL, 0) == -1)
		goto exit_process_1;
	
	
	/*
		Gestione del segnale SIGINT

	*/

	
	sigfillset(&set);
	struct sigaction act;
	act.sa_mask = set;
	act.sa_handler = shutout;
	sigaction(SIGINT,&act,NULL);

	int reu =1;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		perror("errore in socket");
		exit(1); 
	}
	
	
	memset((void *)&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY); /* il server accetta pacchetti su una qualunque delle sue interfacce di rete */
	addr.sin_port = htons(SERV_PORT); /* numero di porta del server */
	
	
	
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reu, sizeof(int)) < 0)
		 perror("setsockopt(SO_REUSEADDR) failed");
	
	  /* assegna l'indirizzo al socket */
	if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("errore in bind");
		exit(1);
	}
	printf("dentro il main\n");
	if (pthread_create(&tid,NULL,mylisten,NULL) != 0 ) {
		perror("errore in mylisten");
		exit(1);
	}

	memset((void *)&pacrcv , 0, sizeof(pacchetto));
	memset(cmd_rcvd , 0 , sizeof(cmd_rcvd));

	const char delim[2] = "_";
	
	packet p ;
	// fare memset di pacchetto p
	memset((void *)&p , 0 , sizeof(p));

	while(1){
		connsd = myaccept(sockfd,&client,&clientlen);
	
		n = recvfrom(sockfd ,(void *)&pacrcv , sizeof(packet) ,0, (struct sockaddr *)&addr , &len);

		if ( n < 0){
			perror("Errore in ricezione del pacchetto.");
			exit(0);

		}

		if ( pacrcv.flags.ack == 0 && pacrcv.flags.syn && pacrcv.flags.fin == 0 &&  pacrcv.acknumb == pacchetto.seqnumb && pacrcv.seqnumb == pacchetto.acknumb){ //***************stai facendo controlli sbagliati devi vedere se acknumb giusto e seqnumb giusto, ack = 0,controlla ache i flag magari************

			//memcpy((void *)cmd_rcvd , (void *)pacrcv.data , sizeof(pacrcv.data);

			token = strtok(pacrcv.data , delim);

			const char newdelim[2] = '\0';

			while ( token != NULL){
				token = strtok( NULL , newdelim);

			}

			memset((void *)&p , 0 , sizeof(p));
			p.flags.ack = 1;
			p.flags.fin = 0;
			p.flags.syn = 0;

			p.acknumb = pacrcv.seqnumb + sizeof(pacrcv.data);
			p.seqnumb = pacrcv.acknumb;

			// fare memset di pacchetto p
			// pacchetto.flags.ack = 1 
			// altri flags restano a 0 
			// acknumb = pacrcv.seqnumb + sizeof(tutta la stringa tokenizzata)
			// seqnumb = pacrcv.acknumb 
			if ( strcmp(&token[0] , "get")){

				if ( sendto(sockfd , (void *)&p , sizeof(packet),0, (struct sockaddr *)&addr , len )< 0 )
					 {
						perror("Errore in invio");
						exit(1);

						
				
					
				}
					printf("Ricevuto il comando get");

			}
				
			if ( strcmp(&token[0] , "ls")){

				if (sendto(sockfd , (void *)&p , sizeof(packet) , 0 , (struct sockaddr *)&addr , len)<0){
					perror("Errore in invio ");
				}
				printf("Ricevuto il comando ls ");
			}
				
			if ( strcmp( &token[0], "put")){

				if (sendto(sockfd , (void *)&p , sizeof(packet), 0 , (struct sockaddr*)&addr , len)< 0){
					perror("Errore in invio");

				}
				printf("Ricevuto comando put");

				descriptor = creat(&token[1], 0666 );
			}

			if ( strcmp( &token[0] , "delete")){
				if ( sendto(sockfd , (void *)&p , sizeof(packet), 0 , (struct sockaddr *)&addr , len)<0){
					perror("Errore in invio");

				}
				printf("Ricevuto comando delete");

			}

		}

		
		close(connsd); 

		
	}
	
	pthread_join(tid, &value);
	exit_process_1:
		semctl(semID, -1, IPC_RMID, 0);

	exit_process_4:
		exit(1);
}

