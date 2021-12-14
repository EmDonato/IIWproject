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
#define SERV_PORT   5193 
#define SIZE  128 

extern key_t semkey;
extern int sockfd, connsd;
extern int IDsem;
extern struct sockaddr_in accepted[BACKLOG];
extern struct sockaddr_in syn_rcvd[N];
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
			accepted[x].sin_family = AF_INET;
			accepted[x].sin_port = addr->sin_port;
			accepted[x].sin_addr.s_addr = addr->sin_addr.s_addr;
			ausiliarAccepted[x] = true;
			return 1;
		}
	}
	return 0;

}


void *mylisten(void *arg){
	
	printf("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n");
	fflush(stdout);
	int32_t r = rand(); //numero della sequenza
	
	struct sembuf buf[1];
	
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
		printf("dentro il while del my listen\n\n\n\n ");

		printf("il pacchetto arrivato e:\n numb:%d\nack:%d\nack:%d\nsyn:%d\n il campo data%s\n ",pacrcv.seqnumb,pacrcv.acknumb,pacrcv.flags.ack,pacrcv.flags.syn,pacrcv.data);
		
		
		//checko il tipo di messaggio che ricevo e da li controllo in che array controllare
		if(pacrcv.flags.ack==0 && pacrcv.flags.syn==1){
			//cerco     se lo trovo ora non sto facendo nulla
			if((foundresult=found(&addr,0)) > -1){
				srand(time(NULL));
				printf("la foundresult e di %d\n\n",foundresult);
				syn_rcvd[foundresult]=addr; //se non lo trovo lo aggiungo
				ausiliarSyn_rcvd[foundresult] = true;
				pacchetto.seqnumb = r; //assegno un numero casuale come numero sequenza
				pacchetto.acknumb = pacrcv.seqnumb + 1; //richiedo il byte successivo
				pacchetto.flags.headlen = 10;		//**************cambiare size of
				pacchetto.flags.ack = 1;
				pacchetto.flags.rst = 0;
				pacchetto.flags.syn = 1;
				pacchetto.flags.fin = 0;
				printf("il pacchetto da mandare la prima volta:\n %d \n %d \n %d \n %d\n %s ",pacchetto.seqnumb,pacchetto.acknumb,pacchetto.flags.ack,pacchetto.flags.syn,pacchetto.data);

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
					printf("fatto il found and destroy\n");
					buf[0].sem_num = 0;
					buf[0].sem_op = 1;
					buf[0].sem_flg = 0;
					semop(semID, buf, 1);
					((int32_t *)arg)[0] = pacrcv.seqnumb;
					((int32_t *)arg)[1] = pacrcv.acknumb;
					printf("arg0 %d, aarg1%d \n\n",((int32_t *)arg)[0], ((int32_t *)arg)[1]);
					printf("\n\n********************dato il token del semaforo nell mylisten***************\n\n");
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
			printf("porta: %d\nIP: %d \n ",accepted[i].sin_port, accepted[i].sin_addr.s_addr);
			
		
		}
		i = 0;
		printf("\n\ncontenuto della lista delle connessioni in attesa di syn ack: \n");		
		for(i=0;i<N;i++){
			printf("porta: %d\nIP: %d\n ",syn_rcvd[i].sin_port, syn_rcvd[i].sin_addr.s_addr);
			
		
		}
		printf("\n\ncontenuto delle posizioni dell arrey delle accettate: \n");		
		for(i=0;i<N;i++){
			printf("%d ",ausiliarAccepted[i]);
			
		
		}	
		printf("\n\ncontenuto delle posizioni dell arrey delle syn_rcvd: \n");		
		for(i=0;i<N;i++){
			printf("%d ",ausiliarSyn_rcvd[i]);
			
		
		}
		printf("\n\n");
	}

}



int myaccept(int sockfd, struct sockaddr_in *addr, socklen_t *addrlen){
	
	struct sockaddr_in addrpers;
	memset((void *)&addrpers, 0, sizeof(addrpers));

	int i = 0, IDsock;
	struct sembuf buf[1];
	printf("\n\n********************in attesa del token del semaforo nell accept***************\n\n");
	buf[0].sem_num = 0;
	buf[0].sem_op = -1;
	buf[0].sem_flg = 0;
	semop(semID, buf, 1);
	printf("\n\n********************preso il token del semaforo nell accept***************\n\n");
	while(ausiliarAccepted[i] == false){
		printf("in while per trovare la posizione\n\n");
		i++;
	}
	printf("l aposizione del vero sta nella posizione %d\n\n\n",i);
	printf("\n\n********************trovato la connessione in posizione: %d***************\n\n",i);
	addr->sin_family = accepted[i].sin_family;
	addr->sin_port = accepted[i].sin_port;
	addr->sin_addr.s_addr = accepted[i].sin_addr.s_addr;
	*addrlen = sizeof(accepted[i]);
	memset((void *)&accepted[i], 0, sizeof(struct sockaddr_in));
	ausiliarAccepted[i] = false;
	if ((IDsock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		perror("errore in socket");
		exit(1); 
	}
	int reu =1;


	if (setsockopt(IDsock, SOL_SOCKET, SO_REUSEADDR, &reu, sizeof(int)) < 0)
		 perror("setsockopt(SO_REUSEADDR) failed");
	  /* assegna l'indirizzo al socket */
	addrpers.sin_family = AF_INET;
	addrpers.sin_addr.s_addr = htonl(INADDR_ANY); /* il server accetta pacchetti su una qualunque delle sue interfacce di rete */
	addrpers.sin_port = htons(SERV_PORT); /* numero di porta del server */
	if (bind(IDsock, (struct sockaddr *)&addrpers, sizeof(addrpers)) < 0) {
		perror("errore in bind");
		exit(1);
	}
	return IDsock;
	
}


void shutout(int sig){
	
	printf("\n\n********************entrato nel gestore del segnale SIGINT***************\n\n");
	semctl(semID, -1, IPC_RMID, 0);
	exit(1);
	
}

int reorder(packetsend array[], int size){
	
  //it's a bubblesort
  
  packetsend temp;
  // loop to access each array element
  for (int step = 0; step < size - 1; ++step) {
      
    // loop to compare array elements
		for (int i = 0; i < size - step - 1; ++i) {
		  
		  // compare two adjacent elements
		  // change > to < to sort in descending order
		  if (array[i].seqnumb > array[i + 1].seqnumb) {
			
			// swapping occurs if elements
			// are not in the intended order
			memcpy((void *)&temp,(void *)&array[i],sizeof(packetsend));
			memcpy((void *)&array[i],(void *)&array[i + 1],sizeof(packetsend));
			memcpy((void *)&array[i + 1],(void *)&temp,sizeof(packetsend));

			}
		}
    }
	return 0;
}



void printArray(packetsend array[], int size) {
  for (int i = 0; i < size; ++i) {
    printf("in posizione %d\n seqnumb = %d\n acknumb = %d\n overwritable = %d\n position = %d\n", i,  array[i].seqnumb, array[i].acknumb, array[i].overwritable,array[i].position);
	printf("\n\n");
  }
  printf("\n");
}



int strswitch(char *str){
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
	printf("dentro la funzione compare la i e %d",i);
	return i;
}



void makePacket(packet *p,int32_t sn,int32_t an, int  flg){
	
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














