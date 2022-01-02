#include <sys/stat.h>
#include <unistd.h>
#include <packet.h>

.
.
.
.
.
.

struct stat st;             
struct sockaddr;






int sockfd;
char filename_rcvd[20];


long int acknum = 0;   




int total_frame = 0 , resend_frame = 0 , drop_frame = 0;
long int i = 0;

int clientfd;



stat(filename_rcvd , &st);
file_size = st.st_size;                   									// recupera dimensione del file da struct stat

file_ptr = fopen(file_name, "rb") ;     									// apertura file ( "rb" per file anche non di testo)

if (( file_size % MAXLINE ) != 0 )
	
	total_frame = (file_size / MAXLINE) + 1 ;
else
	total_frame = (file_size / MAXLINE) ;



printf ( "Numero di pacchetti generati dal file: % d" , total_frame);


length = sizeof(client_address);


.
.
.
.

while ( ack_num != total_frame){                     						// controlla se ack_num coincide con attuale lunghezza del frame , quindi tutti i pacchetti prima inviati sono stati ricevuti
	

	// se non coincidono allora devo reinviare il frame
	sendto(socketfd , &(total_frame), sizeof(total_frame), 0 , (struct sockaddr *)&client_address , sizeof( client_address));
	recvfrom(socketfd, &(ack_num)  , sizeof(ack_num), 0 , (struct sockaddr *)&client_address , (socklent_t *)&length);

	resend_frame++;

}


// Trasmissione sequenziale seguita da ack per ogni frame

for(i = 1 ; i <= total_frame ; i++){


	memset(&pacchetto , 0 , sizeof(pacchetto));
	acknum = 0;
	seqnum = i;

	sendto ( socketfd , &(pacchetto), sizeof(pacchetto), 0 , (struct sockaddr *)&client_address , sizeof( client_address));      // invia il frame
	recvfrom(socketfd, &(ack_num), sizeof(ack_num), 0 , (struct sockaddr *)&client_address , (socketlen_t *)&length);            // riceve l'ack corrispondente

	while (ack_num != pacchetto.acknumb){

			sendto(socketfd , &(pacchetto) , sizeof(pacchetto) , 0 , (struct sockaddr *)&client_address , sizeof( client_address));
			recvfrom(socketfd , &(ack_num), sizeof ( ack_num), 0 , (struct sockaddr *)&client_address, (socklen_t *)&length);
			print(" pacchetto : %ld    scartato %d volte\n", pacchetto.acknumb , ++drop_frame);


			resend_frame ++;



	}

	resend_frame = 0;
	drop_frame = 0 ;

	
}



	
