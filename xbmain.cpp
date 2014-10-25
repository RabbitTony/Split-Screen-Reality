#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <ctime>
#include <mutex>
#include <thread>
#include <vector>
#include <queue>
#include <iterator>
#include <cstdint>
#include <unistd.h>
#include <time.h>
#include "xbeeDMapi.h"
#include "TTYserial.h"
#include "xbmain.h"
#include "stopwatch.h"


// Global variables
std::string modem = "/dev/ACM0";
globalFlags gf;
bool TTYFailure = false;
bool globalStop = false;
bool START = false

int main(int argc, char* argv[])
{
	//Process command-line arguments

	//Start threads

	//Clean up and exit

	if (argc == 2) modem = argv[1];
	
	gf.sendVideo = gf.displayVideo = gf.requestVideo = false;
	gf.addressForVideoRequestingNode = addressToRequestVideoFrom = 0x00;
	
	std::thread tty_t(TTYMonitor_main);

	START = true;

	if (TTYFailure) 
	{
		globalStop = true;
		std::cout << "Error opening serial port.\n";
		return -1;
	}

	return 0;
}

void control_main(void)
{
	//Setup the VCR. 

	//Check the stuff. 
	
	return;
}

void TTYMonitor_main(void)
{
	//Startup the serial. 

	//Check for failure.

	//Infinite loop.

	TTYserial tty;

	tty.begin(modem, 57600);
	if (!(tty.status()))
	{
		TTYFailure = true;
		return;
	}

	while (!(START))
	{
		if (globalStop) return;
	}

	while (!(globalStop))
	{
		if (tty.available() && globalStop == false)
		{
			inBytesMutex.lock();
			inBytes.push_back(tty.readbyte());
			inBytesMutex.unlock();
		}

		if (outBytes.size() && globalStop == false)
		{
			outBytesMutex.lock();
			tty.sendByte(outBytes.front());
			usleep(1);
			outBytes.pop_front();
			outBytesMutex.unlock();
		}
	}

	
	return;
}

void RFMonitor_main(void)
{
	//Start running. 
	
	xbeeDMapi xb;
	rcvdPacket inpkt;

	while (!(START)) {}

	while (globalStop == false)
	{
		if(xb.pktAvailable())
		{
			uint8_t ptype = xb.rcvPkt(inpkt);
			if (ptype == APIid_RP)
			{
				if (inpkt.length == 72 && gf.displayVideo == true) // Packet of video. 
				{
					incomingVideoMutex.lock();
					incomingVideo.push(inpkt.data);
					incomingVideoMutex.unlock();
				}

				else if (inpkt.length == 1) // This is a command packet
				{
					RFPacketRequest ipr;
					ipr.requestType = inpkt.data.front();
					ipr.addressForRequest = inpkt.from;
					RFIncomingFIFOMutex.lock();
					RFIncomingFIFOMutex.push(inpkt);
					RFIncomingFIFOMutex.unlock();
				}
			}
			
			else if (ptype == APIid_ATCR && inpkt.ATCmd[0] == 'N' && inpkt.ATCmd[1] == 'D')
			{
				networkMap.update(inpkt.from);
			}
		}

		if(RFOutgoingFIFO.size()) //We have a request that needs to go out. 
		{
			RFOutgoingFIFOMutex.lock();
			RFPacketRequest opr = RFOutgoingFIFO.front();
			RFOutgoingFIFO.pop();
			RFOutgoingFIFOMutex.unlock();

			//Now we need to figure out what it is.
			//The requests are going to be actually setup by the control_main thread.
			//This is because the UI information is going to be processed there and the
			//decision to request video from a particular node will be arranged there. 


			if (opr.requestType != task_videoIn && opr.requestType != task_networkMapUpdate)
			{
				xb.makeUnicastPkt(opr.addressForRequest);
				xb.loadUnicastPkt(opr.payload);
				xb.sendPkt();
					
				//check for the acknowledgement. 
				stopwatch to;
				bool DONE = false;

				while (to.read() <= 1000 && DONE == false)
				{
					if (xb.pktAvailable())
					{
						uint8_t ptype = xb.rcvPacket(inpkt);
						if (ptype == APIid_TS && inpkt.deliveryStatus == 0x00)
						{
							DONE = true;
						}

						// Need to add some code for repeating the sending. 
					}
				}
			}

			if (opr.requestType == task_networkMapUpdate)
			{
				xb.ATNDPkt();
				xb.sendPkt();
			}

						
		}
	}
	return;
}

void UIMonitor_main(void)
{
	//Start running. 

	return;
}

char VCR_threaded::play(const RFPacketRequest& invid)
{
	//If recording, fail
	//Set playing and store current request. 

	return 0;
}

char VCR_threaded::record(const RFPacketRequest& outvid)
{
	//If playing, fail
	//Set recording and store current request. 

	return 0;
}

void VCR_threaded::VCRMain(void)
{
	
	//Looping
	//if playing and full video recieved, play the segment
	//if recording keep sending video until segment is over

	return;
}

char VCR_threaded::VCRIndicator(void)
{
	//return flags for recording, playing, or stopped. 
	return status;
}

char VCR_threaded::stop(void)
{
	//stop recording or playing.
	return;
}

