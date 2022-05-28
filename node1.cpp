#include <ZsutEthernet.h>
#include <ZsutEthernetUdp.h>
#include <ZsutFeatures.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

#define UDP_REMOTE_PORT 17210
#define LOCAL_PORT 4444

#define MAX_BUFFER 64
#define REQUEST_BUFFER 5

byte mac[] =  {0x01,0x23,0x09,0x67,0x89,0xab};
ZsutEthernetUDP Udp;
ZsutIPAddress serverIP = ZsutIPAddress(192,168,56,103);

unsigned int localPort = LOCAL_PORT;
unsigned int remotePort = UDP_REMOTE_PORT;

unsigned char requestBuffer[REQUEST_BUFFER];
unsigned char packetBuffer[MAX_BUFFER];

int helloReceived = 0;
uint16_t measure;
uint32_t timer;



void millis_delay(unsigned long delay){
	unsigned long timer = ZsutMillis() + delay;
	for(;;){
	   if(timer < ZsutMillis()) break;
	}
}


void setup(){
    Serial.begin(9600);
    ZsutEthernet.begin(mac);
    Serial.println(ZsutEthernet.localIP());
    Udp.begin(localPort);
    Serial.println("Node up and running ... ");

    strcpy(requestBuffer,"test");
}

void loop(){

  	Udp.beginPacket(serverIP, remotePort);
	int w = Udp.write(requestBuffer, REQUEST_BUFFER);
	Udp.endPacket();
	millis_delay(3000);
	Serial.println("Packet sent!");

}
