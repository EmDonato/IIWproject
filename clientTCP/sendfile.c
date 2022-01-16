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
#include <time.h>


#define MAXLINE 1024     // dimensione campo data.
#define BUFFER_SIZE 20  // dimensione del buffer.
#define W 5             // dimensione finestra di invio.
#define T 1            // tempo in secondi per il timer.


int seqn = 0;
int ackn = 0;
struct sockaddr_in servaddr;
pthread_t tid_write, tid_send, tid_ack;
int sockfd1;
char *buffer;
bool timer = false;
int32_t ack_atteso = 1;
sem_t sem_S[BUFFER_SIZE], sem_R[BUFFER_SIZE], sem_T, sem_ctrl;
FILE *fp;
int fd;
int n_pkts = 0;
int last_seqn;
int red_bytes[BUFFER_SIZE] = {0};
float p;

// Crea tante strutture di controllo quanti sono i posti nella window.
packetsend ctrl[W];




void alarm_handler( int signum)
{
//funizione per la gestione del timeout, rinvio dei pacchetti spediti


  // Ordina array di packetsend. Funzione Ema.
  reorder( ctrl, W);
  // Spedisci i pacchetti in ordine (+ timer dopo il primo).
  packet pack;
  int r;
  char foo[MAXLINE];
  alarm(T);
  for (int i = 0; i < W; i++)
  {
    if(ctrl[i].overwritable == false)
    {
      pack.seqnumb = ctrl[i].seqnumb;
      pack.acknumb = ctrl[i].acknumb;
      pack.last = ctrl[i].last;
      pack.data_size = ctrl[i].data_size;
      memcpy((void *)pack.data, (void *) &buffer[(ctrl[i].position)*MAXLINE], MAXLINE*sizeof(char));
      if(sendto(sockfd1, (const void *)&pack, sizeof(packet), 0, (struct sockaddr*) &servaddr, sizeof(servaddr))==-1)printf("ERRORE\n");
    }
  }
} 


int reorder(packetsend array[], int size){
	// funzione per il riordino dell array deipacchetti mandati 
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

// Funzioni di utilizzo dei thread.
void *thread_write()
{
	// thread che legge dal file 
  int i, r, i_mod, size, totBytes = MAXLINE;

  // Calcola la dimensione del file
  size = (int) lseek(fd, 0L, SEEK_END);

  lseek(fd, 0L, SEEK_SET);
  // Calcola quanti blocchi serviranno
  n_pkts = size % MAXLINE ? size/MAXLINE + 1 : size/MAXLINE;
  if(n_pkts == 0)
	  n_pkts = 1;
  
    last_seqn = seqn + (n_pkts-1)*(MAXLINE+1); //CHIEDERE CONFERMA

  // Popola i pacchetti
  for ( i = 0; i < n_pkts; i++)
  {    
	
    i_mod = (i != 0)? i%(BUFFER_SIZE) : i;
    // wait <s[i], 1>
    sem_wait(&sem_S[i_mod]);
    memset((void *)&buffer[i_mod*MAXLINE], 0, MAXLINE*sizeof(char));
    red_bytes[i_mod]=read(fd,(void *)&buffer[i_mod*MAXLINE],MAXLINE);
    // signal <r[i], 1>
    sem_post(&sem_R[i_mod]);
  } 
}

void *thread_send()
{ 
// thread che invia i pacchetti
  int j, i = 0;
  packet pack;
  
  // loop di invio. 
  while( 1)
  {
    // wait <t, 1>
    sem_wait(&sem_T);
    // wait <r[i], 1>
    sem_wait(&sem_R[i]);

    // compila il pacchetto.
    pack.seqnumb = (int32_t) seqn;
    pack.acknumb = (int32_t) ackn;
    memset((void*   )pack.data, 0, MAXLINE*sizeof(char));
    memcpy((void *)pack.data, (void *) &buffer[i*MAXLINE], MAXLINE*sizeof(char));
    pack.data_size = red_bytes[i];
    if ((int)seqn == last_seqn)
    {
      pack.last = true;
    }
    else
    {
      pack.last = false;
    }

    // compila struttura di controllo.
	sem_wait(&sem_ctrl);
    for( j = 0; j < W; j++)
    {
      if( ctrl[j].overwritable)
      {
        ctrl[j].acknumb = ackn;
        ctrl[j].seqnumb = seqn;
        ctrl[j].position = i;
        ctrl[j].overwritable = false;
        ctrl[j].last = pack.last;
        ctrl[j].data_size = pack.data_size;
        break;
      }
    }

	sem_post(&sem_ctrl);
    seqn = seqn + 1 + red_bytes[i];

    i = (i + 1) % BUFFER_SIZE;

    sendto(sockfd1, (const void *)&pack, sizeof(packet), 0, (struct sockaddr*) &servaddr, sizeof(servaddr));
    if( !timer)
    {
      // imposta un timer.
      alarm(T);
      timer = true;
    }

    if (pack.last == true)
    {
      break;
    }
  }
}


void *thread_ack()
{
	//thread per la ricezione degli ack
struct packet *temp = malloc(sizeof(struct packet));
socklen_t addr_size = sizeof( struct sockaddr_in);
int32_t ack;
int i, aux = 0;
int numb;
double probaus;
srand (time(NULL)); 
while(1){
    numb = rand();
    probaus = (double)numb /(double)RAND_MAX; 
		recvfrom(sockfd1, temp, sizeof(packet), 0, (struct sockaddr*)&servaddr, &addr_size);
		if(probaus<=p){
      
    }
    else
    {
      ack = temp -> acknumb;
  		sem_wait(&sem_ctrl);
  		reorder(ctrl, W);
  		for(int i = 0; i <= W; i++){
  			if((ctrl[i].overwritable) == false)
  			{
  				ack_atteso = ctrl[i].seqnumb+ctrl[i].data_size+1;
  				break;
  			}
  		}
  		sem_post(&sem_ctrl);		
  		if (ack == ack_atteso)
  		{
  			sem_wait(&sem_ctrl);
  			for ( i = 0; i < W; i++){
  				if ((ctrl[i].seqnumb + ctrl[i].data_size + 1) == ack)
  				{
  				  ctrl[i].overwritable = true;
  				  sem_post(&sem_S[ctrl[i].position]);
  				  break;
  				}
  				if(i==W-1) 
  				{  
  				  printf("ERROR\n");
  				  exit(-1);
  				}
  			}
  			sem_post(&sem_T);
  			sem_post(&sem_ctrl);

        for (int i = 0; i < W; i++)
        {
          if ((ctrl[i].seqnumb > ack) && (ctrl[i].overwritable == false))
          {
            alarm(T);
            timer = true;
            break;
          }
        }
  		}


      if(ack > ack_atteso)
      {
        sem_wait(&sem_ctrl);
        reorder(ctrl, W);
        for (int i = 0; i < W; i++)
        {
          if ((ctrl[i].seqnumb < ack) && (ctrl[i].overwritable == false))
          {
            ctrl[i].overwritable = true;
            sem_post(&sem_S[ctrl[i].position]);
            sem_post(&sem_T);
          }
          else if ((ctrl[i].seqnumb > ack) && (ctrl[i].overwritable == false))
          {
            aux = 1;
          }
        }
        sem_post(&sem_ctrl);
        if (aux == 1)
        {
          alarm(T);
          timer = true;
          aux = 0;
        }
      }

      if (temp -> last == 1){
            alarm(0);
            timer = false;
            break;
      }
    }
	} 
}


int sendFile(int given_fd, int32_t seqnumb, int32_t acknumb, int given_sockfd, struct sockaddr_in given_servaddr, float prob)
{
// thread principale della funzione per l invio dei file
  p = prob;
  fd = given_fd;
  seqn = seqnumb;
  ack_atteso = seqnumb + MAXLINE + 1;
  ackn = acknumb;
  buffer = (char*) malloc(BUFFER_SIZE*MAXLINE*sizeof(char));
  
  struct sigaction act;
    sigset_t set ;
    sigfillset(&set);
    memset((void *)&act,0,sizeof(struct sigaction));

    if((sigprocmask(SIG_BLOCK,&set,NULL))==-1){
        printf("errore nel settare i segnali");
        exit(1);
    }

    act.sa_mask=set;
    act.sa_handler=alarm_handler;
    if((sigaction(SIGALRM,&act,NULL))==-1){
        printf("errore nel settare i segnali");
        exit(1);
    }
    if(sigemptyset(&set)) exit(1);
    if(sigaddset(&set,SIGALRM)) exit(1);
    if(sigaddset(&set,SIGINT)) exit(1);


    if((sigprocmask(SIG_UNBLOCK,&set,NULL))==-1){
        printf("errore nel settare i segnali\n");
        exit(1);
    }

  // Inizializza semafori.
  for (int i = 0; i < BUFFER_SIZE; i++)
  {
    sem_init(&sem_S[i], 0, 1);
    sem_init(&sem_R[i], 0, 0);
  }
  sem_init(&sem_T, 0, W);
  sem_init(&sem_ctrl, 0, 1);


  servaddr = given_servaddr;
  sockfd1 = given_sockfd;

  for (int i = 0; i < W; i++)
  {
    ctrl[i].overwritable = true;
  }


  pthread_create(&tid_write, NULL, thread_write, NULL);
  pthread_create(&tid_send, NULL, thread_send, NULL);
  pthread_create(&tid_ack, NULL, thread_ack, NULL);

  pthread_join(tid_write, NULL);
  pthread_join(tid_ack, NULL);
  pthread_join(tid_send, NULL);

  return(0);

}

