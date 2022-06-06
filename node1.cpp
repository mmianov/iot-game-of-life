#include <ZsutEthernet.h>
#include <ZsutEthernetUdp.h>
#include <ZsutFeatures.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

#define UDP_REMOTE_PORT 17210
#define LOCAL_PORT 4444

#define MAX_BUFFER 64
#define REQUEST_BUFFER 1
#define RECEIVE_BUFFER 10
#define BOUNDARY_UPDATE_CODE  1<<0 | 1<<1 | 1<<2 | 1<<3
#define AREA_UPDATE_CODE 1<<3 | 0<<2 | 1<<1 | 0<<0
// TODO: define protocol bits

byte mac[] =  {0x01,0x43,0x09,0x67,0x89,0xab};
ZsutEthernetUDP Udp;
ZsutIPAddress serverIP = ZsutIPAddress(192,168,56,103);

unsigned int localPort = LOCAL_PORT;
unsigned int remotePort = UDP_REMOTE_PORT;

unsigned char requestBuffer[REQUEST_BUFFER];
unsigned char receiveBuffer[MAX_BUFFER];
unsigned char protocolBuffer[48];

uint16_t measure;
uint32_t timer;
int area[5][6];
int registered = 0;


void millis_delay(unsigned long delay){
	unsigned long timer = ZsutMillis() + delay;
	for(;;){
	   if(timer < ZsutMillis()) break;
	}
}


// register node to the server
int register_node(){
    // 00 11 - register message
    requestBuffer[0] = 0<<0 | 0<<1 | 0<<2 | 1<<3;
    while(1){
        Udp.beginPacket(serverIP,remotePort);
        int reg_node = Udp.write(requestBuffer, REQUEST_BUFFER);
        Udp.endPacket();
        Serial.println("Sending node register request ... ");
        
        millis_delay(100);
        int recv_packet = Udp.parsePacket();
        if(recv_packet){
              int read_packet = Udp.read(receiveBuffer,RECEIVE_BUFFER);
               if(receiveBuffer[0] == requestBuffer[0]){
                    Serial.println("Node registered successfully!");
                    return 1;
                }
             
    }
    millis_delay(500);
  } 
    return 0;
}

// receive boudnary update area from the server and write it to 2D array
int receive_area(){
    int recv_packet = Udp.parsePacket();
    if(recv_packet){
        if(receiveBuffer[0] == BOUNDARY_UPDATE_CODE){
            int read_packet = Udp.read(protocolBuffer,10);
             Serial.print("[*]Received boundary update message!\n");
             int shift = 0;
             int bytes = 1;
             for(int i=0;i<5;i++){
                for(int j=0;j<6;j++){
                   if(shift == 8){
                      shift = 0;
                      bytes++;
                   }
                   int bit = (protocolBuffer[bytes] >> shift)&1;
                   area[i][j] = (protocolBuffer[bytes] >> shift) & 1;
                   shift++;
                }
             }
             return 1;
        }
    }

    return 0;


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


// compute next generation of game of life
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



void visualise_2DarrayNumbers(int *array,int rows, int cols){
    for(int i =0;i<rows;i++){
        for(int j=0;j<cols;j++){
            if(*((array + i*cols)+j) == 0){ //zmieniono na i*cols zamiast i*rows
                Serial.print("0");
            }
            else{
                Serial.print("1");
            }

        }
       Serial.println("\n");
    }
}

int write_to_buffer(int *area, int rows, int cols){
    protocolBuffer[0] = AREA_UPDATE_CODE;
    int bytes = 1;
    int shift = 0;
    for(int i=0;i<rows;i++){
        for(int j=0;j<cols;j++){
            if (shift==8){
                shift = 0;
                bytes++;
            }
            protocolBuffer[bytes] = protocolBuffer[bytes] | (*((area + i*cols)+j) << shift);
            shift++;
        }
    }
    return bytes;
}

void setup(){
    Serial.begin(9600);
    ZsutEthernet.begin(mac);
    Serial.println(ZsutEthernet.localIP());
    Udp.begin(localPort);
    Serial.println("Node up and running ... ");
    memset(requestBuffer,0,sizeof(requestBuffer));
}

void loop(){
    if(!registered){
        if(register_node()){
            registered = 1;
        }
        Serial.println("-------------------------------------")
        Serial.println("[*] The game is about to begin ...")
        Serial.println("-------------------------------------")
    }

    millis_delay(100);

    int received_area = receive_area();
    if(received_area){

        // copy received array to the one that will hold the state of the new generation
        int next_gen_area[5][6];
        memcpy(next_gen_area,area,5*6*sizeof(int));

        // compute next generation
        compute_game_of_life((int*)area,(int*)next_gen_area,5,6);
        Serial.println("[*]Computed game of life");

        // insert into protcol buffer
        memset(protocolBuffer,0,sizeof(protocolBuffer));
        // insert bits into buffer from server.c>
        int written_bytes = write_to_buffer((int*)next_gen_area,5,6);
        Serial.println("[*]Prepared protocl buffer");

        // send to server
        Udp.beginPacket(serverIP,remotePort);
        int area_updated = Udp.write(protocolBuffer, 48);
        Udp.endPacket();
        Serial.println("[*]Sent area update to server");

    }

}
