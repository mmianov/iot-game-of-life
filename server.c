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
#define MAX_NODES 2

int server_socket;

struct sockaddr_in node_addr;
struct sockaddr_in nodes[MAX_NODES];
int addr_len = sizeof(struct sockaddr_in);

char received_message[MAX_MSG_SIZE];
char register_message[MAX_MSG_SIZE];

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
    if(bind(*server_socket,(struct sockaddr*)&server_addr, sizeof(server_addr)) != 0){
        perror("Bind:");
        return -1;
    }

    printf("Server initialized on %s:%d\n",inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
    return 1;
}

// receives data on server socket
int receive_data(char *message){
    // clear buffer
    memset(message, 0, sizeof(message));
    struct sockaddr_in node_addr;

    // receive incoming data
    int received_data = recvfrom(server_socket, message, MAX_MSG_SIZE, 0, (struct sockaddr*)&node_addr, &addr_len);
    if (received_data > 0){
	return 1;
    }
    else{
        return -1; 
    }
}

// clears message buffer
void clear_msg_buffer(char *message){
    memset(message, 0, sizeof(message));
}

// opens registration for nodes
int open_registration(){
    printf("[*] Node registration is now open ...\n\r");
    int num_of_nodes = 0;
    fd_set read_fds;
    fd_set write_fds;

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec =500000;

    for(;;){
        // reset file descriptor sets
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        // add server socket file descriptor to fd read set and stdin to fd write set
        FD_SET(server_socket,&read_fds);
        FD_SET(STDIN_FILENO, &write_fds);
        // wait for server socket fds to accept new connection or wait for keyboard input
        select(server_socket+1, &read_fds, &write_fds, NULL, NULL); // no timeout

        // check for new connection to server
            if(FD_ISSET(server_socket, &read_fds)) {
                    // receive message and save node address to node_addr
                    recvfrom(server_socket, register_message, MAX_MSG_SIZE, 0, (struct sockaddr*)&node_addr, &addr_len);
                    nodes[num_of_nodes] = node_addr;
                    printf("Node number &d connected: %s\n\r",num_of_nodes+1,inet_ntoa(node_addr.sin_addr));
                    num_of_nodes ++;
                    // reset node_addr placeholder
                    memset(&node_addr, 0, sizeof(node_addr));
                }
            // check for user input to stop registration
            if(FD_ISSET(STDIN_FILENO, &write_fds)){
                char user_input[4];
                read(STDIN_FILENO, user_input, sizeof(user_input));
                printf("Wpisano: %s\r",user_input);
                if(!strcmp(user_input,"stop")){
                    printf("[*]Node registration finished\r");
                    break;
                }
		        //memset(&user_input,0,sizeof(user_input));
            }

            if(num_of_nodes >=MAX_NODES){
                printf("[*]Maximum node amount reached!\n\r");
                break;
            }
    }

    return 1;
}

int main(){
    initialize_server(&server_socket, SERVER_PORT);
    open_registration();
//   for(;;){
//     receive_data(received_message);
//     printf("Received message: %s",received_message);
//
//    }


}
