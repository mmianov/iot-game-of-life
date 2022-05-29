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
#define MAX_NODES 8
#define GAME_STATE_REGISTER 1
#define NODE_AREA_UPDATE 2


int server_socket;

struct sockaddr_in node_addr;
struct sockaddr_in register_nodes[MAX_NODES];
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
int receive_data(char *message,struct sockaddr_in *node_addr){
    // clear buffer
    memset(message, 0, sizeof(message));
    //struct sockaddr_in node_addr;

    // receive incoming data
    int received_data = recvfrom(server_socket, message, MAX_MSG_SIZE, 0, (struct sockaddr*)&node_addr, &addr_len);
    if(received_data > 0) return 1;
    return 0;
}


int handle_node_area_update(){
    printf("handle node area udpate");
}

int handle_boundary_update(){
    printf("handle boundary udpate");
}

int handle_message(char *message){

   // TODO : Maybe change to only 2 first bits and payload?
   // check codename and argument bits
   int first_bit = (message[0] >> 0) & 1;
   int second_bit = (message[0] >> 1) & 1;
   int third_bit = (message[0] >> 2) & 1;
   int fourth_bit = (message[0] >> 3) & 1;

   if (first_bit == 0 && second_bit == 0 && third_bit == 0 && fourth_bit == 1){
        printf("Debug: received register message\n");
        return GAME_STATE_REGISTER;
   }
   else if (first_bit == 1 && second_bit == 0 && third_bit == 1 && fourth_bit== 1){
        return NODE_AREA_UPDATE;
   }
   else return -1;
}

int is_registered(struct sockaddr_in node, int nodes_to_connect){
    for(int i=0;i<nodes_to_connect;i++){
        // check if node IP address is already in possible_nodes set
        if(node.sin_addr.s_addr == register_nodes[i].sin_addr.s_addr){
            return 1;
        }
    }
    printf("Debug: node is not registered!\n");
    return 0;
}

// clears message buffer
void clear_msg_buffer(char *message){
    memset(message, 0, sizeof(message));
}

// opens registration for nodes
int server_register_nodes(){
    int num_of_nodes = 0;
    int nodes_to_connect;
    printf("Please specify number of nodes to connect: ");
    scanf("%d",&nodes_to_connect);
    if(nodes_to_connect > MAX_NODES){
        printf("Server can connect up to %d nodes!\n",MAX_NODES);
        return 0;
    }
    printf("[*] Node registration is now open ...\n\r");

    fd_set read_fds;
    fd_set write_fds;

    for(;;){
        // reset file descriptor sets and add server socket to watch list
        FD_ZERO(&read_fds);
        FD_SET(server_socket,&read_fds);
        select(server_socket+1, &read_fds, NULL, NULL,NULL);

        // check for new connection to server
        if(FD_ISSET(server_socket, &read_fds)) {
            // receive message
            memset(&node_addr,0,sizeof(node_addr));
            receive_data(&register_message,&node_addr);
            printf("Debug: new message received");
            if(handle_message(register_message) == GAME_STATE_REGISTER && !is_registered(node_addr,nodes_to_connect)){
                register_nodes[num_of_nodes] = node_addr;
                num_of_nodes ++;
                printf("Node  %d/%d connected: %s\n\r",num_of_nodes,nodes_to_connect,inet_ntoa(node_addr.sin_addr));
            }
        }
            if(num_of_nodes >=nodes_to_connect){
                printf("[*]All nodes connected! Closing registration ...\n\r");
                break;
            }
    }

    return 1;
}

int main(){
    initialize_server(&server_socket, SERVER_PORT);
    server_register_nodes();
//   for(;;){
//     receive_data(received_message);
//     printf("Received message: %s",received_message);
//
//    }
    //printf("%s",inet_ntoa(nodes[0].sin_addr));
    close(server_socket);


}
