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
#define AREA_UPDATE 2
#define GAME_STATE_REGISTER_CONFIRM 0<<0 | 0<<1 | 0<<2 | 1<<3
#define BOUNDARY_UPDATE_CODE  1<<0 | 1<<1 | 1<<2 | 1<<3
#define AREA_UPDATE_CODE 1<<3 | 0<<2 | 1<<1 | 0<< 0


int server_socket;

struct sockaddr_in node_addr; // variable to perform operations on nodes during registration
struct sockaddr_in nodes[MAX_NODES]; // variable to hold nodes in registration period
int addr_len = sizeof(struct sockaddr_in);

char received_message[MAX_MSG_SIZE]; // received message
char register_message[MAX_MSG_SIZE]; // register message (to be merged into ALP later on)


// structure to hold information about nodes in the game
struct game_node{
    int id; // node identifier
    struct sockaddr_in net_addr; // node ip address
    int cols; // columns in game of life 2D array
    int rows; // rows in game of life 2D array
    int ** area;
    int sent_area_update; // value to see if node has already sent an area update
};

int game_nodes_amount = 4;
const int map_rows = 6;
const int map_cols = 8;
int area_rows = map_rows/2;
int area_cols = map_cols/2;
int node_area_rows = map_rows/2 + 2; // 2 additional rows for top and bottom bordering areas //
int node_area_cols = map_cols/2 + 2; // 2 additional cols for left and right bordering areas //

char protocol_message[5];
int map[6][8];

// --- GAME NODES FUNCTIONS ---

void visualise_2DarrayNumbers(int *array,int rows, int cols){
    for(int i =0;i<rows;i++){
        for(int j=0;j<cols;j++){
           if(*((array + i*cols)+j) == 0){ //zmieniono na i*cols zamiast i*rows
                printf("0");
            }
            else{
                printf("1");
            }

        }
        printf("\n");
    }
}

// creates game nodes
void create_game_nodes(struct game_node *game_nodes, int game_nodes_amount){
    for(int i=0;i<game_nodes_amount;i++){
        // check struct sockaddr_in nodes[MAX_NODES] and only get registered nodes
        game_nodes[i].id = i+1;
        game_nodes[i].net_addr = nodes[i];
        game_nodes[i].rows = node_area_rows;
        game_nodes[i].cols = node_area_cols;
        game_nodes[i].area = malloc(node_area_rows*sizeof(int*));
        for(int j=0;j<node_area_rows;j++){
             game_nodes[i].area[j] = malloc(node_area_cols*sizeof(int));
        }
        game_nodes[i].sent_area_update = 0;
    }
}

void display_game_nodes(struct game_node *game_nodes,int game_nodes_amount){
    for(int i=0;i<game_nodes_amount;i++){
        printf("---------------------------------\n\r");
        printf("Node id: %d\n",game_nodes[i].id);
        printf("Node IP: %s\n",inet_ntoa(game_nodes[i].net_addr.sin_addr));
        printf("Cols: %d Rows: %d (frame included)\n",game_nodes[i].cols,game_nodes[i].rows);
        printf("Node area with frame:\n");
        visualise_2DarrayNumbers((int*)game_nodes[i].area,game_nodes[i].rows,game_nodes[i].cols);
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

int handle_message(char *message){

   // TODO : Maybe change to only 2 first bits and payload?
   // check codename and argument bits (intentionally verbose, to see the ALP)
   int first_bit = (message[0] >> 0) & 1;
   int second_bit = (message[0] >> 1) & 1;
   int third_bit = (message[0] >> 2) & 1;
   int fourth_bit = (message[0] >> 3) & 1;

   if (first_bit == 0 && second_bit == 0 && third_bit == 0 && fourth_bit == 1){
        return GAME_STATE_REGISTER;
   }
   else if (first_bit == 1 && second_bit == 0 && third_bit == 1 && fourth_bit== 0){
        return AREA_UPDATE;
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
            if(rand_val == 1 && num_of_ones*2 < num_of_zeros){
                          *((array+i*cols)+j) = rand_val; // zmieniono na i*cols zamiast i*rows
                          num_of_ones++;
                        }
            else{
                 *((array+i*cols)+j) = 0; // zmieniono na i*cols zamiast i*rows
                  num_of_zeros++;
            }
        }
    }
}

void visualise_2Darray(int *array,int rows, int cols){
    for(int i =0;i<rows;i++){
        for(int j=0;j<cols;j++){
            if(*((array + i*cols)+j) == 0){ //zmieniono na i*cols zamiast i*rows
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
	        //int wrap_cols = (y+j+cols) % cols;
            //int wrap_rows = (x+i+rows) % rows;
            // not sure which one to go with yet
            int wrap_j = (y+j+cols) % (cols);
            int wrap_i = (x+i+rows) % (rows);
            //sum = sum + *((array + rows*(x+i))+y+j);
            sum = sum + *((array + cols*wrap_i)+wrap_j); // zmieniono na cols*wrap_rows
        }
    }
    sum = sum - *((array + x*cols)+y); // zmieniono na x*cols + y
    return sum;
}


// divides map according to amount of nodes in game and possible map size
void divide_map(int *area1,int *area2,int *area3,int *area4,int initial){

    //int map[map_rows][map_cols]; // changed to global
    //memset(map,0,sizeof(map));
    if(initial){
        fill2DArray((int*)map,map_rows,map_cols);
    }

    //printf("Original map: \n");
    //visualise_2DarrayNumbers((int*)map,map_rows,map_cols);
    //sleep(2);

    int temp_area[node_area_rows][node_area_cols]; // area for calculations

    // define adjusting cols and rows for each area
    // [n - area][1 - rows, 0 - cols]
    int area_adjust[4][2];

    area_adjust[0][1] = 0;
    area_adjust[0][0] = 0;
    area_adjust[1][1] = (map_rows/2) + (node_area_rows>node_area_cols ? -1 : 1);
    area_adjust[1][0] = 0;
    area_adjust[2][1] = 0;
    area_adjust[2][0] = (map_cols/2) + (node_area_rows>node_area_cols ? 1 : -1);
    area_adjust[3][1] = (map_rows/2) + (node_area_rows>node_area_cols ? -1 : 1);
    area_adjust[3][0] = (map_cols/2) + (node_area_rows>node_area_cols ? 1 : -1);

    for(int n=0;n<game_nodes_amount;n++){
        for(int i=0;i<node_area_rows;i++){
            for(int j=0;j<node_area_cols;j++){
                // area surrounding 'frame' conditions
                if(i == 0 || i == node_area_rows-1 || j == 0 || j == node_area_cols -1){
                    int i_mapped = (i+area_adjust[n][0]-1)<0 ? i+area_adjust[n][0]-1+map_rows : (i+area_adjust[n][0]-1)%map_rows;
                    int j_mapped = (j+area_adjust[n][1]-1)<0 ? j+area_adjust[n][1]-1+map_cols : (j+area_adjust[n][1]-1)%map_cols;
                    temp_area[i][j] = map[i_mapped][j_mapped];
                }
                else{
                    temp_area[i][j] = map[i+area_adjust[n][0]-1][j+area_adjust[n][1]-1];
                }
            }
        }
        if(n == 0) memcpy(area1,temp_area,sizeof(temp_area));
        if (n == 1) memcpy(area2,temp_area,sizeof(temp_area));
        if (n == 2) memcpy(area3,temp_area,sizeof(temp_area));
        if (n == 3) memcpy(area4,temp_area,sizeof(temp_area));

        memset(temp_area,0,sizeof(temp_area));
    }
}


void reassemble_map(int*new_map,int *area1,int *area2,int *area3,int *area4){

    // map_rows - wiersze mapy
    // node_area_rows - wiersze z ramka
    // area_rows - bez ramki

    // DLA WSZYSTKICH MUSI I ZACZYNAC SIE OD ZERA - DOCHODZI DO DRUGIEGO NODA ALE NIE BIERZE GO OD 0 TYLKO OD 0 = 5 NP.
    for(int i=0;i<map_rows;i++){
        for(int j=0;j<map_cols;j++){
            if(i < map_rows/2 && j < map_cols/2){
                *((new_map+i*map_cols)+j) = *((area1+i*area_cols)+j);
            }
            else if(i < map_rows/2 && j >= map_cols/2){
                *((new_map+i*map_cols)+j) = *((area2+i*area_cols)+(j- map_cols/2));
            }
            else if(i >= map_rows/2 && j < map_cols/2){
                *((new_map+i*map_cols)+j) = *((area3+(i-map_rows/2)*area_cols)+j);
            }
            else if(i >= map_rows/2 && j >= map_cols/2){
                *((new_map+i*map_cols)+j) = *((area4+(i-map_rows/2)*area_cols)+(j - map_cols/2));
            }
        }
    }
}


void trim_area(int*frame_area,int *area){
    for(int i=0;i<area_rows;i++){
        for(int j=0;j<area_cols;j++){
            *((area+i*area_cols)+j) = *((frame_area+(i+1)*node_area_cols)+(j+1));
    }
}
}



void compute_game_of_life(int *arr,int *new_arr,int rows, int cols){

    // iterate over array
     for(int i =0;i<rows;i++){
        for(int j=0;j<cols;j++){
            // get current cell state (state before computing next generation)
            int state = *((arr+i*cols)+j); // zmieniono na i*cols
            int one = 1;
            int zero = 0;

             // count cell neighbours
            int neighbours = countNeighbours((int*)arr,rows,cols,i,j);

            if (state == 0 && neighbours == 3){
                 memcpy(((new_arr+i*cols)+j),&one,sizeof(int)); // zmieniono na i*cols
            }
            else if (state == 1 && (neighbours < 2 || neighbours >3)){
                memcpy(((new_arr+i*cols)+j),&zero,sizeof(int)); // zmieniono na i*cols
            }
            else{
                memcpy(((new_arr+i*cols)+j),&state,sizeof(int)); // zmieniono na i*cols
            }

        }
    }
}


int write_to_buffer(int *area, int rows, int cols){
    protocol_message[0] = BOUNDARY_UPDATE_CODE;
    int bytes = 1;
    int shift = 0;
    for(int i=0;i<rows;i++){
        for(int j=0;j<cols;j++){
            if (shift==8){
                shift = 0;
                bytes++;
            }
            protocol_message[bytes] = protocol_message[bytes] | (*((area + i*cols)+j) << shift);
            shift++;
        }
    }
    return bytes;
}

int receive_from_buffer(int *area, int rows, int cols){

    int shift = 0;
    int bytes = 1;
    for(int i=0;i<rows;i++){
        for(int j=0;j<cols;j++){
           if(shift == 8){
              shift = 0;
              bytes++;
           }
           int bit = (protocol_message[bytes] >> shift)&1;
          *((area+i*cols)+j) = (protocol_message[bytes] >> shift) & 1;
           shift++;
        }
    }

}

int main(){
//    int rows = 40;
//    int cols = 90;
//    int arr[rows][cols];
//    memset(arr,0,rows*cols*sizeof(int));
//    fill2DArray((int*)arr,rows,cols);
//    //printf("Original array: \n");
//    visualise_2Darray((int*)arr,rows,cols);
//    sleep(1);
//    system("clear");
//
//    int next_gen[rows][cols];
//    memcpy(next_gen,arr,rows*cols*sizeof(int));
//
//    for(;;){
////        // copy contents of current generation
////        memcpy(next_gen,arr,rows*cols*sizeof(int));
//        // compute next generation and save in next_gen array
//        compute_game_of_life((int*)arr,(int*)next_gen,rows,cols);
//        visualise_2Darray((int*)next_gen,rows,cols);
//        sleep(1);
//        system("clear");
//        // set next generation to be current generation in next loop
//        memcpy(arr,next_gen,rows*cols*sizeof(int));
//    }

    // server setup
    initialize_server(&server_socket, SERVER_PORT);
    open_registration();

    // store game nodes
    struct game_node game_nodes[game_nodes_amount];
    memset(game_nodes,0,sizeof(game_nodes));

    // initialize game nodes
    create_game_nodes(game_nodes,game_nodes_amount);

    // prepare placeholders for areas with frames
    int area1[node_area_rows][node_area_cols];
    int area2[node_area_rows][node_area_cols];
    int area3[node_area_rows][node_area_cols];
    int area4[node_area_rows][node_area_cols];

    int temp_area[node_area_rows][node_area_cols];
    memset(temp_area,0,sizeof(temp_area));

    // for debug purposes
    game_nodes[0].area =(int**)area1;
    game_nodes[1].area =(int**)area2;
    game_nodes[2].area =(int**)area3;
    game_nodes[3].area =(int**)area4;
    //display_game_nodes(game_nodes,game_nodes_amount);

    // prepare placeholders for areas without frame
    int area1_trimmed[area_rows][area_cols];
    memset(area1_trimmed,0,sizeof(area1_trimmed));
    int area2_trimmed[area_rows][area_cols];
    memset(area2_trimmed,0,sizeof(area1_trimmed));
    int area3_trimmed[area_rows][area_cols];
    memset(area3_trimmed,0,sizeof(area1_trimmed));
    int area4_trimmed[area_rows][area_cols];
    memset(area4_trimmed,0,sizeof(area1_trimmed));

    int temp_area_trimmed[area_rows][area_cols];
    memset(temp_area_trimmed,0,sizeof(temp_area_trimmed));

    memset(&protocol_message,0,sizeof(protocol_message));

    // divide the map with initial random values
    divide_map((int*)area1,(int*)area2,(int*)area3,(int*)area4,1);

     //send  values
    write_to_buffer((int*)area1,node_area_rows,node_area_cols);
    printf("Wrote to buffer\n");
    if(sendto(server_socket, protocol_message, strlen(protocol_message), 0, (struct sockaddr *)&game_nodes[0].net_addr, addr_len) == -1){
        printf("ERROR %s (%s:%d) \n", strerror(errno));
    }
    printf("Wyslano area 1 intial");

    memset(&protocol_message,0,sizeof(protocol_message));
    write_to_buffer((int*)area2,node_area_rows,node_area_cols);
    if(sendto(server_socket, protocol_message, strlen(protocol_message), 0, (struct sockaddr *)&game_nodes[1].net_addr, addr_len) == -1){
        printf("ERROR %s (%s:%d) \n", strerror(errno));
    }
    printf("Wyslano area 2 intial");

    memset(&protocol_message,0,sizeof(protocol_message));
    write_to_buffer((int*)area3,node_area_rows,node_area_cols);
    sendto(server_socket, protocol_message, strlen(protocol_message), 0, (struct sockaddr *)&game_nodes[2].net_addr, addr_len);

    printf("Wyslano area 3 intial");
    memset(&protocol_message,0,sizeof(protocol_message));
    write_to_buffer((int*)area4,node_area_rows,node_area_cols);
    sendto(server_socket, protocol_message, strlen(protocol_message), 0, (struct sockaddr *)&game_nodes[3].net_addr, addr_len);
    printf("Wyslano area 4 intial");

    int area1_recv = 0;
    int area2_recv = 0;
    int area3_recv = 0;
    int area4_recv = 0;

    fd_set read_fds;
    printf("Przed for\n");
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1000;


    for(;;){

        //printf("Waiting ...");
        // reset file descriptor sets and add server socket to watch list
        FD_ZERO(&read_fds);
        FD_SET(server_socket,&read_fds);
        select(server_socket+1, &read_fds, NULL, NULL,&timeout);

        // check for new connection to server
        if(FD_ISSET(server_socket, &read_fds)) {
            // receive message
            memset(&node_addr,0,sizeof(node_addr));
            node_addr = receive_data(received_message);

            // find which node connected - node with id 1 will always be responsible for calculating area 1
            int current_node = 0;
             for(int i=0;i<game_nodes_amount;i++){
                if(game_nodes[i].net_addr.sin_addr.s_addr == node_addr.sin_addr.s_addr){
                    current_node = game_nodes[i].id;
                    break;
                }
             }
            // if node sends area update
            if(handle_message(received_message) == AREA_UPDATE){
               // receive untrimmed area
               receive_from_buffer((int*)temp_area,node_area_rows,node_area_cols);
               // trim area
               trim_area((int*)temp_area,(int*)temp_area_trimmed);
               if(current_node == 1){
                  // copy trimmed area one to area1 variable
                  memcpy(area1_trimmed,temp_area_trimmed,sizeof(temp_area_trimmed));
                  // acknowledge that area 1 sent update
                  area1_recv = 1;
                   printf("Received from node 1!\n;");
               }
               else if (current_node == 2){
                  memcpy(area2_trimmed,temp_area_trimmed,sizeof(temp_area_trimmed));
                  area2_recv = 1;
                  printf("Received from node 2!\n;");
               }
               else if (current_node == 3){
                  memcpy(area3_trimmed,temp_area_trimmed,sizeof(temp_area_trimmed));
                  area3_recv = 1;
                  printf("Received from node 3!\n;");
               }
               else if (current_node == 4){
                  memcpy(area4_trimmed,temp_area_trimmed,sizeof(temp_area_trimmed));
                  area4_recv = 1;
                  printf("Received from node 4!\n;");
               }
            }
        }
        // if all areas are calculated (updated)
        if(area1_recv && area2_recv && area3_recv && area4_recv ){

            printf("All areas received!\n;");
            // reassemble map
            int res_map[map_rows][map_cols];
            memset(res_map,0,sizeof(res_map));
            reassemble_map((int*)res_map,(int*)area1_trimmed,(int*)area2_trimmed,(int*)area3_trimmed,(int*)area4_trimmed);
            // copy to global map variable
            memcpy(map,res_map,sizeof(res_map));
            // play animation
            visualise_2DarrayNumbers((int*)res_map,map_rows,map_cols);
            sleep(1);
            system("clear");
            //divide the map again
            divide_map((int*)area1,(int*)area2,(int*)area3,(int*)area4,0);

            //resend areas
            write_to_buffer((int*)area1,node_area_rows,node_area_cols);
            sendto(server_socket, protocol_message, strlen(protocol_message), 0, (struct sockaddr *)&game_nodes[0].net_addr, addr_len);

            write_to_buffer((int*)area2,node_area_rows,node_area_cols);
            sendto(server_socket, protocol_message, strlen(protocol_message), 0, (struct sockaddr *)&game_nodes[1].net_addr, addr_len);

            write_to_buffer((int*)area3,node_area_rows,node_area_cols);
            sendto(server_socket, protocol_message, strlen(protocol_message), 0, (struct sockaddr *)&game_nodes[2].net_addr, addr_len);

            write_to_buffer((int*)area4,node_area_rows,node_area_cols);
            sendto(server_socket, protocol_message, strlen(protocol_message), 0, (struct sockaddr *)&game_nodes[3].net_addr, addr_len);
            printf("Sending new values!");
            // reset flags

            int area1_recv = 0;
            int area2_recv = 0;
            int area3_recv = 0;
            int area4_recv = 0;
        }
    }

    close(server_socket);




    // TODO 1. zmienić area1 na game_node[0].area w write_to_buffer
    // TODO 2. odebrać wiadomość po stronie node'a, wpisać do tablicy, policzyć next_gen, - DONE
    // wpisać bajt po bajcie do tablicy, odesłać
    // TODO 3. odebrać na serwerze tablice od node'a, wpisac wartosci w odpowiednie miejsca na glownej mapie,
    // bez ramki
    // TODO 4. synchronizacja 4 hostów: rozesłać grę, czekać na odpowiedź, wyświetlić mapę, rozesłać nową gre itp.
    // TODO5 ! Dodać obsługę rozłączenia node gdy za długo nie odeśle area
    // TODO6 ! gra w zycie na granicy mapy
    // Kwestia do zastanowienia: potwierdzenie odbioru


}
