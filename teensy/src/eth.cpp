
#include <NativeEthernet.h>
#include <NativeEthernetUdp.h>

#include "eth.h"
#include "teetools.h"
#include "rbuf.h"
#include "xstdef.h"
#include "cmdhandler.h"
#include "xmroundbuf.h"
#include "led_strip.h"

/// @brief  ethernet connection status
enum EthStatus {
	sEthInit,
	sEthError,
	sEthNoLink,
	sEthGood
};

EthStatus ethStatus = sEthInit;
EthernetLinkStatus linkStatus = Unknown; ///< this is from NativeEthernet.h
unsigned int aliveTime = 0;
static char macs[24];

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
//byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
const IPAddress ip(192, 168, 0, 177); ///< this is teensy IP address

const unsigned int localPortUdp = 8888; // local port to listen on
const unsigned int localPortTcp = 8889; // local port to listen on

// buffers for receiving and sending data
static const int maxPacketSize = 512;
char packetBuffer[maxPacketSize]; // buffer to hold incoming packet,
//char ReplyBuffer[] = "acknowledged\r\n";	   // a string to send back

// this is a buffer for sending info to the other side
const int rbSize = 1024;
unsigned char buf[rbSize];
static ByteRoundBuf rb;

static bool clientConnected = false;

/// @brief this is a buffer for writing
const int bufSize2 = 256;
unsigned char buf2[bufSize2];

// An EthernetUDP instance to let us send and receive packets over UDP
//EthernetUDP udp;
static EthernetServer server;
static EthernetClient client;
EthInfoHandler infoHandler = 0;  ///< for log file upload

unsigned int lastIncomingInfoTime = 0;

#pragma pack(1)
struct PingStruct {
	char hdr[4];
	unsigned int time;
	unsigned char id;
	unsigned char reserved[3];
};
#pragma pack()
PingStruct pingStruct;
XMRoundBuf<PingStruct, 8> pings; ///< times when we actually send the ping to the client

void printPings() {
	unsigned int now = millis();
	const int bs = 256;
	char b[bs];
	char* s = b;
	for (int i = 0; i < pings.num; i++) {
		s += snprintf(s, b + bs - s, "(%u, %.3f) ", pings[i].id, pings[i].time / 1000.0);
	}
	xmprintf(1, "now = %u; pings<%d> = [%s]  %u\r\n", now, pings.num, b, now);
}


void teensyMAC(uint8_t *mac);

void ethPrint() {
	xmprintf(3, "ETH status=%d; clientConnected=%s;  client=%s   \r\n", static_cast<int>(ethStatus),
		clientConnected ? "yes" : "no", client ? "yes" : "no");
}
void ethSetInfoHandler(EthInfoHandler h) {
	infoHandler = h;
}

void ethSetup() {
	// start the Ethernet
	xmprintf(1, "eth starting ....  ");
	clientConnected = false;
	uint8_t mac[6];
	teensyMAC(mac);
	xmprintf(17, "mac %s  ", macs);
	Ethernet.begin(mac, ip);
	xmprintf(17, " .. begin .. ");

	// Check for Ethernet hardware present
	if (Ethernet.hardwareStatus() == EthernetNoHardware) 	{
		ethStatus = sEthError;
		xmprintf(17, "EthernetNoHardware \r\n");
		return;
	}
	linkStatus = Ethernet.linkStatus();
	switch (linkStatus)  {
	case LinkOFF:
	case Unknown:
		xmprintf(17, "  sEthNoLink   ");
		ethStatus = sEthNoLink;
		break;
	}; 
		
	
	// start UDP
	//udp.begin(localPort);
	// start the server

  	server.begin(localPortTcp);
	ethStatus = sEthGood;
	xmprintf(17, "eth started 1 \r\n");
	initByteRoundBuf(&rb, buf, rbSize);
	pings.clear();
	pingStruct.hdr[0] = 'p'; pingStruct.hdr[1] = 'i'; pingStruct.hdr[2] = 'n'; pingStruct.hdr[3] = 'g'; 
	pingStruct.id = 0; pingStruct.time = millis(); pingStruct.reserved[0] = 0;

	client = server.available();
	ledstripMode(lsEyeConnection, 200, 0x00000008);
	lastIncomingInfoTime = 0;
	xmprintf(1, "eth started 2; client=%s \r\n", client ? "yes" : "no");
}

void ethFeed(char* s, int size) {
	bool irq = disableInterrupts();
	if (!clientConnected) {
		enableInterrupts(irq);
		return;
	}
	put_rb_s(&rb, s, size);
	enableInterrupts(irq);
	//xmprintf(2, "ethFeed: + %d bytes \r\n", size);
}

void ethLoop(unsigned int now) {
	if ((now - lastIncomingInfoTime) > 1250) {
		//xmprintf(1, "ethLoop() disconnected?")
		ledstripMode(lsEyeConnection, 200, 0x00a000a0);
	}

	EthernetLinkStatus currentLinkStatus = Ethernet.linkStatus();
	if (currentLinkStatus != linkStatus) {
		linkStatus = currentLinkStatus;
		pings.clear();
		xmprintf(1, "ETH link status %d \r\n", linkStatus);
	}
	if ((linkStatus == LinkOFF) || (linkStatus == Unknown)) {
		ethStatus = sEthNoLink;
		if (clientConnected) {
			clientConnected = false;
			xmprintf(1, "ETH client disconnected 3;  linkStatus = %d\r\n", linkStatus);
			ledstripMode(lsEyeConnection, 200, 0x00000095);
		}
		return;
	}

	//return;

	if (!client) {
		client = server.available();
	}
	if (!client) {
		if (clientConnected) {
			clientConnected = false;
			xmprintf(1, "ETH client disconnected 2 \r\n");
			ledstripMode(lsEyeConnection, 200, 0x00000095);
		}
		return;
	}

	//return;

	bool con = client.connected();
	if (clientConnected != con) {
		clientConnected  = con;
		if (clientConnected) {
			bool irq = disableInterrupts();
			resetByteRoundBuf(&rb);
			enableInterrupts(irq);
			pings.clear();
			xmprintf(1, "ETH client connected \r\n");
		} else {
			xmprintf(1, "ETH client disconnected \r\n");
			//client.close();
			client.stop();
			if (infoHandler) {
				infoHandler(gfFinish, 0); //   stop file transmission if client disconnects
			}
			//printPings();
			return;
		}
	}
	if (!clientConnected) {
		return;
	}

	// handle the incoming information
	int bs, bs1;
	while ((bs = client.available()) > 0) {
		bs1 = client.read(packetBuffer, maxPacketSize-2);
		mxat(bs1 < maxPacketSize);
		packetBuffer[maxPacketSize-1] = 0;
		packetBuffer[bs1] = 0;
		
		//xmprintf(2, "ETH: got %d bytes {%s} \r\n", bs1, packetBuffer);
		if ((bs1 > 0)) {
			lastIncomingInfoTime = now;
			ledstripMode(lsEyeConnection, 200, 0x0000a000);
		 	if (strncmp(packetBuffer, "console", 7) == 0 ) {  //  console client connected
				client.write("tee ", 4);
			} else if (strncmp(packetBuffer, "ping", 4) == 0 ) {
				
			} else 	if (infoHandler) { 
				unsigned int pn;
				if ((bs1 >= 16) && (memcmp(packetBuffer, "TBWF", 4) == 0)) { // teensy, please send next
					memcpy(&pn, packetBuffer + 8, 4);
					infoHandler(gfPleaseSendNext, pn);
				} else if ((bs1 >= 16) && (memcmp(packetBuffer, "TBWN", 4) == 0)) {  // teensy, please repeat 
					memcpy(&pn, packetBuffer + 8, 4);
					infoHandler(gfPleaseRepeat, pn);
				} else if ((bs1 >= 16) && (memcmp(packetBuffer, "TEEE", 4) == 0)) {  // teensy, end of the transmission
					infoHandler(gfFinish, 0);
				} else if (bs1 > 0) {
					packetBuffer[maxPacketSize-1] = 0;	
					xmprintf(2, "ETH: got %s\r\n", packetBuffer);
				}
			} else { // just normal command
				int bs2 = (bs1 >= maxPacketSize) ? maxPacketSize-1 : bs1;
				packetBuffer[bs2] = 0;
				processTheCommand(packetBuffer, bs2); // process the command
			}
		}
	}

	// if we have some info in 'rb' then lets send it to the other side
	bool irq = disableInterrupts();
	int num = rb.num;
	enableInterrupts(irq);
	while (num > 0) {
		irq = disableInterrupts();
		int bs = get_rb_s(&rb, buf2, bufSize2-1);
		num = rb.num;
		enableInterrupts(irq);
		if (bs > 0) {
			int u = client.availableForWrite();
			buf2[bs] = 0; // !
			buf2[bufSize2-1] = 0; // ?
			int test = client.write(buf2, bs);
			
			//xmprintf(2, "\r\nETH availableForWrite=%d; test = %d; bs=%d *sending* {%s} \r\n", u, test, bs, buf2);
		}
	}


	/*
	// if there's data available, read a packet
	int packetSize = udp.parsePacket();
	if (packetSize) {
		IPAddress ra = udp.remoteIP();
		uint16_t rp = udp.remotePort();
		// read the packet into packetBufffer
		int bs = udp.read(packetBuffer, maxUdpSize);
		if (bs >= maxUdpSize) {
			bs = maxUdpSize-1;
		}
		packetBuffer[bs] = 0;  //  put zero AFTER the message
		xmprintf(2, "ETH got %d bytes from %d.%d.%d.%d port %d (%s)\r\n",
			bs,
			ra[0], 	ra[1], ra[2], ra[3],
			rp,
			packetBuffer
		);

		// send a reply to the IP address and port that sent us the packet we received
		udp.beginPacket(ra, rp);
		udp.write(ReplyBuffer);
		udp.endPacket();

		if ((bs > 0) && (packetBuffer[0] == '@')) {
			processTheCommand(packetBuffer + 1); // process the command
		}
		

		
  	} 
	*/  
	unsigned int ms = millis();
	if (clientConnected && ((ms - aliveTime) > 245) && (!infoHandler)) {
		aliveTime = ms;
		//udp.beginPacket("192.168.0.181", 8888);
		//udp.write("alive\r\n");
		//udp.endPacket();
		//if (clientConnected) {
		pingStruct.id += 1;
		pingStruct.time = ms;
		client.write((const char *)&pingStruct, sizeof(pingStruct));

		//client.write("ping\n", 6);
		//}
		pings.put(pingStruct);
	}
}

void teensyMAC(uint8_t *mac) {
	

	//Serial.println("using HW_OCOTP_MAC* - see https://forum.pjrc.com/threads/57595-Serial-amp-MAC-Address-Teensy-4-0");
	for(uint8_t by=0; by<2; by++) mac[by]=(HW_OCOTP_MAC1 >> ((1-by)*8)) & 0xFF;
	for(uint8_t by=0; by<4; by++) mac[by+2]=(HW_OCOTP_MAC0 >> ((3-by)*8)) & 0xFF;
		
	snprintf(macs, 24, "MAC: %02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	//Serial.println(teensyMac);
}
