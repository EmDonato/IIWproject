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
Quest'ultimo e implementato in modo tale da farsi consegnare il segnale SIGALARM ogni 3 secondi, per tentare di ritentare la connessione
se il server non trasmette l ack del syn. la gestione del segnale e compito della funzione delete, la quale va a cancellare il thread maeconnection, 
il thread che implementa verametelo scambio di pacchetti, quest ultimo thread manda pacchetti e cambia lo stato del client, fino a quando non e ESTABLISHED 
attende l ack del server, se lo riceve cambia stato e invia il pacchetto di conferma */


volatile int state=CLOSED;
int sockfdglb;
pthread_t  tidglb;
int32_t seqN,ackN;



void *makeconnection(void *arg){
	
	
	//printf("dentro la funione della connessione\n"); 
	struct sockaddr_in servaddrrcv;
	packet pac,pacrcv;
	srand(time(NULL));
	int n,len =sizeof(struct sockaddr_in);
	int32_t r = rand(); //numero della sequenza
	seqN = r;
	memset((void *)&pac,0,sizeof(packet));
	// inizializzazione del pacchetto
	
	pac.seqnumb=r;
	pac.flags.syn=1;
	if (sendto(sockfdglb, (const void *)&pac, sizeof(packet), 0, (struct sockaddr *)arg, len ) < 0) {
		perror("errore in sendto 1");
		exit(1);
	}
	state = SYN_SEND;
	printf("lo stato e: %d\n",state);
	while(state != ESTABLISHED){
		
		n = recvfrom(sockfdglb,(void *)&pacrcv, sizeof(packet), 0 ,(struct sockaddr *)&servaddrrcv,&len ); // ************checkare se viene dallo stesso indirizzo ip********
		if (n < 0) {
			perror("errore in recvfrom");
			exit(1);
		}
		if(pacrcv.flags.ack == 1 && pacrcv.flags.syn == 1){
			state = ESTABLISHED;
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
	pthread_exit(0);
 
	
}



void delete(int sig){
	
	
	if(state!=ESTABLISHED){
		printf("dentro la funzione che cancella l pid; %ld\n",tidglb);
		pthread_cancel(tidglb);
		state=CLOSED;
	}

	
}



void *myconnect(void *arg){
	
	
	pthread_t tid;
	
	void *status;
	// struttura per la gestione del segnale che non vogliamo ignorare o in generale vogliamo cambiargli lo stato
	struct sigaction act;
	// maschera di bit per la gestione del blocco dei segnali
	sigset_t set ;
	// tutto a 1
	sigfillset(&set);
	// azzeriamo la struttura 
	memset((void *)&act,0,sizeof(struct sigaction));
// rendiamo valida la bitmap bloccando tutto quello a 1
	if((sigprocmask(SIG_BLOCK,&set,NULL))==-1){
		printf("errore nel settare i segnali");
		exit(1);
	
	}
	// inizializiamo la struttura gli diamo la maskera e la funzione 
	act.sa_mask = set;
	act.sa_handler = delete;//scrivere la funzione //capire come mandare gli argomenti 
	// cambiamo lo stato e gestiamo il segnale 
	if((sigaction(SIGALRM,&act,NULL))==-1){
		printf("errore nel settare i segnali");
		exit(1);
	}
	// azzeriamo la bitmap
	if(sigemptyset(&set)) exit(1);
	// mettiamo a 1 i segnali che vogliamo gestire
	if(sigaddset(&set,SIGALRM)) exit(1);
	if(sigaddset(&set, SIGINT)) exit(1);
	// tutti quelli a 0 rimangono bloccati gli 1 sbloccati
	if((sigprocmask(SIG_UNBLOCK,&set,NULL))==-1){
		printf("errore nel settare i segnali\n");
		exit(1);
	
	}
	
	// mi cerco di connettere fino a quando la connessione non e stabilita
	
	while(state!=ESTABLISHED){
		//metto la sveglia
		alarm(3);
		if(pthread_create(&tid,NULL,makeconnection, arg)!=0){//metti gli argmenti 
			exit(1);
		}
		
		tidglb=tid;
		printf("tidglb ha questo tid: %ld\n",tidglb);
		if((pthread_join(tid, &status) ) == -1){
	         exit(1);  //gestisci lo stato e l errore 
            }
	}
	alarm(0); // tolgo la sveglia
	pthread_exit(0);
	
	
}
	





int main(int argc, char *argv[ ]) {

	int   sockfd, n,i=0,resRequest;
	void *status;
	pthread_t tid;
	
	struct    sockaddr_in   servaddr;
	packet packet;
	memset((void *)&packet,0,sizeof(packet));
	char command[128], command2[128];
	char cmd[2][64];
	char * aus;
	if (argc != 2) { /* controlla numero degli argomenti */
	fprintf(stderr, "utilizzo: daytime_clientUDP <indirizzo IP server>\n");
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

	memset((void *)&packet,0,sizeof(packet));


	// preso il comando


	printf("\n\n*********************la connessione e stabilita: %d*********************\n\n",state);
while(1){
	printf("\n\n*********************digita un comando*********************\n\ncomandi disponibili: [list,put,get,delete]");
	memset((void *)cmd,0,sizeof(int32_t)*2);
	
	scanf("%[^\n]",command);
	getchar();
	printf("il comando e %s\n\n",command);
	stpcpy(command2,command);
	printf("il comando2 e %s\n\n",command2);
	aus = strtok (command," ");
	i = 0;
	printf("aus e %s\n",aus);
	while (aus != NULL)
	{
		printf("in strtok loop\n\n");
		strcpy(cmd[i],aus);
		i++;
		aus = strtok (NULL, " ");
		
	}
	if(i==2){
		printf("il comando digitato e: %s\nsul file: %s\n",cmd[0],cmd[1]);
	}
	else{
		printf("il comando digitato e: %s\n",cmd[0]);
	}
	
	// finito di prendere il comando
	
	
	//gestione del comando 
	
	switch(strswitch(cmd[0])) {
		
		case 0:
			printf("\n\n*********************digitato list*********************\n\n");
			createFirstPac(&packet,seqN,ackN,command2);
			printf("il pacchetto da mandare contiene:\n %d \n %d \n %d \n %d\n %s ",packet.seqnumb,packet.acknumb,packet.flags.ack,packet.flags.syn,packet.data);
			if((resRequest = sendRequest(sockfd,&servaddr,&packet)) == 0){
				
				printf("pacchetto mandato correttamente\n\n");
				
				
			}
			else{
				printf("/n/n*********************************problema con la list*********************************************/n/n");
			}
			break;
		
		case 1:
			printf("\n\n*********************digitato delete*********************\n\n");
			createFirstPac(&packet,seqN,ackN,command2);
			printf("il pacchetto da mandare contiene:\n %d \n %d \n %d \n %d\n %s ",packet.seqnumb,packet.acknumb,packet.flags.ack,packet.flags.syn,packet.data);
			if((resRequest = sendRequest(sockfd,&servaddr,&packet)) == 0){
				
				printf("pacchetto mandato correttamente\n\n");
				
			}	
			else{
				printf("/n/n*********************************file non presente nel server*********************************************/n/n");
			}
		break;

		case 2:
			printf("\n\n*********************digitato put*********************\n\n");
			createFirstPac(&packet,seqN,ackN,command2);
			printf("il pacchetto da mandare contiene:\n %d \n %d \n %d \n %d\n %s ",packet.seqnumb,packet.acknumb,packet.flags.ack,packet.flags.syn,packet.data);
			if((resRequest = sendRequest(sockfd,&servaddr,&packet)) == 0){
				
				printf("pacchetto mandato correttamente\n\n");
				
			}
			else{
				printf("/n/n*********************************file gia presente nel server o non spazio disponibile sul server*********************************************/n/n");
			}			
			break;
			
		case 3:
			printf("\n\n*********************digitato get*********************\n\n");
			createFirstPac(&packet,seqN,ackN,command2);
			printf("il pacchetto da mandare contiene:\n %d \n %d \n %d \n %d\n %s ",packet.seqnumb,packet.acknumb,packet.flags.ack,packet.flags.syn,packet.data);
			if((resRequest = sendRequest(sockfd,&servaddr,&packet)) == 0){
				
				printf("pacchetto mandato correttamente\n\n");
				
			}
			else{
				printf("/n/n*********************************file non presente nel server*********************************************/n/n");
			}			
			break;			
	}
}
	
	
	return 0;

}
