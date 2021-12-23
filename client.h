
#define MAXLINE 128     // dimensione campo data.
#define BUFFER_SIZE 20  // dimensione del buffer.
#define W 5             // dimensione finestra di invio.
#define T 3            // tempo in secondi per il timer.
#define PORT 8080

int seqn = 0;
int ackn = 0;


struct sockaddr_in servaddr;

pthread_t tid_write, tid_send, tid_ack;

int sockfd;

char *buffer;

bool timer = false;

int32_t ack_atteso = 1;

sem_t sem_S[BUFFER_SIZE], sem_R[BUFFER_SIZE], sem_T;

FILE *fp;

int fd;

int n_pkts;

int red_bytes[BUFFER_SIZE] = {0};

typedef struct packet
{
  int32_t seqnumb;
  int32_t acknumb;
  bool last;
  int data_size;
  struct flags{
      uint8_t headlen:4;
      uint8_t ack:1;
      uint8_t rst:1;
      uint8_t syn:1;
      uint8_t fin:1;
      }flags;
  char data[MAXLINE];
}packet;

typedef struct packetsend{
  int32_t seqnumb;
  int32_t acknumb;
  bool overwritable;
  int position;
  bool last;
  int data_size;
}packetsend;

// Crea tante strutture di controllo quanti sono i posti nella window.
packetsend ctrl[W];

int reorder(packetsend array[], int size){
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
  int i, r, i_mod, size, totBytes = MAXLINE;

  // Calcola la dimensione del file
  size = (int) lseek(fd, 0L, SEEK_END);

  lseek(fd, 0L, SEEK_SET);
  // Calcola quanti blocchi serviranno
  n_pkts = size % MAXLINE ? size/MAXLINE + 1 : size/MAXLINE;
  
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
  int j, i = 0;
  packet pack;
  
  // loop di invio. 
  while( 1)
  {
    printf("sending pkt %d\n", seqn);
    // wait <t, 1>
    sem_wait(&sem_T);
    // wait <r[i], 1>
    sem_wait(&sem_R[i]);

    // compila il pacchetto.
    pack.seqnumb = (int32_t) seqn;
    memset((void*   )pack.data, 0, MAXLINE*sizeof(char));
    memcpy((void *)pack.data, (void *) &buffer[i*MAXLINE], MAXLINE*sizeof(char));
    pack.data_size = red_bytes[i];
    if ((int)seqn == n_pkts-1)
    {
      pack.last = true;
    }
    else
    {
      pack.last = false;
    }

    // compila struttura di controllo.
    for( j = 0; j < W; j++)
    {
      if( ctrl[j].overwritable)
      {
        ctrl[j].acknumb = seqn + 1;
        ctrl[j].seqnumb = seqn;
        ctrl[j].position = i;
        ctrl[j].overwritable = false;
        ctrl[j].last = pack.last;
        ctrl[j].data_size = pack.data_size;
        break;
      }
    }
    i = (i + 1) % BUFFER_SIZE;
    //printf("%s",  pack.data);

    sendto(sockfd, (const void *)&pack, sizeof(packet), 0, (struct sockaddr*) &servaddr, sizeof(servaddr));
    
    seqn++;
    if( !timer)
    {
      // imposta un timer.
      alarm(T);
      timer = true;
    }    
    if (pack.last == true)
    {
      close(fd);
      exit(0);
    }
  }
}


void *thread_ack()
{
  struct packet *temp = malloc(sizeof(struct packet));
  socklen_t addr_size = sizeof( struct sockaddr_in);
  int32_t ack;
  int i, aux = 0;
  while(1)
  {

    recvfrom(sockfd, temp, sizeof(packet), 0, (struct sockaddr*)&servaddr, &addr_size); //va definito bene sockfd e servaddr
    ack = temp -> acknumb;

    if (ack == ack_atteso)
    {
      for ( i = 0; i < W; i++)
      {
        if ((ctrl[i].seqnumb + 1) == ack)
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
      ack_atteso++;
    }

    //Non testato
    if (ack > ack_atteso)
    {
      reorder(ctrl, W);
      for ( i = 0; i < W; i++)
      {
        if (ctrl[i].seqnumb + 1 <= ack)
        {
          ctrl[i].overwritable = true;
          aux++;
        }
      }
      for ( i = 0; i < aux; i++){
        sem_post(&sem_T);
      }
      for ( i = 0; i < aux; i++){
        sem_post(&sem_S[ctrl[i].position]);
      }
      aux = 0;
    }
  } 
}

// Funzione per gestione alarm.
void alarm_handler( int signum)
{
  printf("alarm\n");
  // Ordina array di packetsend. Funzione Ema.
  printf("%d\n",reorder( ctrl, W));
  // Spedisci i pacchetti in ordine (+ timer dopo il primo).
  packet pack;
  int r;
  char foo[MAXLINE];
  for (int i = 0; i < W; i++)
  {
    pack.seqnumb = ctrl[i].seqnumb;
    pack.acknumb = ctrl[i].acknumb;
    pack.last = ctrl[i].last;
    pack.data_size = ctrl[i].data_size;
    memcpy((void *)pack.data, (void *) &buffer[(ctrl[i].position)*MAXLINE], MAXLINE*sizeof(char));
    //printf("%d %s\n",ctrl[i].position, (char *)pack.data);
    if(sendto(sockfd, (const void *)&pack, sizeof(packet), 0, (struct sockaddr*) &servaddr, sizeof(servaddr))==-1)printf("ERRORE\n");
  }
}


void TCPfunction(int given_fd, int32_t seqnumb, int32_t acknumb, int given_sockfd, struct sockaddr_in given_servaddr)
{

  fd = given_fd;
  seqn = seqnumb;
  ack_atteso = seqnumb + 1;
  ackn = acknumb;
  buffer = (char*) malloc(BUFFER_SIZE*MAXLINE*sizeof(char));

  // Gestione alarm.
  signal( SIGALRM, alarm_handler);

  // Inizializza semafori.
  for (int i = 0; i < BUFFER_SIZE; i++)
  {
    sem_init(&sem_S[i], 0, 1);
    sem_init(&sem_R[i], 0, 0);
  }
  sem_init(&sem_T, 0, W);

/*
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(PORT);
  servaddr.sin_addr.s_addr = INADDR_ANY;
*/

  //sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  servaddr = given_servaddr;
  sockfd = given_sockfd;

  for (int i = 0; i < W; i++)
  {
    ctrl[i].overwritable = true;
  }

  pthread_create(&tid_write, NULL, thread_write, NULL);
  pthread_create(&tid_send, NULL, thread_send, NULL);
  pthread_create(&tid_ack, NULL, thread_ack, NULL);

  pthread_join(tid_write, NULL);
  pthread_join(tid_send, NULL);
  pthread_join(tid_ack, NULL);

}