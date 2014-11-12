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
#include <fcntl.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <wiringPi.h>

// Structures
struct ISRflags
{
	//As these are acted on by buttons, these are LOCAL requests. 
	volatile int numberOfNode;
	volatile bool stop;
};

ISRflags userInput;

// Global variables
std::string modem = "/dev/ACM0";
globalFlags gf;
bool TTYFailure = false;
bool globalStop = false;
bool START = false;
bool recordSegment = false; 

int main(int argc, char* argv[])
{
	//Process command-line arguments

	//Start threads

	//Clean up and exit
	
	//First setup the user input and begin monitoring the GPIO resources
	userInput.numberOfNode = -1;
	userInput.stop = false;
	wiringPiSetup();
	wiringPiISR(buttonForNode1, INT_EDGE_FALLING, &node1ISR);
	wiringPiISR(buttonForNode2, INT_EDGE_FALLING, &node2ISR);
	wiringPiISR(buttonForNode3, INT_EDGE_FALLING, &node3ISR);
	wiringPiISR(buttonForStop, INT_EDGE_FALLING, &stopISR);

	if (argc == 2) modem = argv[1];
	
	gf.sendVideo = gf.displayVideo = gf.requestVideo = false;
	gf.addressForVideoRequestingNode = gf.addressToRequestVideoFrom = 0x00;
	
	std::thread tty_t(TTYMonitor_main);
	std::thread controlMain_t(control_main);
	std::thread UIMon_t(UIMonitor_main);

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

	//Check the stuff and monitor the RF fifos.
	// RFFifos => RF information and packets
	// todoFIFO => information from the UI.
	// The UI thread will put information into the todoFIFO.

	while(!(START)) {}
	VCR_threaded vcr;
	vcr.power();
	bool FIRSTRUN = true;
	stopwatch nmapRefreshTimer;

	while (globalStop == false)
	{

		if (FIRSTRUN == true || nmapRefreshTimer.read() >= 20000)
		{
			if (FIRSTRUN) FIRSTRUN = false;
			else nmapRefreshTimer.reset();
			RFPacketRequest rfp;
			rfp.requestType = task_networkMapUpdate;
			rfp.addressForRequest = BCaddr;
			RFOutgoingFIFOMutex.lock();
			RFOutgoingFIFO.push(rfp);
			RFOutgoingFIFOMutex.unlock();
		}

		if (incomingVideo.size())
		{
			RFPacketRequest rfp;
			incomingVideoMutex.lock();
			rfp.payload = incomingVideo.front();
			incomingVideo.pop();
			incomingVideoMutex.unlock();
			rfp.requestType = task_videoIn;
			vcr.play(rfp);
		}

		if (RFIncomingFIFO.size()) //These are remote requests / commands.
		{
			RFPacketRequest ipr;
			RFIncomingFIFOMutex.lock();
			ipr = RFIncomingFIFO.front();
			RFIncomingFIFO.pop();
			RFIncomingFIFOMutex.unlock();
			
			if (ipr.requestType == task_stop) //Remote stop request.
			{
				vcr.stop();
			}

			else if (ipr.requestType == task_requestVideo)
			{
				vcr.record(ipr);
			}
		}




		if (todoFIFO.size()) //These are commands from the LOCAL node.
		{
			globalFlags cr;
			todoFIFO.pop();

			if (cr.requestVideo)
			{
				RFPacketRequest rfp;

				rfp.addressForRequest = cr.addressToRequestVideoFrom;
				rfp.requestType = task_videoOut;
				RFOutgoingFIFOMutex.lock();
				RFOutgoingFIFO.push(rfp);
				RFOutgoingFIFOMutex.unlock();
			}

			else if (cr.stopVideo) //This is a stop request from the LOCAL node.
			{
				gf.stopVideo = true;
				RFPacketRequest rfp;
				rfp.addressForRequest = vcr.getToAddress();
				rfp.requestType = task_stop;
				rfp.payload = task_stop;
				RFOutgoingFIFOMutex.lock();
				RFOutgoingFIFO.push(rfp);
				RFOutgoingFIFOMutex.unlock();
				vcr.stop();				
			}
		}






	}
	
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
				if (inpkt.length > 5) // Packet of video.
				{
					incomingVideoMutex.lock();
					incomingVideo.push(inpkt.data);
					incomingVideoMutex.unlock();
				}

				else if (inpkt.length == 5 && inpkt.data[1] == 0x02 && inpkt.data[2] == 0x03
						&& inpkt.data[3] == 0x05 && inpkt.data[4] == 0x07) // This is a command packet
				{ //This is handled in control main. 
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
				stopwatch sendTimeOut;
				bool DONE = false;

				while (sendTimeOut.read() <= 1000 && DONE == false)
				{
					if (xb.pktAvailable())
					{
						uint8_t ptype = xb.rcvPkt(inpkt);
						if (ptype == APIid_TS && inpkt.deliveryStatus == 0x00)
						{
							DONE = true;
							std::cout << "Packet sent successfully to: ";
							printf("%x:%x:%x:%x:%x:%x:%x:%x\n", opr.addressForRequest[0], opr.addressForRequest[1],
									opr.addressForRequest[2], opr.addressForRequest[3], opr.addressForRequest[4],
									opr.addressForRequest[5], opr.addressForRequest[6], opr.addressForRequest[7]);
						}

						// Add code for repeating transmission. 
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
	while(START == false) {}

	bool USED = false;
	bool FIRSTRUN =true;
	globalFlags tdi;

	while(globalStop == false)
	{
		if (USED == true || FIRSTRUN == true)
		{
			FIRSTRUN = false;
			USED = false;
			tdi.stopVideo = false;
			tdi.sendVideo = false;
			tdi.addressForVideoRequestingNode = 0x00;
			tdi.addressToRequestVideoFrom = 0x00;
			tdi.displayVideo = false;
			tdi.requestVideo = false;
		}

		if (userInput.stop == true)
		{
			userInput.stop = false;
			tdi.stopVideo = true;
			todoFIFO.push(tdi);
			USED = true;
		}

		else if (userInput.numberOfNode)
		{
			if (userInput.numberOfNode <= (networkMap.neighborCount() -1))
			{
				tdi.addressToRequestVideoFrom = networkMap[userInput.numberOfNode];
				tdi.requestVideo = true;
				userInput.numberOfNode = -1;
				todoFIFO.push(tdi);
				USED = true;
			}
		}

		
	}
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

	if (_STOP == false) // If the stop button has not been pressed.
	{
		if (n > 5)
		{
			_PLAY = true;
			gf.displayVideo = true;
		}
		if (n > 7) // This is a full video packet and checking for trailing bytes isn't required.
		{
			_m.lock();
			_cassetteTape.push(invid.payload); //Actually writing to file and playing happens in VCRMain
			_m.unlock();
			return true;
		}
		if (n <= 7)
		{
			int lastbyte = n - 1;
			bool DONE = false;
			while(lastbyte >5 && DONE == false) //Check to see how many bytes are actually present.
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
		gf.requestVideo = true;

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
	while (globalStop == false && _POWER == true)
	{
		if (_PLAY == true)
		{
			if (lastNumberOfVideoBytesRead < 72)
			{
				int fd = open(infile.c_str(), O_WRONLY | O_CREAT);
				std::vector<uint8_t>::iterator it;
				while(!(_cassetteTape.empty()))
				{
					char byte = 0;
					for (it = _cassetteTape.front().begin(); it != _cassetteTape.front().end(); it++)
					{
						byte = *it;
						write(fd, &byte,1);
					}
					_cassetteTape.pop();
				}
				close(fd);
				_PLAY = false;
				//make a system call to run OMX player.
				lastNumberOfVideoBytesRead = 0;
			}
		}
		if (_RECORD == true && toAddress != 0x00)
		{
			//Make a system call to raspivid.
			//Read from the fifo.

			recordSegment = true;

			int fd = open(infile.c_str(), O_RDONLY);
			if (fd <= 0)
			{
				std::cout << "Error opening FIFO.\n";
				return;
			}

			bool GO = true;

			while(GO)
			{
				std::vector<uint8_t> v;
				bool GOGO = true;
				while(v.size() < 72 && GOGO == true);
				{
					uint8_t b = 0;
					int n = read(fd, &b, 1);
					if (n == 1) v.push_back(b);
					else GOGO = false;
				}

				if (v.size() > 5 && v.size() <= 72)
				{
					RFPacketRequest rfp;
					rfp.requestType = task_videoOut;
					rfp.addressForRequest = toAddress;
					rfp.payload = v;
					RFOutgoingFIFOMutex.lock();
					_RFOutgoingFIFO->push(rfp);
					RFOutgoingFIFOMutex.unlock();
				}

				if (v.size() <= 5)
				{
					uint8_t lastbyte = 0x13;
					while (v.size() <= 7)
					{
						if (lastbyte == 0x13)
						{
							v.push_back(0x11);
							lastbyte = 0x11;
						}

						else if (lastbyte == 0x11)
						{
							v.push_back(0x13);
							lastbyte = 0x13;
						}

					}

					RFPacketRequest rfp;
					rfp.requestType = task_videoOut;
					rfp.addressForRequest = toAddress;
					rfp.payload = v;
					_RFOutgoingFIFO->push(rfp);

				}

				if (v.size() < 72)
				{
					GO = false;
					_RECORD = false;
				}

			}
		}
	}

	return;
}

char VCR_threaded::stop(void)
{
	//stop recording or playing.
	_m.lock();
	_PLAY = false;
	_RECORD = false;
	_STOP = true;
	lastNumberOfVideoBytesRead = 0;
	toAddress = 0x00;
	while(!(_cassetteTape.empty())) _cassetteTape.pop();
	_m.unlock();
	gf.displayVideo = gf.requestVideo = false;
	gf.sendVideo = false;

	return 0;
}

void node1ISR(void)
{
	userInput.stop = false;
	userInput.numberOfNode = 0;
}

void node2ISR(void)
{
	userInput.stop = false;
	userInput.numberOfNode = 1;
}

void node3ISR(void)
{
	userInput.stop = false;
	userInput.numberOfNode = 2;
}

void stopISR(void)
{
	userInput.stop = true;
	userInput.numberOfNode = -1;
}

void recordMain(void)
{
	while (globalStop == false)
	{
		if (recordSegment)
		{
			recordSegment = false;
			system("raspivid -w 320 -h 240 -b 20000 -t 1000 -o outVideo");
		}
	}
}
