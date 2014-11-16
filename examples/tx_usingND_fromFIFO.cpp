#include <iostream>
#include "xbeeDMapi.h"
#include "TTYserial.h"
#include <mutex>
#include <thread>
#include <vector>
#include <iterator>
#include <string>
#include <cstdint>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <queue>

/*
   Test 3 (27 Auguest, 2014)
   	The purpose of this test is to transfer a video.
*/

std::string modem = "/dev/ttyUSB0";
volatile bool START = false;
volatile bool STOP = false;
volatile bool FAIL = false;
std::queue<uint8_t> vidBuffer;
std::mutex vidBufferMutex;
void readerMain(void);
void w_tty(void);
void w_vidPlayer(std::queue<uint8_t>*, std::mutex*);


int main(int argc, char *argv[])
{
	if (argc == 1) {} //Default is sender, assumed to be running on a laptop.

	else if (argc == 2)
	{
		if (((std::string)argv[1]).length() == 1)
		{
			readerMain();
			return 0;
		}

		else
		{
			modem = (std::string)argv[1];
		}
	}

	else if (argc == 3)
	{
		if (((std::string)argv[1]).length() > 1)
		{
			modem = (std::string)argv[1];
			readerMain();
			return 0;
		}

		if (((std::string)argv[2]).length() > 1)
		{
			modem = (std::string)argv[2];
			readerMain();
			return 0;
		}
	}

	else
	{
		std::cout << "Usage: " << argv[0] << std::endl;
		std::cout << "Usage: " << argv[0] << " [modem]\n";
		std::cout << "Usage: " << argv[0] << " [option]\n";
		std::cout << "Usage: " << argv[0] << " [modem] [option]\n";
		return 0;
	}

	std::thread port(w_tty);
	xbeeDMapi xb;
	START = true;
	
	/////////////////////////////////////////////////////////////////////////////////////////
	// Starting tests.


	//address64 ady(0x0013A20040B39D52);
	//address64 ady(0x0013A20040B8EC7E)
	//address64 ady(0x000000000000FFFF);
	//address64 ady(0x0013A20040B39D45);
	address64 ady;
	time_t start, check;
	bool RECEIVED = false;
	bool SENDERROR = true;
	rcvdPacket pkt;
	xb.zeroPktStruct(pkt);

	clock_t timerstart, timerstop;
	
	//////Open the buffer
	int fd = open("vidFIFO", O_RDONLY | O_NOCTTY | O_NONBLOCK);

	if (fd <= 0) 
	{
		std::cout << "Error opening video file.\n";
		STOP = true;
		port.join();
		return 0;
	}

	std::vector<uint8_t> vdata;
	bool GO = true;
	int n = 0;
	unsigned char b = 0x00;

	// The following code uses the network discovery packet to find the other node in the 
	// two node network and sends the video data to that node.

	xb.ATNDPkt();
	xb.sendPkt();
	time(&start);
	time(&check);

	while (GO == true && difftime(check,start) <= 10)
	{
		if (xb.pktAvailable()) GO = false;
		time(&check);
	}

	if (!(GO))
	{
		xbeeDMapi::zeroPktStruct(pkt);
		xb.rcvPkt(pkt);
		if (pkt.pType == APIid_ATCR) ady = pkt.from;
		else 
		{
			std::cout << "No neighbor found, using broadcast mode.\n";
			ady = 0x000000000000FFFF;
		}
	}

	if (GO)
	{
		std::cout << "No neighbor found, using broadcast mode.\n";
		ady = 0x000000000000FFFF;
	}
	
	GO = true;

	timerstart = clock();
	while(GO) // Main data reading loop. This loop reads data from a file and sends it.
	{
		int num = 0;
		while (num < 72)
		{
			n = read(fd, &b, 1);
			if (n == 1) vdata.push_back((uint8_t)b);
			if (n == 0) 
				{
					GO = false;
					num = 1000;
				}
			num++;	
		}
		

		
		if (vdata.size())
		{
			xb.makeUnicastPkt(ady);
			//xb.makeBCPkt(0x01);
			//xb.loadBCPkt(vdata);
			xb.loadUnicastPkt(vdata);
			vdata.clear();
			xb.sendPkt();
			time(&start);
			time(&check);
			bool WAIT = true;
			while (WAIT && difftime(check, start) <= 5)
			{
				if (xb.pktAvailable()) 
				{
					xb.rcvPkt(pkt);
					if (pkt.pType != 0x00) WAIT = false;
				}
				time(&check);
			}
			if (!(WAIT)) xb.rcvPkt(pkt);
			if (pkt.pType == APIid_TS && WAIT == false)
			{
				printf("Status: %d\n", pkt.deliveryStatus);
			}
			else 
			{
				std::cout << "Transmit Status not received. pType = " << std::hex << pkt.pType << ".\n";
				if (WAIT == false) std::cout << "WAIT IS FALSE!!!!!!\n";
			}

			xbeeDMapi::zeroPktStruct(pkt);
		}
	}
	
	
	///////////////////////////////////////////////End of tests
	timerstop = clock();

	close(fd);

	std::cout << "Packet transmission took " << (float)(timerstop - timerstart)/CLOCKS_PER_SEC << " second(s).\n";


	std::cout << "Tests complete, issuing stop request to TTY thread." << std::endl;

	STOP = true;
	port.join();

	std::cout << "Tests complete, thread shut down, ending sender.\n";
	return 0;
	
}

void w_tty(void)
{
	TTYserial tty;
	tty.begin(modem, 38400);
	
	if (!(tty.status()))
	{
		std::cout << "*******************ERROR OPENING SERIAL PORT*******************\n";
		return;
	}

	while (!(START)) {}

	while (!(STOP))
	{
		if (tty.available() && STOP == false)
		{
			inBytesMutex.lock();
			inBytes.push_back(tty.readbyte());
			inBytesMutex.unlock();
		}

		if (outBytes.size() && STOP == false)
		{
			outBytesMutex.lock();
			tty.sendbyte(outBytes.front());
			usleep(1);
			outBytes.pop_front();
			outBytesMutex.unlock();
		}
	}

	return;
}

void readerMain(void)
{
	std::thread port(w_tty);

	xbeeDMapi xb;
	START = true;

	time_t start;
	time_t check;

	rcvdPacket pkt;
	xb.zeroPktStruct(pkt);
	
	std::cout << "Start of readerMain()\n";

	time(&start);
	time(&check);
	int n = 0;
	
	std::thread plyr(w_vidPlayer, &vidBuffer, &vidBufferMutex);

	while(difftime(check,start) <= 60)
	{
		if(xb.pktAvailable())
		{
			uint8_t ptype = xb.rcvPkt(pkt);
			clock_t ticks = clock();
			if (ptype == APIid_RP)
			{
				for (std::vector<uint8_t>::iterator iter = pkt.data.begin(); iter != pkt.data.end(); iter++)
				{
					vidBufferMutex.lock();
					vidBuffer.push(*iter);
					vidBufferMutex.unlock();
				}
				xbeeDMapi::zeroPktStruct(pkt);
				time(&start);
				printf("Packet processing took: %4.3f seconds.\n", ((float)(ticks - clock()) / CLOCKS_PER_SEC));
			}

			else
			{
			//	std::cout << "Received packet of type: " << ptype << std::endl;
			}

		}
		time(&check);
	}

	std::cout << "End of while loop for readerMain()\n";

	STOP = true;

	port.join();
	plyr.join();
	
	std::cout << "TTY & plyr threads ended.\n";

	return;
}

void w_vidPlayer(std::queue<uint8_t> *vd, std::mutex *vdm)
{
	int fd = open("vidFIFO", O_WRONLY | O_CREAT);

	while (!(STOP))
	{
		if (vd->size())
		{
			vdm->lock();
			uint8_t byte = vd->front();
			write(fd, &byte, 1);
			vd->pop();
			vdm->unlock();
		}
	}

	close(fd);
}

