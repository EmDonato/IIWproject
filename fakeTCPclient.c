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

#include "packet.h"



#define SERV_PORT   5193 
#define MAXLINE     1024
#define CLOSED 0
#define SYN_SEND 1
#define ESTABLISHED 2




volatile int state=CLOSED;
int sockfdglb;
pthread_t  tidglb;




void *makeconnection(void *arg){
	
	
	printf("dentro la funione della connessione\n"); 
  //  un puntatore
	struct sockaddr_in servaddrrcv;
	packet pac,pacrcv;
	srand(time(NULL));
	int n,len =sizeof(struct sockaddr_in);
	int32_t r = rand(); //numero della sequenza
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
		
		n = recvfrom(sockfdglb,(void *)&pacrcv, sizeof(packet), 0 ,(struct sockaddr *) &servaddrrcv,&len ); // ************checkare se viene dallo stesso indirizzo ip********
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
	//metto la sveglia
	
	
	while(state!=ESTABLISHED){
		alarm(3);
		printf("dentro il while della richiesta\n");
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
	
  int   sockfd, n;
  void *status;
  pthread_t tid;
  struct    sockaddr_in   servaddr;

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

  printf("la connessione e stabilita: %d\n",state);




/*  
  if (sendto(sockfd, NULL, 0, 0, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
    perror("errore in sendto");
    exit(1);
  }

  n = recvfrom(sockfd, recvline, MAXLINE, 0 , NULL, NULL);
  if (n < 0) {
    perror("errore in recvfrom");
    exit(1);
  }
  if (n > 0) {
    recvline[n] = 0;        
    if (fputs(recvline, stdout) == EOF)   {  
      fprintf(stderr, "errore in fputs");
      exit(1);
    }
  }
  exit(0); */
 
}
