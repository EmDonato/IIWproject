#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h>

#include <unistd.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <stdbool.h>

#include "packet.h"



#define SERV_PORT   5193 
#define MAXLINE     1024
#define CLOSED 0
#define SYN_SEND 1
#define ESTABLISHED 2

/* dentro al main vi e la creazione el attesa del thread che andra ad occuparsi della connessone, ovvero il "my connaction". 
Quest'ultimo e implementato in modo tale da farsi consegnare il segnale SIGALARM ogni 1 secondi, per tentare di ritentare la connessione
se il server non trasmette l ack del syn. la gestione del segnale e compito della funzione delete, la quale va a cancellare il thread maeconnection, 
il thread che implementa verametelo scambio di pacchetti, quest ultimo thread manda pacchetti e cambia lo stato del client, fino a quando non e ESTABLISHED 
attende l ack del server, se lo riceve cambia stato e invia il pacchetto di conferma */


volatile int state=CLOSED;
int sockfdglb;
pthread_t  tidglb;
int32_t seqN,ackN;
int newPort;
struct sockaddr_in   servaddr;

void *makeconnection(void *arg){
	
	//thread per la gestionedella connessione, si occupa del handshaking
	
	struct sockaddr_in servaddrrcv;
	packet pac,pacrcv;
	srand(time(NULL));
	int n, len =sizeof(struct sockaddr_in);
	int32_t r = rand(); //numero della sequenza
	seqN = r;
	memset((void *)&pac,0,sizeof(packet));
	// inizializzazione del pacchetto
	int numb;
	float probaus;	
	srand (time(NULL));		
	numb = rand();
	probaus = (double)numb /(double)RAND_MAX;
	pac.seqnumb=r;
	pac.flags.syn=1;

	if (sendto(sockfdglb, (const void *)&pac, sizeof(packet), 0, (struct sockaddr *)arg, len ) < 0) {
		perror("errore in sendto 1");
		exit(1);
	}
	state = SYN_SEND;
	while(state != ESTABLISHED){
		
		n = recvfrom(sockfdglb,(void *)&pacrcv, sizeof(packet), 0 ,(struct sockaddr *)&servaddrrcv,&len ); // ************checkare se viene dallo stesso indirizzo ip********
		if(probaus<=PROB){

		}
		else{
			if (n < 0) {
				perror("errore in recvfrom");
				exit(1);
			}
			if(pacrcv.flags.ack == 1 && pacrcv.flags.syn == 1){
				state = ESTABLISHED;
				newPort = atoi(pacrcv.data);
				memset((void *)&pac,0,sizeof(packet));

				pac.seqnumb = r;
				pac.flags.ack = 1;
				pac.flags.syn=0;
				pac.acknumb = pacrcv.seqnumb + 1;
				ackN = pac.acknumb;
				if (sendto(sockfdglb, (const void *)&pac, sizeof(packet), 0, (struct sockaddr *)arg , len) < 0) {
					perror("errore in sendto 2");
					exit(1);
				}
			}
		}
	}
	pthread_exit(0);
}



void delete(int sig){
	
	// gestione del segnale SIGINT
	if(state!=ESTABLISHED){
		pthread_cancel(tidglb);
		state=CLOSED;
	}

	
}



void *myconnect(void *arg){
	
	//thread principale per la connessione 
	
	pthread_t tid;
	void *status;
	// inizializzazione gestione del segnale
	struct sigaction act;
	sigset_t set ;
	sigfillset(&set);
	memset((void *)&act,0,sizeof(struct sigaction));
	if((sigprocmask(SIG_BLOCK,&set,NULL))==-1){
		printf("errore nel settare i segnali");
		exit(1);
	
	}
	act.sa_mask = set;
	act.sa_handler = delete;
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
	
	// mi cerco di connettere fino a quando la connessione non e stabilita
	
	while(state!=ESTABLISHED){
		//metto la sveglia
		alarm(1);
		if(pthread_create(&tid,NULL,makeconnection, arg)!=0){//metti gli argmenti 
			exit(1);
		}
		
		tidglb=tid;
		if((pthread_join(tid, &status) ) == -1){
	         exit(1);  //gestisci lo stato e l errore 
            }
	}
	alarm(0); // tolgo la sveglia
	pthread_exit(0);
	
	
}
	
void *echo_last(void * arg){
	
	//funzione che gestisce la perdita dell ultimo pacchetto del file 
	packet pac;
	int32_t aus;
	int len = sizeof(struct sockaddr);
	while(1){
		if((recvfrom(sockfdglb,(void *)&pac, sizeof(packet), 0 ,(struct sockaddr *)&servaddr,&len )) < 0) {
			perror("errore in recvfrom");
			exit(1);
		}
		aus = pac.seqnumb+pac.data_size + 1;
		pac.seqnumb = pac.acknumb;
		pac.acknumb = aus;
		pac.flags.ack = 1;
		pac.data_size = 0;
		if (sendto(sockfdglb, (const void *)&pac, sizeof(packet), 0, (struct sockaddr *)&servaddr, len) < 0) {
			perror("errore in sendto 1");
			exit(1);
		}
	}
	pthread_exit((void *)0);
}




int main(int argc, char *argv[ ]) {

	int   sockfd, n,i=0,resRequest,newsockfd, file_descriptor;
	void *status;
	pthread_t tid,tid_echo;
	packet packet;
	memset((void *)&packet,0,sizeof(packet));
	char command[128], command2[128];
	char cmd[2][64];
	char * aus;
	if (argc != 2) { /* controlla numero degli argomenti */
	fprintf(stderr, "utilizzo: TCPclient  <indirizzo IP server>\n");
	exit(1);
	}

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { /* crea il socket */
		perror("errore in socket");
		exit(1);
	}
	sockfdglb=sockfd;
	memset((void *)&servaddr, 0, sizeof(servaddr));      /* azzera servaddr */
	servaddr.sin_family = AF_INET;       /* assegna il tipo di indirizzo */
	servaddr.sin_port = htons(SERV_PORT);  /* assegna la porta del server */
	/* assegna l'indirizzo del server prendendolo dalla riga di comando. L'indirizzo � una stringa da convertire in intero secondo network byte order. */
	if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
					/* inet_pton (p=presentation) vale anche per indirizzi IPv6 */
		fprintf(stderr, "errore in inet_pton per %s", argv[1]);
		exit(1);
	}


	//************ la connessione *******************


	if(pthread_create(&tid,NULL,myconnect,(void *)&servaddr)!=0){//metti gli argmenti 
	 exit(1);
	}
	if((pthread_join(tid, &status) ) == -1){
	  exit(1);  //gestisci lo stato e l errore 
	}
	//aggiorno il numero della porta
	servaddr.sin_port = newPort;  /* assegna la porta del server */
	memset((void *)&packet,0,sizeof(packet));

	while(1){
		printf("\n\n*********************digita un comando*********************\n\ncomandi disponibili: [list,put,get,delete]");
		memset((void *)cmd,0,sizeof(int32_t)*2);
		pthread_create(&tid_echo,NULL,echo_last,NULL);
		scanf("%[^\n]",command);
		getchar();
		stpcpy(command2,command);
		aus = strtok (command," ");
		i = 0;
		while (aus != NULL)
		{
			strcpy(cmd[i],aus);
			i++;
			aus = strtok (NULL, " ");
			
		}
	
		pthread_cancel(tid_echo);//gestione perdita ack ultimo pacchetto
		
		
		switch(strswitch(cmd[0])) {
			
			case 0:
				// in list 
				createFirstPac(&packet,seqN,ackN,command2);
				seqN = seqN + strlen(command2)+1;
				
				if((resRequest = sendRequest(sockfd,&servaddr,&packet)) == 0){
					
					printf("\n\n************************************************** FILE NEL SERVER ******************************************************\n\n");
					file_descriptor = open("list.txt",O_CREAT|O_TRUNC|O_RDWR,0666);
					rcv_prob(sockfd,&servaddr,file_descriptor,ackN,seqN,PROB);
					printfList("list.txt");
					remove("list.txt");
				}
				else{
					printf("\n\n*********************************problema con la list*********************************************\n\n");
				}
				break;
			
			case 1:
				// in delete
				createFirstPac(&packet,seqN,ackN,command2);
				seqN = seqN + strlen(command2)+1;
				if((resRequest = sendRequest(sockfd,&servaddr,&packet)) == 0){
					
					printf("\n\n********************************* FILE CANCELLATO *********************************************\n\n");
					
				}	
				else{
					printf("\n\n*********************************file non presente nel server*********************************************\\n");
				}
			break;

			case 2:
				// in put
				createFirstPac(&packet,seqN,ackN,command2);
				seqN = seqN + strlen(command2)+1;
				if((resRequest = sendRequest(sockfd,&servaddr,&packet)) == 0){
					
					file_descriptor = open(cmd[1],O_RDWR,0666);
					sendFile(file_descriptor,seqN ,ackN ,sockfd, (struct sockaddr_in)servaddr,PROB); 	
					printf("\n\n********************************* FILE SPEDITO *********************************************\n\n");

				}
				else{
					printf("\n\n***********************file gia presente nel server o non spazio disponibile sul server**********************************\n\n");
				}			
			break;
				
			case 3:
				// in get
				createFirstPac(&packet,seqN,ackN,command2);
				seqN = seqN + strlen(command2)+1;
				if((resRequest = sendRequest(sockfd,&servaddr,&packet)) == 0){
					file_descriptor = open(cmd[1],O_CREAT|O_TRUNC|O_RDWR,0666);
					rcv_prob(sockfd,&servaddr,file_descriptor,ackN,seqN,PROB);
					printf("\n\n********************************* FILE RICEVUTO *********************************************\n\n");

				}
				else{
					printf("\n\n*********************************file non presente nel server*********************************************\n\n");
				}			
			break;			
		}
	}
return 0;
}
