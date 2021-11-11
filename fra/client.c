#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>


#define MAXLINE 128 

#define BUFFER_SIZE 20
#define W 5

char *buffer;

sem_t s_write[BUFFER_SIZE],s_send[BUFFER_SIZE];
pthread_t tid_write, tid_send;

typedef struct packet
{
  int32_t seqnumb;
  int32_t acknumb;
  struct flags{
      uint8_t headlen:4;
      uint8_t ack:1;
      uint8_t rst:1;
      uint8_t syn:1;
      uint8_t fin:1;
      }flags;
  char data[MAXLINE];
}packet;

struct send_args
{
  int sockfd;
  struct sockaddr_in addr;
};

void *thread_write(void *file_pointer)
{
  
  int i, i_mod;
  int size;
  int n_pkts;
  int totBytes = MAXLINE;

  FILE *fp = (FILE *)file_pointer;
  // Calcola la dimensione del file
  fseek(fp, 0L, SEEK_END);
  size = (int) ftell(fp);
  fseek(fp, 0L, SEEK_SET);
  // Calcola quanti blocchi serviranno
  n_pkts = size % MAXLINE ? size/MAXLINE + 1 : size/MAXLINE;
  printf("%d\n", n_pkts);

  // Popola i pacchetti
  for ( i = 0; i < n_pkts; i++)
  {     
    i_mod = (i != 0)? i%(BUFFER_SIZE) : i;
    sem_wait(&s_write[i_mod]);
    fseek(fp,MAXLINE*i,SEEK_SET);   
    fread(&buffer[i_mod*MAXLINE],totBytes,1,fp);
    sem_post(&s_send[i_mod]);
  }
  
}

void *thread_send(void *void_args)
{
  
  struct send_args *args = (struct send_args *) void_args;
  int i = 0;
  packet pack;
  int sockfd = args -> sockfd;
  struct sockaddr_in addr = args -> addr;

  while(1)
  { 
    sem_wait(&s_send[i]);
    pack.seqnumb = (int32_t) i;
    memcpy((void *)pack.data, (void *) &buffer[i*MAXLINE], MAXLINE*sizeof(char));
    sendto(sockfd,  (const void *)&pack, sizeof(packet), 0, (struct sockaddr*) &addr, sizeof(addr));

    sem_post(&s_write[i]);
    printf("\nsend : %d \n",i);
    i = (i+1)%BUFFER_SIZE;
  } 
}


int main()
{
  FILE *fp;

  char *filename = "file.txt";

  char *ip = "127.0.0.1";
  int port = 8080;

  int server_sockfd;
  struct sockaddr_in server_addr;

  server_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = port;
  server_addr.sin_addr.s_addr = inet_addr(ip);

  struct send_args *args = (struct send_args *) malloc(sizeof(struct send_args));
  args -> sockfd = server_sockfd;
  args -> addr = server_addr; 

  buffer = (char*) malloc(BUFFER_SIZE*MAXLINE*sizeof(char));

  fp = fopen(filename, "r");

  for (int i = 0; i < BUFFER_SIZE; i++)
  {
    sem_init(&s_write[i], 0, 1);
    sem_init(&s_send[i], 0, 0);
  }

  printf("UDP client. \n");
  pthread_create(&tid_write, NULL, thread_write, (void *) fp);
  pthread_create(&tid_send, NULL, thread_send, (void *) args);

  pthread_join(tid_write, NULL);
  pthread_join(tid_send, NULL);

  free(buffer);
  free(args);
  return 0;
}
