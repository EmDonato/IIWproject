/* daytime_serverUDP.c - code for example server program that uses UDP 
   Ultima revisione: 14 gennaio 2008 */

#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h>

#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include<stdint.h>
#include"packet.h"
#define SERV_PORT   5193 



int main(int argc, char **argv) {
  int sockfd;
  socklen_t len;
  struct sockaddr_in addr;
  
  time_t ticks;
  packet pacchetto;
  
  

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { /* crea il socket */
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

  while (1) {
    if ( (recvfrom(sockfd,  (void *)&pacchetto, sizeof(packet), 0, (struct sockaddr *)&addr, &len)) < 0) {
      perror("errore in recvfrom");
      exit(1);
    }
	printf("il pacchetto arrivato contiene:\n %d \n %d \n %d \n %d ",pacchetto.seqnumb,pacchetto.acknumb,pacchetto.flags.ack,pacchetto.flags.syn);
	pacchetto.seqnumb=1242;
	pacchetto.acknumb=21513;
	pacchetto.flags.headlen=10;
	pacchetto.flags.ack=1;
	pacchetto.flags.rst=0;
	pacchetto.flags.syn=1;
	pacchetto.flags.fin=0;
    ticks = time(NULL); /* legge l'orario usando la chiamata di sistema time */
    /* scrive in buff l'orario nel formato ottenuto da ctime; snprintf impedisce l'overflow del buffer. */
    snprintf(pacchetto.data, sizeof(pacchetto.data), "%.24s\r\n", ctime(&ticks)); /* ctime trasforma la data e l�ora da binario in ASCII. \r\n per carriage return e line feed*/
    /* scrive sul socket il contenuto di buff */
    if (sendto(sockfd, (void *)&pacchetto, sizeof(packet), 0, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
      perror("errore in sendto");
      exit(1);
    }
  }
  exit(0);
}
