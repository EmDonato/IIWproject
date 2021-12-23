#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <pthread.h>
#include <stdbool.h>
#include <signal.h>
#include <semaphore.h>

#include<fcntl.h>
#include<sys/sem.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<pthread.h>


#define MAXLINE 128

char *filename = "picture_server.jpeg";

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


void receive(int sockfd, struct sockaddr_in addr, int fd){
    socklen_t addr_size;
    int r;
    struct packet * temp = malloc(sizeof(struct packet));
    while(1){
      addr_size = sizeof(addr);
      memset((void *)&temp->data,0,MAXLINE*sizeof(char));
      recvfrom(sockfd, temp, sizeof(packet), 0, (struct sockaddr*)&addr, &addr_size);
      printf("%d\n",  temp->seqnumb);

      write(fd,(void *)temp->data,temp->data_size*sizeof(char));
      fflush(stdout);
      fsync(fd);
      temp->acknumb = temp->seqnumb+1;
      r=sendto(sockfd, (const void *)temp, sizeof(*temp), 0, (const struct sockaddr *) &addr, sizeof(addr));
      //printf("r = %d\n",r);
      if (temp->last == true)
      {
        break;
      }
    }
    close(fd);
}

int main(){

  char *ip = "127.0.0.1";
  int port = 8080;
  
  int fd = open(filename,O_CREAT|O_TRUNC|O_RDWR,0666);

  int server_sockfd;
  struct sockaddr_in server_addr; //client_addr;
  memset(&server_addr, 0, sizeof(server_addr));

  server_sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = inet_addr(ip); //INADDR_ANY; //inet_addr(ip);

  bind(server_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));

  printf("Server up\n");
  receive(server_sockfd, server_addr,fd);

  close(server_sockfd);

  return 0;
}