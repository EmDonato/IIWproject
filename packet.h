#define MAXLINE     1024



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