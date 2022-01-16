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
#include "packet.h"
 

extern key_t semkey;
extern int sockfd, connsd;
extern int IDsem;
extern struct sockaddr_in accepted[BACKLOG];
extern struct sockaddr_in syn_rcvd[N];
extern struct gate gateAccepted[BACKLOG];
extern struct gate gateSyn_rcvd[N];
extern pthread_t tidglb;
extern bool ausiliarAccepted[BACKLOG];
extern bool ausiliarSyn_rcvd[N];
extern int semID;
int32_t snprc;
int32_t anprc;

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
			accepted[x].sin_family = AF_INET;
			accepted[x].sin_port = addr->sin_port;
			accepted[x].sin_addr.s_addr = addr->sin_addr.s_addr;
			ausiliarAccepted[x] = true;
			gateAccepted[x].sockID=gateSyn_rcvd[i].sockID;
			gateAccepted[x].address=gateSyn_rcvd[i].address;
			memset((void *)&gateSyn_rcvd[i],0,sizeof(struct gate));			

			return 1;
		}
	}
	return 0;

}


void *mylisten(void *arg){
	
	/* funzione che simula la funzione di libreria listen(), si occupa dell'handshaking tra il server 
	e il client, in base a due a i pacchetti che arrivano e lo stato delle stutture di controllo per la connessione
	vengono gestiti i client, a ogni cliente viene associata una nuova porta libera per poi successivamente permettere 
	la connessione sicura 	*/
	
	fflush(stdout);
	srand (time(NULL));	
	int32_t r = rand(); //numero della sequenza
	int numb;
	float probaus;
	srand (time(NULL));		
	numb = rand();
	probaus = (double)numb /(double)RAND_MAX;
	struct sembuf buf[1];
	memset((void *)&accepted, 0, BACKLOG*sizeof(struct sockaddr_in));
	memset((void *)&syn_rcvd, 0, N*sizeof(struct sockaddr_in));
	memset((void *)&gateAccepted, 0, BACKLOG*sizeof(struct gate));
	memset((void *)&gateSyn_rcvd, 0, N*sizeof(struct gate));	
	char nameint[6];
	int i=0,len=sizeof(struct sockaddr_in);
	int foundresult;
	struct sockaddr_in addr,newAddr; 
	packet pacrcv,pacchetto;	 
	srand(time(NULL));
	memset((void *)&pacrcv, 0, sizeof(packet));
	memset((void *)&pacchetto, 0, sizeof(packet));
	int ids,portaus, lenname = sizeof(struct sockaddr);
	
 
	while(1){
		if((recvfrom(sockfd,(void *)&pacrcv, sizeof(packet), 0 ,(struct sockaddr *) &addr,&len )) < 0) {
			perror("errore in recvfrom");
			exit(1);
		}
		if(probaus<=PROB){

		}
		else{

			if(pacrcv.flags.ack==0 && pacrcv.flags.syn==1){
				if((foundresult=found(&addr,0)) > -1){
					srand(time(NULL));
					syn_rcvd[foundresult]=addr; //se non lo trovo lo aggiungo
					//criamo il socket 
					memset((void *)&newAddr, 0, sizeof(struct sockaddr_in));
					if ((ids = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
						perror("errore in socket");
						exit(1); 
					}
					newAddr.sin_family = AF_INET;
					newAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* il server accetta pacchetti su una qualunque delle sue interfacce di rete */
					newAddr.sin_port = htons(0); /* numero di porta del server */				
					if (bind(ids, (struct sockaddr *)&newAddr, sizeof(struct sockaddr_in)) < 0) {
						perror("errore in bind");
						exit(1);
					}
					gateSyn_rcvd[foundresult].sockID =ids;
					getsockname(ids, (struct sockaddr *)&newAddr,&lenname);
					gateSyn_rcvd[foundresult].address = newAddr.sin_port;
					ausiliarSyn_rcvd[foundresult] = true;
					pacchetto.seqnumb = r; //assegno un numero casuale come numero sequenza
					pacchetto.acknumb = pacrcv.seqnumb + 1; //richiedo il byte successivo
					pacchetto.flags.ack = 1;
					pacchetto.flags.rst = 0;
					pacchetto.flags.syn = 1;
					pacchetto.flags.fin = 0;
					sprintf(nameint,"%d",gateSyn_rcvd[foundresult].address);
					strcpy(pacchetto.data,nameint);

					if (sendto(sockfd, (const void *)&pacchetto, sizeof(packet), 0, (struct sockaddr *)&addr, len ) < 0) {
						perror("errore in sendto 1");
						exit(1); // attemzione
					}
					
				}
				else if(foundresult == -2){

				}
				else if(foundresult == -1){
					if (sendto(sockfd, (const void *)&pacchetto, sizeof(packet), 0, (struct sockaddr *)&addr, len ) < 0) {
						perror("errore in sendto 1");
						exit(1); // attemzione
						}
				}
			}

			else if(pacrcv.flags.ack==1 && pacrcv.flags.syn==0){    // controllare il numero dell ack
				if((foundresult=found(&addr,1)) > -1){
					if(foundAndDetroy(&addr, foundresult)){
						buf[0].sem_num = 0;
						buf[0].sem_op = 1;
						buf[0].sem_flg = 0;
						semop(semID, buf, 1);
						((int32_t *)arg)[0] = pacrcv.seqnumb;
						((int32_t *)arg)[1] = pacrcv.acknumb;
					
					}
					else{
						
					}
				}
				else if(foundresult == 0){
				}
				
			}

		}
	}
}



int myaccept(int sockfd, struct sockaddr_in *addr1, socklen_t *addrlen){
	
/* 	fuzione che simula la accept(), viene preso il prim client
	che ha suoerato l'handshake, nella pila delle accepted, viene restituito 
	l'identificativo del sockt e viene,tramite il puntatore, restituito le info sul client  */
	
	struct sockaddr_in addrpers;
	memset((void *)&addrpers, 0, sizeof(addrpers));
	int i = 0, IDsock;
	struct sembuf buf[1];
	buf[0].sem_num = 0;
	buf[0].sem_op = -1;
	buf[0].sem_flg = 0;
	semop(semID, buf, 1);
	while(ausiliarAccepted[i] == false){
		i++;
	}
	addr1->sin_family = accepted[i].sin_family;
	addr1->sin_port = accepted[i].sin_port;
	addr1->sin_addr.s_addr = accepted[i].sin_addr.s_addr;
	*addrlen = sizeof(accepted[i]);
	memset((void *)&accepted[i], 0, sizeof(struct sockaddr_in));
	ausiliarAccepted[i] = false;
	IDsock = gateAccepted[i].sockID;
	memset((void *)&gateAccepted[i], 0, sizeof(struct gate));
	return IDsock;
	
}


void shutout(int sig){
	
	//gestore per il segnale SIGINT, si occupa della chiusura del server 
	printf("\n\n**************************** SERVER NON ATTIVO ****************************\n\n");
	semctl(semID, -1, IPC_RMID, 0);
	raise(SIGKILL);
	
}



int strswitch(char *str){
	
	//funzione che si occupa dell hashing tra il comando digitato e il giusto intero  
	
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



void makePacket(packet *p,int32_t sn,int32_t an, int  flg){
	
	//funzione che crea il pacchetto dati in risposta alla richiesta di comando, 
	//se flg == 0 allore tutto e andato a buon fine, flg == 1 errore generico
	
	if(flg == 0){	
		p->seqnumb = sn;
		p->acknumb = an;
		p->flags.ack = 1;
		strcpy(p->data,"OK");
	}
	else{
		p->seqnumb = sn;
		p->acknumb = an;
		p->flags.ack = 1;
		strcpy(p->data,"ERROR");		
	}
}


int ls(FILE *f) { 

	//funzione che stampa il direttorio corrente in un file 
	
	struct dirent **dirent; int n = 0;

	if ((n = scandir(".", &dirent, NULL, alphasort)) < 0) { 
		perror("Scanerror"); 
		return -1; 
	}
        
	while (n--) {
		fprintf(f, "%s\n", dirent[n]->d_name);	
		free(dirent[n]); 
	}
	
	free(dirent); 
	return 0; 

}














