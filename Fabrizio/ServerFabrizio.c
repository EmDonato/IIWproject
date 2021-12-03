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
	FILE *fptr;
	char command_rcvd[50];

	int file_descriptor;


	pid_t pid;
	pthread_t tid;
	void *value;
	struct sockaddr_in addr, client;;
	int n ,len=sizeof(struct sockaddr_in);
	socklen_t clientlen;
	key_t keysem = 32145;
	
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

		if ( pacrcv.flags.ack == 0 && pacrcv.flags.syn && pacrcv.flags.fin == 0 ){ //***************stai facendo controlli sbagliati devi vedere se acknumb giusto e seqnumb giusto, ack = 0,controlla ache i flag magari************

			

			strcpy(command_rcvd , pacrcv.data);

			char *cmd_name = strtok(command_rcvd , " ");
			char *filename = strtok(command_rcvd , " ");





			memset((void *)&p , 0 , sizeof(p));
			p.flags.ack = 1;
			p.flags.fin = 0;
			p.flags.syn = 0;

			p.acknumb = pacrcv.seqnumb + sizeof(pacrcv.data);  // oppure usiamo p.acknumb = pacrcv.seqnumb + 1 ?
			p.seqnumb = pacrcv.acknumb;

			// fare memset di pacchetto p
			// pacchetto.flags.ack = 1 
			// altri flags restano a 0 
			// acknumb = pacrcv.seqnumb + sizeof(tutta la stringa tokenizzata)
			// seqnumb = pacrcv.acknumb 
			if ( strcmp(cmd_name , "get")){

				if ( sendto(sockfd , (void *)&p , sizeof(packet),0, (struct sockaddr *)&addr , len )< 0 )
					 {
						perror("Errore in invio");
						exit(1);

						
				
					
				}

				if ( access(filename, F_OK)==0){    // controllo se il file esiste passando file name ed opzione F_OK per la ricerca


					fptr = fopen(filename , "rb");     // apre il file e ritorna il puntatore che assegnamo al descrittore 

					// possibile suddivisione del file in blocchi di bytes per successivo invio
					


					// se il file esiste allora invio inizializzando il pacchetto 
					if (sendto(sockfd , (void *)&p , sizeof(packet), 0 , (struct sockaddr *)&addr , len) < 0){
						perror("Errore in invio");
						exit(1);

					}
					// controllo la ricezione di ack e seqnumb di p , altrimenti devo rinviare
					int n = recvfrom(sockfd , (void *)&pacrcv , sizeof(packet), 0 , (struct sockaddr *)&addr , &len );
					if ( n < 0){
						perror("Errore in ricezione.");
						exit(1);

					}
					if (p.acknumb == pacrcv.seqnumb + 1 && p.seqnumb == pacrcv.acknumb){

						printf("Pacchetto inviato correttamente");

					}
					// altrimenti bisogna rinviare il file


					fclose(fptr); //chiudo il file su cui abbiamo lavorato;


				}

				else {

					char *stringerror = "File non esiste\0";
					strcpy(p.data , stringerror)    // copio la stringa di errore nel campo dati del pacchetto

					sendto(sockfd , (void *)&p , sizeof(packet), 0 , (struct sockaddr *)&addr , len); // invio il pacchetto al client con errore di ricerca del file



				}
				// se non esiste il file errore nel campo data del pacchetto da rimandare

					printf("Ricevuto il comando get");

			}
				
			if ( strcmp(cmd_name , "ls")){

				char file_entry[200];
				memset(file_entry , 0 , sizeof(file_entry));

				fptr = fopen("file.txt", "wb");     // creo un file con permessi di scrittura

				if (ls(fptr) == -1){
					perror("ls");
					exit(-1);

				}
				fclose(fptr);

				fptr = fopen("file.txt " , "rb");

				int filesize = fread(file_entry , 1 , 200 , fptr);

				memcpy((void *)&p.data , &fptr ,filesize);



				if (sendto(sockfd , (void *)&p , sizeof(packet) , 0 , (struct sockaddr *)&addr , len)<0){
					perror("Errore in invio ");
				}

				rcvfrom(sockfd , (void *)&pacrcv , sizeof(packet , 0 , (struct sockaddr *)&addr , len));

				
				if (p.acknumb == pacrcv.seqnumb + 1 && p.seqnumb == pacrcv.acknumb){
					printf("Pacchetto inviato correttamente");

				}

				printf("Ricevuto il comando ls ");
			}
				
			if ( strcmp( cmd_name, "put")){

				if (sendto(sockfd , (void *)&p , sizeof(packet), 0 , (struct sockaddr*)&addr , len)< 0){
					perror("Errore in invio");

				}

				rcvfrom(sockfd , (void *)&pacrcv , sizeof(packet), 0 , (struct sockaddr*)&addr , len);
				if (p.acknumb == pacrcv.seqnumb + 1 && p.seqnumb == pacrcv.acknumb){
					printf("Ricevuto comando put");
				
				// controllo prima se il file esiste altrimenti lo creo 
					file_descriptor = creat(&token[1], 0666 );
				}
				else{
					// riprovare
				}

			if ( strcmp( cmd_name , "delete")){
				if ( sendto(sockfd , (void *)&p , sizeof(packet), 0 , (struct sockaddr *)&addr , len)<0){
					perror("Errore in invio");

				}

				rcvfrom(sockfd, (void *)&pacrcv , sizeof(packet), 0 , (struct sockaddr *)&addr , len);
				if(p.acknumb == pacrcv.seqnumb + 1 && p.seqnumb == pacrcv.acknumb){

					if (access(filename  , F_OK ) == -1){ 			// controllo se il file esiste 
						

					} 
					else {
						if (access(filename , R_OK )==-1){    // controllo se il file ha i permessi giusti

						}
						else{
							printf("Nome file è %s \n", filename);
							remove(filename);
							
						}
					}
				}
				// funzione per cercare file , se presente eliminarlo e poi cancellare anche il nome dal .txt creato in ls
				// inviare poi il pacchetto con errore oppure cancellato
				
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

