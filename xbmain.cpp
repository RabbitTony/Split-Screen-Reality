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

/*
   Test 2 (18 Auguest, 2014)
   	The purpose of this test is to attempt a 100 mb transfer.
*/

std::string modem = "/dev/ttyUSB0";
volatile bool START = false;
volatile bool STOP = false;
volatile bool FAIL = false;
void readerMain(void);
void w_tty(void);

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

	std::vector<uint8_t> data;
	for (int i = 0; i < 50; i++)
	{
		data.push_back(0x68);
	}

	address64 ady(0x0013A20040B39D52);
	time_t start, check;
	bool RECEIVED = false;
	bool SENDERROR = true;
	rcvdPacket pkt;
	xb.zeroPktStruct(pkt);

	time_t timerstart, timerstop;
	time(&timerstart);
	for (int i = 0; i < 2000; i++) //sending packets.
	{
		if (FAIL) return 0;
		xb.makeUnicastPkt(ady, 0x01);
		xb.loadUnicastPkt(data);
		xb.sendPkt();
		time(&start);
		time(&check);
		RECEIVED = false;
		SENDERROR = true;
		while (RECEIVED == false && difftime(check,start) <= 1.0)
		{
			if (xb.pktAvailable())
			{
				if (xb.rcvPkt(pkt) == APIid_TS)
				{
					if (pkt.deliveryStatus == 0x00) 
					{
						RECEIVED = true;
						SENDERROR = false;
						std::cout << "Packet #" << i << " sent successfuly.\n";
					}
				}
			}
			time(&check);
		}

		if (difftime(check,start) <= 1.0 && SENDERROR == true)
		{
			std::cout << "Packet send error, return status: ";
			printf("0x%X\n", pkt.deliveryStatus);
		}

		else if (difftime(check,start) > 1.0)
		{
			std::cout << "Packet response timeout for packet #" << i <<std::endl;
		}
		xbeeDMapi::zeroPktStruct(pkt);
	}
	time(&timerstop);

	std::cout << "Packet transmission took " << (double)difftime(timerstop,timerstart) << " second(s).\n";


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

	while(difftime(check,start) <= 30 && n < 20000)
	{
		if(xb.pktAvailable())
		{
			uint8_t ptype = xb.rcvPkt(pkt);

			if (ptype == APIid_RP)
			{
				n++;
				std::cout << "Packet number " << n << " received. Datalength = " << (int)pkt.length << std::endl;
				time(&start);
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
	
	std::cout << "TTY thread ended.\n";

	return;
}

