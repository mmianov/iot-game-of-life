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
// TODO: define protocol bits

byte mac[] =  {0x01,0x43,0x09,0x67,0x89,0xab};
ZsutEthernetUDP Udp;
ZsutIPAddress serverIP = ZsutIPAddress(192,168,56,103);

unsigned int localPort = LOCAL_PORT;
unsigned int remotePort = UDP_REMOTE_PORT;

unsigned char requestBuffer[REQUEST_BUFFER];
unsigned char receiveBuffer[MAX_BUFFER];
unsigned char protocolBuffer[48];

int helloReceived = 0;
uint16_t measure;
uint32_t timer;
int area[5][6];


void millis_delay(unsigned long delay){
	unsigned long timer = ZsutMillis() + delay;
	for(;;){
	   if(timer < ZsutMillis()) break;
	}
}


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

void setup(){
    Serial.begin(9600);
    ZsutEthernet.begin(mac);
    Serial.println(ZsutEthernet.localIP());
    Udp.begin(localPort);
    Serial.println("Node up and running ... ");
    memset(requestBuffer,0,sizeof(requestBuffer));
}

void loop(){
  
    int registration = register_node();
    millis_delay(100);
    while(registration){
//         Serial.println("Waiting for game to start ...");
//         millis_delay(3000);
        int recv_packet = Udp.parsePacket();
            if(recv_packet){
                  int read_packet = Udp.read(protocolBuffer,10);
                  Serial.print("Code name: ");
                  Serial.println(protocolBuffer[0]);
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

                  for(int i=0;i<5;i++){
                        for(int j=0;j<6;j++){
                           Serial.print(area[i][j]);
                        }
           		Serial.println("");
                  }
//                   if(receiveBuffer[0] == 1<<0 | 1<<1 | 1<<2 | 1<<3){
//                        Serial.println(receiveBuffer);
//                        return 1;
//                    }
            }
    } 

}
