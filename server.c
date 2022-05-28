#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <time.h>
#include <sys/time.h>

#define MAX_BUFF 128
#define SERVER_PORT 17210



int main(){


	// struct sockaddr_in = trzyma typ IP, adres IP i port połączenia
	struct sockaddr_in serveraddr, clientaddr;
	int server_socket,  clientaddr_len=sizeof(clientaddr);
	int activity;

	unsigned char received_msg[MAX_BUFF];
	unsigned char msg[MAX_BUFF];


	fd_set read_fds;
	memset(&serveraddr, 0, sizeof(serveraddr));
	memset(&clientaddr, 0, sizeof(clientaddr));

    // tworzenie deskryptora serwera UDP
	if((server_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
		printf("ERROR %s (%s:%d) \n", strerror(errno), __FILE__,__LINE__);
	}

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = INADDR_ANY;
	serveraddr.sin_port = htons(SERVER_PORT);

    // przypisanie deskryptora serwera do
	if(bind(server_socket, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) != 0){
		printf("ERROR %s (%s:%d) \n", strerror(errno), __FILE__,__LINE__);
	}

	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 1000;


	for(;;) {

		FD_ZERO(&read_fds);
		FD_SET(server_socket, &read_fds);
		activity = select(server_socket+1, &read_fds, NULL, NULL, &timeout);

        // jeśli nowe połączenie (?)
		if(FD_ISSET(server_socket, &read_fds)) {
			int received_data = recvfrom(server_socket, msg, MAX_BUFF, 0, (struct sockaddr*)&clientaddr, &clientaddr_len);
			if(received_data<0) {
				printf("ERROR %s (%s:%d) \n", strerror(errno));
				exit(-4);
			}
			printf("New node connection: %s\n",msg);
		}
		else{
		    int received_data = recvfrom(server_socket, msg, MAX_BUFF, 0, (struct sockaddr*)&clientaddr, &clientaddr_len);
				msg[received_data]='\0';
				printf("From: %s:%d: %s\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), msg);
		}

    }

	close(server_socket);

}
