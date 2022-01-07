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
#define SIZE  128 

// ******************************************************manda un  pacchetto per debug**********************************

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
//******************************************************variabili globali per contenere i numeri sequenza e ack del server**********************************




int main(int argc, char **argv){
	
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
	
	
	
	// if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reu, sizeof(int)) < 0)
		 // perror("setsockopt(SO_REUSEADDR) failed");
	
	  /* assegna l'indirizzo al socket */
	if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("errore in bind");
		exit(1);
	}
	printf("dentro il main\n");
	if (pthread_create(&tid,NULL,mylisten,(void *)arrayNumb) != 0 ) {
		perror("errore in mylisten");
		exit(1);
	}
	
	
	while(1){
		printf("*************************still alive*****************************\n\n");
		fflush(stdout);
		int connsd = myaccept(sockfd,&addr,&len);
		printf("\n **************sockfd %d,connsd %d********************\n ",sockfd,connsd);
	
		fflush(stdout);
		if ( (pid = fork()) == 0) {
			close(sockfd);
			
			
			printf("\n\n*******************************nel processo figlio: gestione della richiesta******************************\n\n"); 
			printf("il numero di socket del padre %d\n\n",sockfd);
			printf("il numero di socket e %d\n\n",connsd);
			printf("lindirizzo del client da servire\nporta: %d\nIP: %d\n ",client.sin_port, client.sin_addr.s_addr);
			memset((void *)&pacchetto,0,sizeof(packet));
			while(1){
				printf("*************************in loop**************************\n");
				getsockname(connsd, (struct sockaddr *)&addr,&len);
				printf("\n numero di sochet del figlio e numero di porta associato %d, %d*\n ",connsd,addr.sin_port);	
				if((recvfrom(connsd,(void *)&pacchetto, sizeof(packet), 0 ,(struct sockaddr *) &client,&len )) < 0) {
					perror("errore in recvfrom");
					exit(1);
				}			
				printf("il pacchetto ricevuto dal figlio:\n %d \n %d \n %d \n %d\n %s ",pacchetto.seqnumb,pacchetto.acknumb,pacchetto.flags.ack,pacchetto.flags.syn,pacchetto.data);
				if( pacchetto.flags.ack == 0  ){
					printf("superato il controllo\n\n");
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
					printf("le due stringe arrivate sono cmd[0]%s cmd[1]%s il caso e %d la grandezza del data e %ld\n\n",cmd[0],cmd[1],strswitch(cmd[0]),lendata);
					printf("numero sequenza %d numero atteso %d\n\n",pacchetto.seqnumb,((int32_t *)arrayNumb)[0]);
					memset((void *)&pacchetto,0,sizeof(pacchetto));
					switch(strswitch(cmd[0])) {
						case 0:
							printf("\n\n*********************in list*********************\n\n");
							
							makePacket(&pacchetto,sn,an,0);
							if (sendto(connsd, (const void *)&pacchetto, sizeof(packet), 0, (struct sockaddr *)&client, sizeof(struct sockaddr)) < 0) {
								perror("errore in sendto 1");
								exit(1);
							}
							
							fptr = fopen("a.txt", "wb");	//Create a file with write permission
							
							if (ls(fptr) == -1)		//get the list of files in present directory
								perror("ls");

							fclose(fptr);
							sendFile(fptr,sn ,an ,connsd, (struct sockaddr_in)client); 
							remove(fptr);
						break;
						
						case 1:
						
							printf("\n\n*********************in delete*********************\n\n");	
								if(access(cmd[1],F_OK) == 0){
									strcpy(last_file,cmd[1]);
									makePacket(&pacchetto,sn,an,0);
									printf("il pacchetto da mandare1:\n %d \n %d \n %d \n %d\n %s ",pacchetto.seqnumb,pacchetto.acknumb,pacchetto.flags.ack,pacchetto.flags.syn,pacchetto.data);
									remove(cmd[1]);
									// metti variabile ausiliare
									if (sendto(connsd, (const void *)&pacchetto, sizeof(packet), 0, (struct sockaddr *)&client, sizeof(struct sockaddr)) < 0) {
										perror("errore in sendto 1");
										exit(1);
									}						
														
								}
								else{
									if(strcmp(last_file,cmd[1]) == 0){
										printf("file gia arrivato,ritrasmetto l ok\n\n");
										makePacket(&pacchetto,sn,an,0);

										if (sendto(connsd, (const void *)&pacchetto, sizeof(packet), 0, (struct sockaddr *)&client, sizeof(struct sockaddr)) < 0) {
											perror("errore in sendto 1");
											exit(1);
										}
									}
									else{
										makePacket(&pacchetto,sn,an,1);
										printf("il pacchetto da mandare1:\n %d \n %d \n %d \n %d\n %s ",pacchetto.seqnumb,pacchetto.acknumb,pacchetto.flags.ack,pacchetto.flags.syn,pacchetto.data);
										if (sendto(connsd, (const void *)&pacchetto, sizeof(packet), 0, (struct sockaddr *)&client, sizeof(struct sockaddr)) < 0) {
											perror("errore in sendto 1");
											exit(1);
										}
									}
								}						
							break;

						case 2:
							printf("\n\n*********************in put*********************\n\n");
								if(access(cmd[1],F_OK) == 0){
									makePacket(&pacchetto,sn,an,1);
									printf("il pacchetto da mandare1:\n %d \n %d \n %d \n %d\n %s ",pacchetto.seqnumb,pacchetto.acknumb,pacchetto.flags.ack,pacchetto.flags.syn,pacchetto.data);
									if (sendto(connsd, (const void *)&pacchetto, sizeof(packet), 0, (struct sockaddr *)&client, sizeof(struct sockaddr)) < 0) {
										perror("errore in sendto 1");
										exit(1);
									}
									
									
									
									
								}
								else{
									makePacket(&pacchetto,sn,an,0);
									printf("il pacchetto da mandare1:\n %d \n %d \n %d \n %d\n %s ",pacchetto.seqnumb,pacchetto.acknumb,pacchetto.flags.ack,pacchetto.flags.syn,pacchetto.data);
									if (sendto(connsd, (const void *)&pacchetto, sizeof(packet), 0, (struct sockaddr *)&client, sizeof(struct sockaddr)) < 0) {
										perror("errore in sendto 1");
										exit(1);
									}	
									// creo il file	
									file_descriptor = open(cmd[1],O_CREAT|O_TRUNC|O_RDWR,0666);
									rcv(connsd,&client,file_descriptor,an,sn);
								}
						break;
							
						case 3:
							printf("\n\n*********************in get*********************\n\n");
								if(access(cmd[1],F_OK) == 0){
									makePacket(&pacchetto,sn,an,0);
									printf("il pacchetto da mandare1:\n %d \n %d \n %d \n %d\n %s ",pacchetto.seqnumb,pacchetto.acknumb,pacchetto.flags.ack,pacchetto.flags.syn,pacchetto.data);
									if (sendto(connsd, (const void *)&pacchetto, sizeof(packet), 0, (struct sockaddr *)&client, sizeof(struct sockaddr)) < 0) {
										perror("errore in sendto 1");
										exit(1);
									}
									printf("il fil e presente\n\n");	
									//apro il file
	 								file_descriptor = open(cmd[1],O_RDWR,0666);
									sendFile(file_descriptor,sn ,an ,connsd, (struct sockaddr_in)client); 				
								}
								else{
									makePacket(&pacchetto,sn,an,1);
									printf("il fil non e presente\n\n");
									printf("il pacchetto da mandare1:\n %d \n %d \n %d \n %d\n %s ",pacchetto.seqnumb,pacchetto.acknumb,pacchetto.flags.ack,pacchetto.flags.syn,pacchetto.data);
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
		printf("\n\n*************************PADRE DALL ALTRA PARTE*****************************\n\n");
		
		close(connsd); 
		
		
	}
	
	pthread_join(tid, &value);
	exit_process_1:
		semctl(semID, -1, IPC_RMID, 0);

	exit_process_4:
		exit(1);
}