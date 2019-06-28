#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<string.h>
#include<stdlib.h>
#include<sys/utsname.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<pthread.h>

#define SERVER_PORT 7904
#define MAX_LINE 256
#define MAXNAME 30

/* structure of the packet */
struct packet {
short type;
char mNAME[MAXNAME];
char uNAME[MAXNAME];
char data[MAX_LINE];
int chatroom;
};

int global_s;

void *listener(){
	struct packet packet_rec;

	while(1){
		recv(global_s, &packet_rec, sizeof(packet_rec),0);
		puts(packet_rec.uNAME);
		fputs(": ", stdout);
                puts(packet_rec.data);
        }		
}


int main(int argc, char **argv) {
	struct packet packet_confirm;
	struct packet packet_send;
	struct sockaddr_in sin;
	char buf[MAX_LINE];	
	int s;
	int len;
	pthread_t thread0;

	/* translate host name into peer's IP address */
	if (argc < 2) {
		printf("Usage: %s hostname", argv[0]);
		exit(-1);
	}

	struct hostent *hp = gethostbyname(argv[1]);

	if (hp == NULL) {
		printf("gethostbyname() failed\n");
	}
	

	/* active open */
	if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0){
		perror("tcpclient: socket");
		exit(1);
	}

	/* build address data structure */
	bzero((char*)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
	sin.sin_port = htons(SERVER_PORT);

	
	if(connect(s,(struct sockaddr *)&sin, sizeof(sin)) < 0){
		perror("tcpclient: connect");
		close(s);
		exit(1);
	}

	/* Build and send reg packet */
	printf("Welcome to our chat client! \n");
	printf("\n Input your user name: \n");
	fgets(packet_send.uNAME, MAXNAME, stdin);

	printf("\n Input your chat room number: \n");
	scanf("%d", &packet_send.chatroom);

	packet_send.type = htons(121);
	gethostname(packet_send.mNAME, MAXNAME);
	if(send(s, &packet_send, sizeof(packet_send), 0) < 0){
		printf("\n Could not send Registration Packet. \n");
		exit(1);
	}
	
	/* Recieve confirmation packet */
        if(recv(s,&packet_confirm,sizeof(packet_confirm),0) < 0) {
		printf("\n Could not receive confirmation packet \n");
                exit(1);
        }
	else 
	printf("\n Registration confirmation recieved! \n");
		
	/* Creation of listening thread */
	global_s = s;
	pthread_create(&thread0, NULL, listener, NULL);	

	/* Send chat messages to server */
	packet_send.type = htons(619);
	while(1)
	{
		fgets(packet_send.data, MAXNAME, stdin);
		send(s, &packet_send, sizeof(packet_send), 0);		 
	}
}
