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
#define SERV_PORT 5193
#define BACKLOG 10
#define MAXLINE 1024
#define N 10


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


int found(struct sockaddr_in *addr,int x){
	
	//funzione che ricerca un posto nella coda per stabilire una connessione
	/*viene restituito:
				n tra 0 e N se non e presente il mittente e vi e' un posto libero, posto in posizione n (il primo trovato)
				-1 se gia e presente il mittente 
				-2 se non ci sono piu posti dispoibili
	
	*/
	int position;
	int aus = 0;
	int i = 0;
	bool *list;
	if(x==0){
		list=ausiliarSyn_rcvd;
		for(i=0;i<N;i++){
			if(list[i]==false && aus==0){
				aus = 1;
				position = i;
			}
			else if(addr->sin_port==syn_rcvd[i].sin_port && addr->sin_addr.s_addr==syn_rcvd[i].sin_addr.s_addr){   //se lo trovo
				aus = 1;
				position = -1;
				
			}
		}
	printf("\n\nla posizione trovata e la numero: %d\n\n",position);
		
	}
	else{
		list=ausiliarAccepted;
		for(i=0;i<BACKLOG;i++){
			if(list[i]==false && aus==0){
				aus = 1;
				position = i;
			}
			else if(addr->sin_port==accepted[i].sin_port && addr->sin_addr.s_addr==accepted[i].sin_addr.s_addr){   //se lo trovo
				aus = 1;
				position = -1;
				
			}
		}
	}

	if(aus==0)
		position = -2;
	
	
	
	
	
	return position;
}


int foundAndDetroy(struct sockaddr_in *addr,int x){
	
	//funzione che trova la  connessione nel syn la cancella e mette nella prima posizione libera dell established
	/*viene restituito:
				1 se è riuscito
				0 se non è riuscito
	*/
	int position;
	int aus = 0;
	int i = 0;
	bool *list;

	list=ausiliarSyn_rcvd;

	for(i=0;i<N;i++){

		if(addr->sin_port==syn_rcvd[i].sin_port && addr->sin_addr.s_addr==syn_rcvd[i].sin_addr.s_addr){   //se lo trovo lo cancello e lo metto nella connected 
			memset((void *)&syn_rcvd[i], 0, sizeof(struct sockaddr_in));
			ausiliarSyn_rcvd[i] = false;
			// lo metto nella connected
			accepted[x].sin_port = addr->sin_port;
			accepted[x].sin_addr.s_addr = addr->sin_addr.s_addr;
			ausiliarAccepted[x] = true;

			return 0;
		}
	}
	return 1;

}


void *mylisten(void *arg){
	
/* 	printf("dentro il my listen\n");
	fflush(stdout);
	if ((semID = semget(semkey, 1, IPC_CREAT|0666)) == -1)
		goto exit_process_1;
	
	if (semctl(semID, 0, SETVAL, 0) == -1)
		goto exit_process_1;
	struct sembuf buf[1];
	 */
	
	
	
	
	printf("dentro il my listen\n");
	
	//prima cosa pulisco gli array dove conterro le connessioni
	
	memset((void *)&accepted, 0, BACKLOG*sizeof(struct sockaddr_in));
	memset((void *)&syn_rcvd, 0, N*sizeof(struct sockaddr_in));
	
	int i=0,len=sizeof(struct sockaddr_in);
	int foundresult;
	struct sockaddr_in addr; //struttura dove verra consegnato l informazione sul mittente
	packet pacrcv,pacchetto;	 //struttura dove verra consegnato il pacchetto, pacchetto e quello che mandi
	srand(time(NULL));
	memset((void *)&pacrcv, 0, sizeof(packet));
	memset((void *)&pacchetto, 0, sizeof(packet));
	//creo il semaforo per la connect
 
 
 
	while(1){
		//ricevo un pacchetto
		if((recvfrom(sockfd,(void *)&pacrcv, sizeof(packet), 0 ,(struct sockaddr *) &addr,&len )) < 0) {
			perror("errore in recvfrom");
			exit(1);
		}
		
		//controllo il pacchetto che e arrivato
		printf("wòibfjwkrabfpiqbwobèoufbq\n\n\n\n ");

		printf("il pacchetto arrivato e:\n numb:%d\nack:%d\nack:%d\nsyn:%d\n ",pacrcv.seqnumb,pacrcv.acknumb,pacrcv.flags.ack,pacrcv.flags.syn);
		
		
		//checko il tipo di messaggio che ricevo e da li controllo in che array controllare
		if(pacrcv.flags.ack==0 && pacrcv.flags.syn==1){
			//cerco     se lo trovo ora non sto facendo nulla
			if((foundresult=found(&addr,0)) > -1){
				printf("la foundresult e di %d\n\n",foundresult);
				syn_rcvd[foundresult]=addr; //se non lo trovo lo aggiungo
				ausiliarSyn_rcvd[foundresult] = true;
				pacchetto.seqnumb = rand(); //assegno un numero casuale come numero sequenza
				pacchetto.acknumb = pacrcv.seqnumb + 1; //richiedo il byte successivo
				pacchetto.flags.headlen = 10;		//**************cambiare size of
				pacchetto.flags.ack = 1;
				pacchetto.flags.rst = 0;
				pacchetto.flags.syn = 1;
				pacchetto.flags.fin = 0;
				if (sendto(sockfd, (const void *)&pacchetto, sizeof(packet), 0, (struct sockaddr *)&addr, len ) < 0) {
					perror("errore in sendto 1");
					exit(1); // attemzione
				}
				
			}
			else if(foundresult == -2){
				printf("non piu spazio nella syn_rcvd\n");
			}
			else if(foundresult == -1){
				printf("gia presenten nella syn_rcvd\n");
			}
		}

		else if(pacrcv.flags.ack==1 && pacrcv.flags.syn==0){    // controllare il numero dell ack
			//trovare nella lista e toglierlo e metterlo nell altra list se c'e un posto libero
			if((foundresult=found(&addr,1)) > -1){
				if(foundAndDetroy(&addr, foundresult)){
					//aumento il semaforo di uno
/* 					buf[0].sem_num = 0;
					buf[0].sem_op = 1;
					buf[0].sem_flg = 0;
					semop(semID, buf, 1); */
					printf("completato l handshake\n");
				}
				else{
					printf("non presente nella lista dei syn\n");
				}
			}
			else if(foundresult == 0){
				printf("non piu spazio nella accepted\n");
			}
			
	    }
		
		
		// printa ogni volta il contenuto debug  CANCELLARE
		
		i = 0;
		printf("contenuto della lista delle connessioni stabilite: \n");
		for(i=0;i<BACKLOG;i++){
			printf("porta: %d\nIP: %d \n\n",accepted[i].sin_port, accepted[i].sin_addr.s_addr);
			
		
		}
		i = 0;
		printf("contenuto della lista delle connessioni in attesa di syn ack: \n");		
		for(i=0;i<N;i++){
			printf("porta: %d\nIP: %d \n\n",syn_rcvd[i].sin_port, syn_rcvd[i].sin_addr.s_addr);
			
		
		}		
	}
	exit_process_1:
		semctl(semID, -1, IPC_RMID, 0);

	exit_process_4:
		exit(1);
}


int myaccept(int sockfd, struct sockaddr_in *addr, socklen_t *addrlen){
	
	
	struct sockaddr_in addrpers;
	int i = 0, IDsock;
	struct sembuf buf[1];
	buf[0].sem_num = 0;
	buf[0].sem_op = -1;
	buf[0].sem_flg = 0;
	semop(semID, buf, 1);
	while(ausiliarAccepted[i] == false)
		i++;
	addr->sin_port = accepted[i].sin_port;
	addr->sin_addr.s_addr = accepted[i].sin_addr.s_addr;
	*addrlen = sizeof(accepted[i]);
	memset((void *)&accepted[i], 0, sizeof(struct sockaddr_in));
	ausiliarAccepted[i] = false;
	if ((IDsock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		perror("errore in socket");
		exit(1); 
	}
	
	memset((void *)&addrpers, 0, sizeof(addrpers));
	addrpers.sin_family = AF_INET;
	addrpers.sin_addr.s_addr = htonl(INADDR_ANY); /* il server accetta pacchetti su una qualunque delle sue interfacce di rete */
	addrpers.sin_port = htons(SERV_PORT); /* numero di porta del server */
	
	  /* assegna l'indirizzo al socket */
	if (bind(sockfd, (struct sockaddr *)&addrpers, sizeof(addrpers)) < 0) {
		perror("errore in bind");
		exit(1);
	}	
	return IDsock;
	
}


void shutout(int sig){
	
	
	// semctl(semID, -1, IPC_RMID, 0);
	exit(1);
	
}

int main(int argc, char **argv){
	
	int i;
	pid_t pid;
	pthread_t tid;
	void *value;
	struct sockaddr_in addr, client;
	int len;
	socklen_t clientlen;
	key_t keysem = 32145;
	int lenLissen = sizeof(struct sockaddr_in);
	sigset_t set;
	packet pacchetto;
	char *strstr = "HELLO WORLD\n";

	/*
		Gestione del segnale SIGINT

	*/

	
	sigfillset(&set);
	struct sigaction act;
	act.sa_mask = set;
	act.sa_handler = shutout;
	sigaction(SIGINT,&act,NULL);


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
	printf("dentro il main\n");
	if (pthread_create(&tid,NULL,mylisten,NULL) != 0 ) {
		perror("errore in mylisten");
		exit(1);
	}
	printf("sgfdhjklò\n");
/* 	while(1){
 		connsd = myaccept(sockfd,&client,&clientlen);
		if ( (pid = fork()) == 0) {
			close(sockfd);
			// manda un pacchetto per vedere se sei connesso 
			pacchetto.seqnumb=1242;
			pacchetto.acknumb=21513;
			pacchetto.flags.headlen=10;
			pacchetto.flags.ack=0;
			pacchetto.flags.rst=0;
			pacchetto.flags.syn=1;
			pacchetto.flags.fin=0;
			for(i=0;i<strlen("HELLO WORLD\n");i++){
				pacchetto.data[i]=strstr[i];
			}
			if (sendto(connsd, (const void *)&pacchetto, sizeof(packet), 0, (struct sockaddr *)&addr, len ) < 0) {
				perror("errore in sendto 1");
				exit(1); // attemzione
			}			
		
			
		}
		close(connsd); 
	} */
	
	pthread_join(tid, &value);
}