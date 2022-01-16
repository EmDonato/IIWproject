#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include<semaphore.h>
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

#define SERV_PORT   5193 

// variabili globali

key_t semkey = 12345;
int sockfd, connsd;
int IDsem;
struct sockaddr_in accepted[BACKLOG];
struct sockaddr_in syn_rcvd[N];
struct gate gateAccepted[BACKLOG];
struct gate gateSyn_rcvd[N];
pthread_t tidglb;
int newPort = SERV_PORT + 1;
bool ausiliarAccepted[BACKLOG] = { false };
bool ausiliarSyn_rcvd[N] = { false };
int semID;
extern int32_t snprc;
extern int32_t anprc;
char cmd[2][64];
char * aus;





int main(int argc, char **argv){
	
/* 	sezione principale del programma del server.
	Il programma e organizzato da iun main thread che si occupa
	della gestine della connessione dei client
	sucecssivamente verranno chiamati programmi figli che gestiranno autonomamente 
	le richieste dei client 
	 */
	
	// dichiarazione variabili
	
	int i,file_descriptor;
	pid_t pid;
	pthread_t tid;
	void *value;
	FILE *fptr;
	struct sockaddr_in addr, client;
	int len=sizeof(struct sockaddr_in), ausiliar_delete=0;
	socklen_t clientlen;
	key_t keysem = 32145;
	int lenLissen = sizeof(struct sockaddr_in);
	sigset_t set;
	packet pacchetto;
	char *comando_aus, last_file[64];
	long int lendata;
	int32_t sn;
	int32_t an;
	int32_t *arrayNumb = (int32_t *)malloc(sizeof(int32_t)*2);
	float prob = PROB;
	
	// getstione semaforo
	if ((semID = semget(semkey, 1, IPC_CREAT|0666)) == -1)
		goto exit_process_1;
	
	if (semctl(semID, 0, SETVAL, 0) == -1)
		goto exit_process_1;
	
	//gestione del segnale SIGINT
	sigfillset(&set);
	struct sigaction act;
	act.sa_mask = set;
	act.sa_handler = shutout;
	sigaction(SIGINT,&act,NULL);

	int reu =1;
	
	// creazione del socket
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		perror("errore in socket");
		exit(1); 
	}
	
	
	memset((void *)&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY); /* il server accetta pacchetti su una qualunque delle sue interfacce di rete */
	addr.sin_port = htons(SERV_PORT); /* numero di porta del server */
	
	  /* assegna l'indirizzo al socket */
	if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("errore in bind");
		exit(1);
	}
	printf("\n\n******************************* SERVER ATTIVO ******************************\n\n");
	// nel thread per handshaking
	if (pthread_create(&tid,NULL,mylisten,(void *)arrayNumb) != 0 ) {
		perror("errore in mylisten");
		exit(1);
	}
	
	
	while(1){

		int connsd = myaccept(sockfd,&addr,&len); // funzione che accetta le connessioni 

		if ( (pid = fork()) == 0) {
			// dentro il processo figlio che si occupa della gestione del client associato
			close(sockfd);
			printf("NUOVO CLIENT\n\n");
			getsockname(connsd, (struct sockaddr *)&addr,&len);
			printf("porta: %d\nIP: %d\n ",addr.sin_port, client.sin_addr.s_addr);
			memset((void *)&pacchetto,0,sizeof(packet));
			while(1){

				if((recvfrom(connsd,(void *)&pacchetto, sizeof(packet), 0 ,(struct sockaddr *) &client,&len )) < 0) {
					perror("errore in recvfrom");
					exit(1);
				}			

				if( pacchetto.flags.ack == 0  ){
					if(pacchetto.last == 1 ){
						int32_t ausseq;
						ausseq = pacchetto.seqnumb+pacchetto.data_size + 1;
						pacchetto.seqnumb = pacchetto.acknumb;
						pacchetto.acknumb = ausseq;
						pacchetto.flags.ack = 1;
						pacchetto.data_size = 0;
						if (sendto(connsd, (const void *)&pacchetto, sizeof(packet), 0, (struct sockaddr *)&client, sizeof(struct sockaddr)) < 0) {
							perror("errore in sendto 1");
							exit(1);
						}
					}

					lendata = strlen(pacchetto.data);
					sn = pacchetto.acknumb;
					an = pacchetto.seqnumb + lendata + 1;
					aus = strtok (pacchetto.data," ");
					i = 0;
					while (aus != NULL){
						strcpy(cmd[i],aus);
						i++;
						aus = strtok (NULL, " ");	
					}

					memset((void *)&pacchetto,0,sizeof(pacchetto));
					switch(strswitch(cmd[0])) {
						case 0:

							//nella sezione per la gestione del comando list
							
							makePacket(&pacchetto,sn,an,0);
							if (sendto(connsd, (const void *)&pacchetto, sizeof(packet), 0, (struct sockaddr *)&client, sizeof(struct sockaddr)) < 0) {
								perror("errore in sendto 1");
								exit(1);
							}
							
							fptr = fopen("list_temp.txt", "w+");	
							
							if (ls(fptr) == -1)		
								perror("ls");
							fclose(fptr);
							file_descriptor = open("list_temp.txt",O_RDWR,0666);
							sendFile(file_descriptor,sn ,an ,connsd, (struct sockaddr_in)client,PROB);							
							remove("list_temp.txt");
							
						break;
						
						case 1:
						
							//nella sezione per la gestione del comando delete 
							if(access(cmd[1],F_OK) == 0){
								// file presente, nella sezione di rimozione 
								makePacket(&pacchetto,sn,an,0);
								remove(cmd[1]);
							
								if (sendto(connsd, (const void *)&pacchetto, sizeof(packet), 0, (struct sockaddr *)&client, sizeof(struct sockaddr)) < 0) {
									perror("errore in sendto 1");
									exit(1);
								}						
													
							}
							else{
								//file non presente 
								makePacket(&pacchetto,sn,an,1);
								if (sendto(connsd, (const void *)&pacchetto, sizeof(packet), 0, (struct sockaddr *)&client, sizeof(struct sockaddr)) < 0) {
									perror("errore in sendto 1");
									exit(1);
								}
								
							}						
						break;

						case 2:
						
							//nella sezione per la gestione del comando put
							
							if(access(cmd[1],F_OK) == 0){
								//file presente nel server 
								makePacket(&pacchetto,sn,an,1);
								if (sendto(connsd, (const void *)&pacchetto, sizeof(packet), 0, (struct sockaddr *)&client, sizeof(struct sockaddr)) < 0) {
									perror("errore in sendto 1");
									exit(1);
								}
								
								
								
								
							}
							else{
								//file non presente nel server 
								makePacket(&pacchetto,sn,an,0);
								if (sendto(connsd, (const void *)&pacchetto, sizeof(packet), 0, (struct sockaddr *)&client, sizeof(struct sockaddr)) < 0) {
									perror("errore in sendto 1");
									exit(1);
								}	
								file_descriptor = open(cmd[1],O_CREAT|O_TRUNC|O_RDWR,0666);
								rcv_prob(connsd,&client,file_descriptor,an,sn,PROB);
							}
						break;
							
						case 3:
						
							//nella sezione per la gestione del comando get 
							
							if(access(cmd[1],F_OK) == 0){
								//file presente nel server, nella sezione perl invio
								makePacket(&pacchetto,sn,an,0);
								if (sendto(connsd, (const void *)&pacchetto, sizeof(packet), 0, (struct sockaddr *)&client, sizeof(struct sockaddr)) < 0) {
									perror("errore in sendto 1");
									exit(1);
								}
								file_descriptor = open(cmd[1],O_RDWR,0666);
								sendFile(file_descriptor,sn ,an ,connsd, (struct sockaddr_in)client,PROB); 				
							}
							else{
								//file non presente nel server 
								makePacket(&pacchetto,sn,an,1);
								if (sendto(connsd, (const void *)&pacchetto, sizeof(packet), 0, (struct sockaddr *)&client, sizeof(struct sockaddr)) < 0) {
									perror("errore in sendto 1");
									exit(1);
								}							
							}
								
						break;									
					}			
				}
				
			}		
		}		
		close(connsd); 		
	}
	
	pthread_join(tid, &value);
	exit_process_1:
		semctl(semID, -1, IPC_RMID, 0);
		

	exit_process_4:
		exit(1);
		free((void *)&arrayNumb);
}