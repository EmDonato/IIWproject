#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>


#define MAXLINE 128

typedef struct packet{
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

void receive(int sockfd, struct sockaddr_in addr){
  socklen_t addr_size;
  struct packet * temp = malloc(sizeof(struct packet));
  while(1){

    addr_size = sizeof(addr);
    recvfrom(sockfd, temp, sizeof(packet), 0, (struct sockaddr*)&addr, &addr_size);
    printf("~ %s \n",  temp->data);
  }
  return;
}

int main(){

  char *ip = "127.0.0.1";
  int port = 8080;

  int server_sockfd;
  struct sockaddr_in server_addr, client_addr;

  server_sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = port;
  server_addr.sin_addr.s_addr = inet_addr(ip);

  bind(server_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));

  printf("UDP Server. \n");
  receive(server_sockfd, client_addr);

  close(server_sockfd);

  return 0;
}