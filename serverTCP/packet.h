#define BACKLOG 10
#define MAXSIZE 1024 
#define N 10
#define NEWSERV_PORT 519


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
  char data[MAXSIZE];
}packet;

typedef struct packetsend{
  int32_t seqnumb;
  int32_t acknumb;
  bool overwritable;
  int position;
  bool last;
  int data_size;
}packetsend;

int ls(FILE *f);



typedef struct gate{
	
	int sockID;
	int address;
	
}gate;

void makePacket(packet *,int32_t ,int32_t , int );

int strswitch(char *str);

void shutout(int sig);


int myaccept(int sockfd, struct sockaddr_in *addr, socklen_t *addrlen);


void *mylisten(void *arg);


int foundAndDetroy(struct sockaddr_in *addr,int x);


int found(struct sockaddr_in *addr,int x);


int reorder(packetsend array[], int size);

void printArray(packetsend array[], int size);


void createFirstPac(packet *,int32_t,int32_t,char *);


int sendRequest(int ,struct sockaddr_in *,packet *);

int sendFile(int , int32_t , int32_t , int , struct sockaddr_in );

int rcv(int , struct sockaddr_in * ,int ,int32_t ,int32_t );
