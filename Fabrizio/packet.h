#define MAXLINE     1024
#define SERV_PORT 5193
#define BACKLOG 10
#define MAXLINE 1024
#define N 10
#define NEWSERV_PORT 519



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




void shutout(int sig);


int myaccept(int sockfd, struct sockaddr_in *addr, socklen_t *addrlen);


void *mylisten(void *arg);


int foundAndDetroy(struct sockaddr_in *addr,int x);


int found(struct sockaddr_in *addr,int x);