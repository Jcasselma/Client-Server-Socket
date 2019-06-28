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
#include<time.h>

#define SERVER_PORT 7904
#define MAX_LINE 256
#define MAX_PENDING 10
#define MAXNAME 30

struct packet{
        short type;
        char mName[MAXNAME];
        char uName[MAXNAME];
        char data[MAX_LINE];
        int chatroom;
};

struct registration{
        int port;
        int sockid;
        char mName[MAXNAME];
        char uName[MAXNAME];
	int chatroom;
};

int clientIndex = 0;
struct registration table[10];
struct packet buffer[10];
pthread_mutex_t my_mutex = PTHREAD_MUTEX_INITIALIZER;

void *join_handler(){
	int client = clientIndex - 1;
	struct packet packet_send;
	struct packet packet_recv;
	int sockid = table[client].sockid;
	int sent, bufNum;
	
	strcpy(packet_send.mName, table[client].mName);
	strcpy(packet_send.uName, table[client].uName);
	packet_send.chatroom = table[client].chatroom;
	packet_send.type = htons(149);	
	
        if(send(sockid, &packet_send, sizeof(packet_send), 0) < 0){
                printf("\n Could not send Confirmation Packet \n");
                exit(1);
        }	

	/* Main loop: recieves chats from client, adds them to buffer */
	while(1) {
		if(recv(sockid, &packet_recv, sizeof(packet_recv), 0)<0){
                printf("\n Could not recieve\n");
                exit(1);
        	}
		strcpy(packet_send.data, packet_recv.data);
		sent = 1;
		bufNum = 0;
		pthread_mutex_lock;
		while(sent == 1) {
			if (buffer[bufNum].type == htons(117)) {
				buffer[bufNum].type = htons(149);
				strcpy(buffer[bufNum].mName, packet_send.mName);
				strcpy(buffer[bufNum].uName, packet_send.uName);
				strcpy(buffer[bufNum].data, packet_send.data);
				buffer[bufNum].chatroom = packet_send.chatroom;
				sent = 0;
			}
			else {
				bufNum++;
			}
		}
		pthread_mutex_unlock;
	}
}


void *chat_multicaster(){
        struct packet packet_send;
	int x, i, j;

	
	for (x = 0; x < 10; x++) {
		buffer[x].type = htons(117);
	}
	
	while(1) {
		for (i = 0; i < 10; i++) {
			if (buffer[i].type == htons(149)){
				pthread_mutex_lock;
				strcpy(packet_send.uName, buffer[i].uName);
				strcpy(packet_send.data, buffer[i].data);
				packet_send.chatroom = buffer[i].chatroom;
				buffer[i].type = htons(117);
				pthread_mutex_unlock;
				for (j = 0; j < clientIndex; j++) {
					if (table[j].chatroom == packet_send.chatroom) {
						send(table[j].sockid, &packet_send, sizeof(packet_send), 0);
					}
				}
			}
		}
	}
}


int main(int argc, char* argv[])
{
	struct sockaddr_in sin;
	struct sockaddr_in clientAddr;
	struct packet packet_reg;
	int s, new_s;
	int len;

	//initialization of threads + creation of multicaster
	pthread_t threads[11];
	pthread_create(&threads[0], NULL, chat_multicaster, NULL);	

	/* setup passive open */
	if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0){
		perror("tcpserver: socket");
		exit(1);
	}

	/* build address data structure */
	bzero((char*)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(SERVER_PORT);

	if(bind(s,(struct sockaddr *)&sin, sizeof(sin)) < 0){
		perror("tcpclient: bind");
		exit(1);
	}

	listen(s, MAX_PENDING);

	/* Main loop: accepts new clients, adds them to registration table 
 * 	and creates a join handler thread for each one*/
	while(1){
        	if((new_s = accept(s, (struct sockaddr *)&clientAddr, &len)) < 0){
                	perror("tcpserver: accept");
                	exit(1);
        	}
        	if(recv(new_s, &packet_reg, sizeof(packet_reg), 0) < 0) {
                	printf("\n Could not receive registration packet \n");
                	exit(1);
        	}

		strcpy(table[clientIndex].mName, packet_reg.mName);
        	strcpy(table[clientIndex].uName, packet_reg.uName);
		table[clientIndex].port = clientAddr.sin_port;
		table[clientIndex].sockid = new_s;
		table[clientIndex].chatroom = packet_reg.chatroom;
		clientIndex++;
		pthread_create(&threads[clientIndex], NULL, join_handler, NULL);
	}
}
