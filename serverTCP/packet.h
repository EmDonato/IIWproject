#define BACKLOG 10
#define MAXSIZE 1024 
#define N 10
#define PROB 0.01

//header file contenente le tracce delle funzioni e le strutture dati

typedef struct packet 
{
	// struttura dati del pacchetto
	
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
	
	// struttura dati che tiene traccia dei pacchetti spediti
	
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

int  printfList(char *);

int myaccept(int sockfd, struct sockaddr_in *addr, socklen_t *addrlen);

void *mylisten(void *arg);

int foundAndDetroy(struct sockaddr_in *addr,int x);

int found(struct sockaddr_in *addr,int x);

int rcv_prob(int socketID, struct sockaddr_in *add,int fileID,int32_t ackn,int32_t seqn,float prob);

int reorder(packetsend array[], int size);

void createFirstPac(packet *,int32_t,int32_t,char *);

int sendRequest(int ,struct sockaddr_in *,packet *);

int sendFile(int , int32_t , int32_t , int , struct sockaddr_in,float );


