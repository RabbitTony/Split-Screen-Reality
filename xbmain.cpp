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
   Test 1 (17 Auguest, 2014)
   	The purpose of this test is to do a complete test of all classes/functions
	written thus far. 

	Functions under test / results:

	xbeeDMapi :
		rcvPkt
		(make/load)BCPkt
		(make/load)UnicastPkt
		ATNDPkt
	xbeeNeighbors :
		update
		neighborCount
		operator[]
		remove
		clear
*/

std::string modem = "/dev/ttyUSB0";
volatile bool START = false;
volatile bool STOP = false;
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

	xb.makeBCPkt();
	std::vector<uint8_t> data;
	data.push_back(0x68);
	data.push_back(0x69);
	data.push_back(0x68);
	data.push_back(0x13);
	xb.loadBCPkt(data);
	xb.sendPkt();
	
	std::cout << "BC packet sent, data was 0x68 0x69 0x68 0x69.\n";
	time_t start;
	time_t check;
	time(&start);
	time(&check);
	rcvdPacket pkt;
	xb.zeroPktStruct(pkt);

	while (difftime(check,start) <= 3.00)
	{
		if (xb.pktAvailable())
		{
			if(xb.rcvPkt(pkt) == APIid_TS)
			{
				std::cout << "***TS status received***\n";
				printf("Retry count: %d\n", pkt.txRetryCount);
				printf("Delivery status: 0x%X\n", pkt.deliveryStatus);
			}
			else
			{
				std::cout << "***Non-TS packet type received***\n";
				printf("Type: 0x%X\n", pkt.pType);
			}

			xb.zeroPktStruct(pkt);
		}
		time(&check);
	}
	
	std::cout << "Testing unicast data.\n";
	//Unicast. 
	address64 ady(0x0013A20040B39D52);
	printf("%x.%x.%x.%x.%x.%x.%x.%x\n", ady[0], ady[1], ady[2], ady[3], ady[4], ady[5], ady[6], ady[7]);
	if (xb.makeUnicastPkt(ady, 0x02)) std::cout << "Unicast packet made successfully.\n";
	//std::vector<uint8_t> data;
	data.clear();
	data.push_back(0x68);
	data.push_back(0x69);
	data.push_back(0x68);
	data.push_back(0x69);
	if (xb.loadUnicastPkt(data)) std::cout << "Unicast packet loaded successfully.\n";
	if (!xb.sendPkt())
	{
		std::cout << "Error with unicast data.\n";
	}


	time(&start);
	time(&check);
	xb.zeroPktStruct(pkt);

	while (difftime(check,start) <= 15.00)
	{
		if (xb.pktAvailable())
		{
			if(xb.rcvPkt(pkt) == APIid_TS)
			{
				std::cout << "TS status received:\n";
				printf("Retry count: %d\n", pkt.txRetryCount);
				printf("Delivery status: 0x%X\n", pkt.deliveryStatus);
				xb.zeroPktStruct(pkt);
			}
			else
			{
				std::cout << "Non-TS packet type received.\n";
			}
		}
		time(&check);
	}
	
	//test for AT-ND packet.

	xb.ATNDPkt();
	if(!(xb.sendPkt()))
	{
		std::cout << "Error sending ATND packet.\n";
		STOP = true;
		port.join();
		return 0;
	}

	xb.zeroPktStruct(pkt);
	
	std::cout << "AT-ND packet sent.\n";
	xbeeNeighbors nmap;

	time(&start);
	time(&check);

	while (difftime(check,start) <= 30.0)
	{
		if (xb.pktAvailable())
		{
			if(xb.rcvPkt(pkt) == APIid_ATCR)
			{
				if (pkt.ATCmd[0] == 0x4E && pkt.ATCmd[1] == 0x44)
				{
					std::cout << "Found node with following address:\n";
					printf("%x:%x:%x:%x:%x:%x:%x:%x\n", pkt.from[0], pkt.from[1], pkt.from[2], pkt.from[3],
							pkt.from[4], pkt.from[5], pkt.from[6], pkt.from[7]);
					nmap.update(pkt.from);
				}
			}

			else
			{
				std::cout << "Non-ATCR packet type received.\n";
			}
		}
		time(&check);
	}

	std::cout << "At end of while loop for AT-ND test, nmap contains " << nmap.neighborCount() << " neighbor(s).\n";

	for(int i = 0; i < nmap.neighborCount(); i++)
	{
		address64 adr = nmap[i];
		printf("N[%d] = %x:%x:%x:%x:%x:%x:%x:%x\n", i, adr[0], adr[1], adr[2], adr[3],
			adr[4], adr[5], adr[6], adr[7]);
	}
	
	std::cout << "Testing the remove method of xbeeNeighbors.\n";

	if (nmap.remove(0))
	{
		std::cout << "Remove successful, neighbor count: " << nmap.neighborCount() << std::endl;
	}

	else
	{
		std::cout << "Error remove neighbor, neighbor count: " << nmap.neighborCount() << std::endl;
	}

	std::cout << "AT-ND test complete.\n";

	std::cout << "Tests complete, issuing stop request to TTY thread." << std::endl;

	STOP = true;
	port.join();

	std::cout << "Tests complete, thread shut down, ending sender.\n";
	return 0;
	
}

void w_tty(void)
{
	TTYserial tty;
	tty.begin(modem, 57600);
	
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

	while(difftime(check,start) <= 120.0)
	{
		if(xb.pktAvailable())
		{
			uint8_t ptype = xb.rcvPkt(pkt);

			if (ptype == APIid_RP)
			{
				std::cout << "Received data packet:\n";
				printf("FROM : %x.%x.%x.%x.%x.%x.%x.%x\n", pkt.from[0], pkt.from[1], pkt.from[2], 
						pkt.from[3], pkt.from[4], pkt.from[5], pkt.from[6], pkt.from[7]);
				std::cout << "With data:\n";
				for (std::vector<uint8_t>::iterator iter = pkt.data.begin(); iter != pkt.data.end(); iter++)
				{
					printf("0x%x\n", *iter);
				}

			}

			else
			{
				std::cout << "Received packet of type: " << ptype << std::endl;
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

