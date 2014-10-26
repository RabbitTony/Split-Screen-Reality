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
bool START = false;

int main(int argc, char* argv[])
{
	//Process command-line arguments

	//Start threads

	//Clean up and exit

	if (argc == 2) modem = argv[1];
	
	gf.sendVideo = gf.displayVideo = gf.requestVideo = false;
	gf.addressForVideoRequestingNode = gf.addressToRequestVideoFrom = 0x00;
	
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
			tty.sendbyte(outBytes.front());
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
				if (inpkt.length > 5 && gf.displayVideo == true) // Packet of video.
				{
					incomingVideoMutex.lock();
					incomingVideo.push(inpkt.data);
					incomingVideoMutex.unlock();
				}

				else if (inpkt.length == 5 && inpkt.data[1] == 0x02 && inpkt.data[2] == 0x03
						&& inpkt.data[3] == 0x05 && inpkt.data[4] == 0x07) // This is a command packet
				{
					RFPacketRequest ipr;
					ipr.requestType = inpkt.data.front();
					ipr.addressForRequest = inpkt.from;
					RFIncomingFIFOMutex.lock();
					RFIncomingFIFO.push(ipr);
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
						uint8_t ptype = xb.rcvPkt(inpkt);
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

bool VCR_threaded::play(const RFPacketRequest& invid)
{
	//If recording, fail
	//Set playing and store current request. 
	if (_RECORD == true || _threadSTOP == true || _POWER == false)
	{
		return false;
	}

	//First check the amount of data. if n < 72 bytes, this is the last packet
	//for this segment.

	int n = invid.payload.size();

	if (n <= 5) return false;

	if (_STOP == false) // If the stop button has been pressed.
	{
		if (n > 5) _PLAY = true;
		if (n == 72) // This is a full video packet and checking for trailing bytes isn't required.
		{
			_m.lock();
			_cassetteTape.push(invid.payload); //Actually writing to file and playing happens in VCRMain
			_m.unlock();
			return true;
		}
		if (n > 5 && n < 72)
		{
			int lastbyte = n - 1;
			bool DONE = false;
			while(lastbyte > 4 && DONE == false) //Check to see how many bytes are actually present.
			{
				DONE = true;
				if (invid.payload[lastbyte] == 0x11)
				{
					if (invid.payload[lastbyte - 1] == 0x13) DONE = false;
				}
				if (invid.payload[lastbyte] == 0x13)
				{
					if (invid.payload[lastbyte - 1] == 0x11) DONE = false;
				}

				if (DONE == false) lastbyte--;
			}

			//Now check for possible fail conditions (unlikely but who knows)
			if (DONE == false)
			{
				_PLAY = false;
				return false;
			}

			std::vector<uint8_t> tempinvid;
			for (int i = 0; i < lastbyte; i++) //lastbyte technically is 1 after the last byte.
				tempinvid.push_back(invid.payload[i]);
			_m.lock();
			_cassetteTape.push(tempinvid);
			_m.unlock();
			return true;
		}
	}
	return false;
}

bool VCR_threaded::record(const RFPacketRequest& outvid)
{
	//If playing, fail
	//Set recording and store current request. 
	if (_PLAY == true || _threadSTOP == true || _POWER == false)
		return false;
	if (_STOP == false)
	{
		_RECORD = true;

		if (toAddress == outvid.addressForRequest) return true;
		else if (toAddress == 0x00)
		{
			toAddress = outvid.addressForRequest;
			return true;
		}
	}
	return false;
}

void VCR_threaded::VCRMain(void)
{
	
	//Looping
	//if playing and full video received, play the segment
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

