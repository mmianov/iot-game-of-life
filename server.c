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

#define SERVER_PORT 17210
#define MAX_MSG_SIZE 50

int server_socket;
int node_addr_len = sizeof(struct sockaddr_in);

// initializes UDP server
int initialize_server(int *server_socket,int port){

    // get file descriptor for server socket
    *server_socket = socket(AF_INET, SOCK_DGRAM,0);
    // check for errors
    if (*server_socket < 0){
        perror("Socket: ");
        return -1;
    }

    // create data structure to hold server parameters
    struct sockaddr_in server_addr;
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // assign the address specified by server_addr to server_socket file descriptor
    if(bind(*server_socket,(struct sockaddr*)&server_addr, sizeof(struct sockaddr)) != 0){
        perror("Bind:");
        return -1;
    }

    printf("Server initialized on %s:%d\n",inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
    return 0;
}

// receives data on server socket
void receive_data(int *server_socket, char *message){
    struct sockaddr_in node_addr;

    // receive incoming data
    int received_data = recvfrom(*server_socket, message, MAX_MSG_SIZE, 0, (struct sockaddr*)&node_addr, &node_addr_len);
}

int main(){
    if (initialize_server(&server_socket, SERVER_PORT) != 0){
        exit(EXIT_FAILURE);
    }

    while(1){
        char received_message[MAX_MSG_SIZE];
        receive_data(&server_socket, received_message);
        printf("Received message: %s", received_message);

    }


}
