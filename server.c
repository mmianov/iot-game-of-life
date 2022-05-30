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
#define GAME_STATE_REGISTER_CONFIRM 0<<0 | 0<<1 | 0<<2 | 1<<3

int server_socket;

struct sockaddr_in node_addr; // variable to perform operations on nodes during registration
struct sockaddr_in nodes[MAX_NODES]; // variable to hold nodes in registration period
int addr_len = sizeof(struct sockaddr_in);

char received_message[MAX_MSG_SIZE]; // received message
char register_message[MAX_MSG_SIZE]; // register message (to be merge into ALP later on)

// structure to hold information about nodes in the game
struct game_node{
    int id; // node identifier
    struct in_addr IP_addr; // node ip address
    int neighbours[MAX_NODES-1]; // neighbouring nodes
    int cols; // columns in game of life 2D array
    int rows; // rows in game of life 2D array
    int sent_area_update; // value to see if node has already sent an area update
};

int game_nodes_amount; // variable to store game nodes

// --- GAME NODES FUNCTIONS ---

// creates game nodes
void create_game_nodes(struct game_node *game_nodes, int game_nodes_amount){
    for(int i=0;i<game_nodes_amount;i++){
        // check struct sockaddr_in nodes[MAX_NODES] and only get registered nodes
        game_nodes[i].id = i+1;
        game_nodes[i].IP_addr = nodes[i].sin_addr;
        memset(game_nodes[i].neighbours,0,sizeof(game_nodes[i].neighbours));
        game_nodes[i].cols = 0;
        game_nodes[i].rows = 0;
        game_nodes[i].sent_area_update = 0;
    }
}

void display_game_nodes(struct game_node *game_nodes,int game_nodes_amount){
    for(int i=0;i<game_nodes_amount;i++){
        printf("---------------------------------\n\r");
        printf("Node id: %d\n",game_nodes[i].id);
        printf("Node IP: %s\n",inet_ntoa(game_nodes[i].IP_addr));
        // neighbours
        printf("Cols: %d Rows: %d\n",game_nodes[i].cols,game_nodes[i].rows);
        printf("Area update status: %d\n",game_nodes[i].sent_area_update);
        printf("---------------------------------\n\r");
    }
}

// --- SERVER FUNCTIONS ---

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
struct sockaddr_in receive_data(char *message){
    // clear buffer
    memset(message, 0, sizeof(message));
    // receive incoming data
    //printf("receive_data() function\n");
    int received_data = recvfrom(server_socket, message, MAX_MSG_SIZE, 0, (struct sockaddr*)&node_addr, &addr_len);
    return node_addr;
}


int handle_node_area_update(){
    printf("handle node area udpate");
}

int handle_boundary_update(){
    printf("handle boundary udpate");
}

int handle_message(char *message){

   // TODO : Maybe change to only 2 first bits and payload?
   // check codename and argument bits (intentionally verbose, to see the ALP)
   int first_bit = (message[0] >> 0) & 1;
   int second_bit = (message[0] >> 1) & 1;
   int third_bit = (message[0] >> 2) & 1;
   int fourth_bit = (message[0] >> 3) & 1;

   if (first_bit == 0 && second_bit == 0 && third_bit == 0 && fourth_bit == 1){
        //printf("Debug: received register message\n");
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
        if(node.sin_addr.s_addr == nodes[i].sin_addr.s_addr){
            printf("Debug: node %s already registered!\n",inet_ntoa(node.sin_addr));
            return 1;
        }
    }
    //printf("Debug: node is not registered!\n");
    return 0;
}


// opens registration for nodes
int open_registration(){
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

    for(;;){
        // reset file descriptor sets and add server socket to watch list
        FD_ZERO(&read_fds);
        FD_SET(server_socket,&read_fds);
        select(server_socket+1, &read_fds, NULL, NULL,NULL);

        // check for new connection to server
        if(FD_ISSET(server_socket, &read_fds)) {
            // receive message
            memset(&node_addr,0,sizeof(node_addr));
            node_addr = receive_data(register_message);

            if(handle_message(register_message) == GAME_STATE_REGISTER && !is_registered(node_addr,nodes_to_connect)){
                nodes[num_of_nodes] = node_addr;
                num_of_nodes ++;
                printf("Node %d/%d connected: %s\n\r",num_of_nodes,nodes_to_connect,inet_ntoa(node_addr.sin_addr));
                memset(&register_message,0,sizeof(register_message));
                register_message[0] = GAME_STATE_REGISTER_CONFIRM;
                sendto(server_socket, register_message, strlen(register_message), 0, (struct sockaddr *)&node_addr, addr_len);
            }
        }
            if(num_of_nodes >=nodes_to_connect){
                printf("[*]All nodes connected! Closing registration ...\n\r");
                break;
            }
    }
    // set global variable
    game_nodes_amount = nodes_to_connect; // todo: maybe change to global variable only?
    return 1;
}

// --- GAME OF LIFE FUNCTIONS ---

void fill2DArray(int *array,int rows, int cols){
    srand(time(0));
    int num_of_ones = 0;
    int num_of_zeros = 0;
    for(int i =0;i<rows;i++){
        for(int j=0;j<cols;j++){
            int rand_val = rand()%2;
            if(rand_val == 1 && num_of_ones*5 < num_of_zeros){
                          *((array+i*rows)+j) = rand_val;
                          num_of_ones++;
                        }
            else{
                 *((array+i*rows)+j) = 0;
                  num_of_zeros++;
            }
        }
    }
}

void visualise_2Darray(int *array,int rows, int cols){
    for(int i =0;i<rows;i++){
        for(int j=0;j<cols;j++){
            if(*((array + i*rows)+j) == 0){
                printf("-");
            }
            else{
                printf("*");
            }

        }
        printf("\n");
    }
}

int countNeighbours(int *array,int rows, int cols, int x, int y){
    int sum = 0;

    for(int i =-1;i<2;i++){
        for(int j=-1;j<2;j++){
            // count live neighbours
            int wrap_cols = (y+j+cols) % cols;
            int wrap_rows = (x+i+rows) % rows;
            //sum = sum + *((array + rows*(x+i))+y+j);
            sum = sum + *((array + rows*wrap_rows)+wrap_cols);
        }
    }
    sum = sum - *((array + x*rows)+y);
    return sum;
}



void compute_game_of_life(int *arr,int *new_arr,int rows, int cols){

    // iterate over array
     for(int i =0;i<rows;i++){
        for(int j=0;j<cols;j++){
            // get current cell state (state before computing next generation)
            int state = *((arr+i*rows)+j);
            int one = 1;
            int zero = 0;

             // count cell neighbours
            int neighbours = countNeighbours((int*)arr,rows,cols,i,j);

            if (state == 0 && neighbours == 3){
                 memcpy(((new_arr+i*rows)+j),&one,sizeof(int));
            }
            else if (state == 1 && (neighbours < 2 || neighbours >3)){
                memcpy(((new_arr+i*rows)+j),&zero,sizeof(int));
            }
            else{
                memcpy(((new_arr+i*rows)+j),&state,sizeof(int));
            }

        }
    }
}



int main(){
    int rows = 40;
    int cols = 80;
    int arr[rows][cols];
    memset(arr,0,rows*cols*sizeof(int));
    fill2DArray((int*)arr,rows,cols);
    //printf("Original array: \n");
    visualise_2Darray((int*)arr,rows,cols);
    sleep(1);

    int next_gen[rows][cols];
    memcpy(next_gen,arr,rows*cols*sizeof(int));

    for(;;){
//        // copy contents of current generation
//        memcpy(next_gen,arr,rows*cols*sizeof(int));
        // compute next generation and save in next_gen array
        compute_game_of_life((int*)arr,(int*)next_gen,rows,cols);
        visualise_2Darray((int*)next_gen,rows,cols);
        sleep(1);
        system("clear");
        // set next generation to be current generation in next loop
        memcpy(arr,next_gen,rows*cols*sizeof(int));
    }

//    // server setup
//    initialize_server(&server_socket, SERVER_PORT);
//    open_registration();
//
//    // store game nodes
//    struct game_node game_nodes[game_nodes_amount];
//    memset(game_nodes,0,sizeof(game_nodes));
//
//    // initialize game nodes
//    create_game_nodes(game_nodes,game_nodes_amount);
//    display_game_nodes(game_nodes,game_nodes_amount);
//
//    close(server_socket);


}
