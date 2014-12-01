#include <iostream>
#include "xbeeDMapi.h"
#include "TTYserial.h"
#include "xbmain.h"
#include "stopwatch.h"
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
#include <cstdint>
#include <string>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>



int main(int argc, char* argv[])
{
	bool SLAVEMODE = false;
	if (argc > 1) // Handle proram arguments. 
	{
		for (int i = 1; i < argc; i++)
		{
			if (argv[i][0] == '-')
			{
				if (argv[i][1] == 's') SLAVEMODE = true;
				if (argv[i][1] == 'm') SLAVEMODE = false;
				if (argv[i][1] == 'h')
				{
					std::cout << "Useage: " << argv[0] << " -[smh] [modem]\n";
					std::cout << "NOTE: program defaults to master mode with a modem at\n";
					std::cout << modem << std::endl;
					std::cout << "Further, device speed is locked at 57600 baud.\n";
				}
			}
			else modem = argv[i];
		}
	}

	std::cout << "Starting TTY thread with: " << modem << " / " << baud << std::endl;
	std::thread port(TTYMonitor_main);
	std::thread videoPlayingMonitor(checkIfVideoPlaying_thread);

	START = true;

	while (TTYStarted == false) {}

	if (TTYStartFailed) 
	{
		std::cout << "TTY failure, modem was " << modem << std::endl;
		if (port.joinable()) port.join();
		return 0;
	}

	if (SLAVEMODE) slaveMain(modem);
	else masterMain(modem);

	if (videoPlayingMonitor.joinable()) videoPlayingMonitor.join();
	if (port.joinable()) port.join();

	std::cout << "All threads ended nicely.\n";
	return 0;
}

void TTYMonitor_main(void)
{
	TTYserial tty;
	tty.begin(modem, baud);
	
	if (!(tty.status()))
	{
		std::cout << "*******************ERROR OPENING SERIAL PORT*******************\n";
		TTYStarted = true;
		TTYStartFailed = true;
		return;
	}

	else 
	{
		std::cout << "TTY started.\n";
		TTYStarted = true;
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

void slaveMain(std::string m)
{
	xbeeDMapi xb;
	std::cout << "Entering slave infinite loop.\n";
	while (!(STOP))
	{
		if (xb.pktAvailable())
		{
			std::cout << "Packet received.\n";
			rcvdPacket rp;
			xb.rcvPkt(rp);
			if (rp.pType == APIid_RP)
			{
				if (rp.data[0] & (1<<SSRPT_videoRequest))
				{
					std::cout << "Received video request. Starting to buffer.\n";
					system("raspivid -w 320 -h 240 -fps 20 -t 4000 -b 20000 -o outVideo");
					int fd = open("outVideo", O_RDONLY);
					if (fd <= 0) std::cout << "ERROR OPENING OUTGOING VIDEO FILE.\n";
					bool GO = true;
					std::vector<uint8_t> v;
					while (GO)
					{
						char byte;
						int n = read(fd, &byte, 1);
						if (n == 1 && v.size() < 70) v.push_back(byte);
						if (n == 0 || v.size() == 70)
						{
							SSRPacketCreator outgoingPacket(SSRPT_videoPacket);
							outgoingPacket.load(v);
							if (outgoingPacket.check() == false) std::cout << "Packet to big.\n";
							xb.makeUnicastPkt(rp.from);
							xb.loadUnicastPkt(outgoingPacket.get());
							xb.sendPkt();
							bool DONE = false;
							bool REDO = false;
							stopwatch rsend_stopwatch;
							while (DONE == false)
							{
								if (xb.pktAvailable())
								{
									rcvdPacket rp;
									xb.rcvPkt(rp);
									if (rp.pType == APIid_TS) 
									{
										if (rp.deliveryStatus == 0x00)
										{
											DONE = true;
											std::cout << "Good packet sent.\n";
										}
										else REDO = true;
									}

									if (REDO)
									{
										printf("Packet transmission error: 0x%X -- retrying.\n", rp.deliveryStatus);
										xb.sendPkt();
										REDO = false;
									}
								}

								if (rsend_stopwatch.read() > 10*1000) 
								{
									std::cout << "Timeout on packet sending, giving up.\n";
									DONE = true;
								}
							}
							v.clear();
						}

						if (n == 0) 
						{
							GO = false;
							close(fd);
							system("mv outVideo outVideo.bak");
						}
					}
				}
				else std::cout << "Unknown packet type received.\n";
			}
		}
	}
	return;
}

void masterMain(std::string m)
{
	xbeeDMapi xb;
	stopwatch ATNDResend;

	std::cout << "Starting master main." << std::endl;
	NMAP_stopwatch.reset();

	while (!(STOP))
	{
		//First thing: send a network map request. 
		ATNDResend.reset();
		xb.ATNDPkt();
		xb.sendPkt();
		std::cout << "Sent ND command.\n";
		while (videoBuffer.size()) videoBuffer.pop();

		bool GOTNEIGHBOR = false;
		while(GOTNEIGHBOR == false && ATNDResend.read() < 10*1000)
		{
			if (xb.pktAvailable())
			{
				rcvdPacket pkt;
				xbeeDMapi::zeroPktStruct(pkt);
				xb.rcvPkt(pkt);

				if (pkt.pType == APIid_ATCR)
				{
					if(pkt.ATCmd[0] == 'N' && pkt.ATCmd[1] == 'D')
					{
						std::cout << "Neighbor responded to map request.\n";
						NMAP.update(pkt.from);
						GOTNEIGHBOR = true;
					}
				}
			}

			if (NMAP.neighborCount()) GOTNEIGHBOR = true;
		}

		if (GOTNEIGHBOR == false) std::cout << "Network map response timed out.\n";

		if (GOTNEIGHBOR) // We found a neighbor, now we start the looping through asking for video. 
		{
			printf("GOTNEIGHBOR is true, index = %d, count = %d\n", neighborIndex, NMAP.neighborCount());
			// This loop could be replaced to work with Pi-Game for selecting which neighbor to select a video from. . . 
			for (int i = 0; i < NMAP.neighborCount(); i++) // Print out the neighbors for debugging purposes. 
			{
				address64 t = NMAP[i];
				printf("%X:%X:%X:%X:%X:%X:%X:%X\n", t[0], t[1], t[2], t[3], t[4], t[5], t[6], t[7]);
			}
			if (neighborIndex < NMAP.neighborCount() && NMAP.neighborCount() > 0) //Make sure we are asking for a valid address. 
			{
				currentNeighbor = NMAP[neighborIndex];

				SSRPacketCreator outgoingPacket(SSRPT_videoRequest); //setup a video request packet.
				xb.makeUnicastPkt(currentNeighbor);
				xb.loadUnicastPkt(outgoingPacket.get());
				printf("SSR message type: 0x%x\n", outgoingPacket.get()[0]);
				if (xb.sendPkt() == false) std::cout << "Packet transmission failure.\n";

				//Need to get the acknowledgement status
				stopwatch TS_stopwatch;
				while (!(xb.pktAvailable()) && TS_stopwatch.read() <= 10*1000)
				{
					rcvdPacket rp;
					xb.rcvPkt(rp);
					if (rp.pType == APIid_TS)
					{
						std::cout << "Transmit status received.\n";
						printf("Status is: 0x%f\n", rp.deliveryStatus);
					}

					//else std::cout << "other packet type received.\n";
				}

				if (TS_stopwatch.read() > 10*1000) std::cout << "Timeout on TS message.\n"; //Let us know if it timed out. Debugging. 
				std::cout << "Sending video request to neighbor with address:\n";
				printf("%x--%x--%x--%x--%x--%x--%x--%x\n", currentNeighbor[0], currentNeighbor[1], 
					currentNeighbor[2], currentNeighbor[3], currentNeighbor[4], currentNeighbor[5],
					currentNeighbor[6], currentNeighbor[7]);
				stopwatch videoTimeout;

				bool KEEPWAITING = true;
				bool FULLSEGMENT = false;
				
				std::cout << "Entering wait loop for video.\n";
				while(KEEPWAITING) // This is the receiving video loop. 
				{
					if (videoTimeout.read() >= 60*1000) 
					{
						KEEPWAITING = false;
						while (videoBuffer.size()) videoBuffer.pop();
					}

					else
					{
						if (xb.pktAvailable())
						{
							rcvdPacket pkt;
							xbeeDMapi::zeroPktStruct(pkt);
							xb.rcvPkt(pkt);

							if (pkt.pType == APIid_ATCR)
							{
								std::cout << "Received ATCR message packet: ";
								if (pkt.ATCmd[0] == 'N' && pkt.ATCmd[1] == 'D') 
								{
									NMAP.update(pkt.from);
									std::cout << "hello message.\n";
								}
							}

							else if (pkt.pType == APIid_RP)
							{
								if (pkt.data[0] & (1<<SSRPT_videoPacket))
								{
									std::cout << "Received a video packet from current neighbor.\n";
									videoTimeout.reset();
									pkt.data.erase(pkt.data.begin());
									videoBuffer.push(pkt.data);
									if (pkt.data.size() < 70)
									{
										KEEPWAITING = false;
										FULLSEGMENT = true;
									}
								}
							}

							else
							{
								std::cout << "Received other packet, ignoring.\n";
							}
						}
					}

				}

				if (FULLSEGMENT)
				{
					while(currentlyPlayingVideo) {}
					int fd = open("inVideo", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
					if (fd <= 0) 
					{
						std::cout << "Video received but found file opening failure.\n";
						while(videoBuffer.size()) videoBuffer.pop();
					}

					else
					{ 
						std::cout << "**********GOT VIDEO AND PLAYING IT****************\n";
						while (videoBuffer.size())
						{
							std::vector<uint8_t> v = videoBuffer.front();
							videoBuffer.pop();
							char byte = 0x00;
							for (std::vector<uint8_t>::iterator it = v.begin(); it != v.end(); it++)
							{
								byte = *it;
								write(fd, &byte,1);
							}
						}

						close(fd);

						//system("omxplayer --fps 20 inVideo");
						currentlyPlayingVideo = true;
						system("mv inVideo currentVideo");
						system("./playvideo&");
						//system("cp inVideo inVideo.bak");
						//system("rm inVideo");
					}
				}

				neighborIndex++; // move on to the next neighbor and reset if we are at the end of the list. 
				if (neighborIndex >= NMAP.neighborCount()) neighborIndex = 0;

				if (NMAP_stopwatch.read() >= (120*1000) && NMAP.neighborCount() > 0)
				{
					NMAP.clear();	
					std::cout << "Network Map reset after timeout.\n";
				} 
			}

			else //This should never happen, but just in case. 
			{
				neighborIndex = 0;
				std::cout << "neighborIndex was reset to zero. \n";
			}
		}
	}
}

bool SSRPacketCreator::create(uint8_t type)
{
	if (type < 1 || type > 5) return false;
	clear();
	_type |= (1<<type);
	return true;
}

bool SSRPacketCreator::load(std::vector<uint8_t> p)
{
	if (p.size() <= 0 || p.size() > 70) return false;

	_p = p;
	return true;
}

bool SSRPacketCreator::check(void)
{
	if (_p.size() < 71) return true;
	else return false;
}

std::vector<uint8_t> SSRPacketCreator::get(void)
{
	std::vector<uint8_t> tv;
	tv.push_back(_type);
	if(_p.size() > 0)
	{
		for(std::vector<uint8_t>::iterator it = _p.begin(); it != _p.end(); it++)
		{
			tv.push_back(*it);
		}
	}

	return tv;
}

void checkIfVideoPlaying_thread(void)
{
	// This thread simply watches a fifo and toggles the variable to let the code know to play the next video
	// this avoids data races. 
	while(!(START)) {}
	int fd = open("videoPlayingFIFO", O_RDONLY);

	
	while(!(STOP))
	{
		int n;
		char b;
		n = read(fd, &b, 1);
		if (n == 1)
		{
			if (b == 'd') currentlyPlayingVideo = false;
		}
	}

	close(fd);
}