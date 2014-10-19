#include <iostream>
#include <thread>
#include <queue>
#include <vector>
#include <mutex>
#include <string>
#include "xbeeDMapi.h"

//Data structures & constants. 
extern struct globalFlags {
	bool sendVideo;
	address64 addressForVideoRequestingNode;
	bool displayVideo;
	bool requestVideo;
	address64 addressToRequestVideoFrom;
	bool stopVideo;
};

const std::string &infile = "inVideo";
const std::string &outfile = "outVideo";
const int &buttonForNode1 = 1 //relates to the wiringPI pinout.
const int &buttonForNode2 = 4 // " "
const int &buttonForNode3 = 5 // " "
const int &buttonForStop = 6 //  " "

struct RFPacketRequest {
	int requestType;
	address64 addressForRequest; //Depending on whether this is incoming or outgoing, could be destination or source.
	std::vector<uint8_t> payload;
};



//FIFOs and such. 
xbeeNeighbors networkMap;
std::queue<globalFlags> todoFIFO; // This buffer contains the tasks that need to be done. 
std::mutex RFOutgoingFIFOMutex, RFIncomingFIFOMutex;
std::queue<RFPacketRequest> RFOutgoingFIFO;
std::queue<RFPacketRequest> RFIncomingFIFO;
std::mutex outgoingVideoMutex, incomingVideoMutex;
std::queue<std::vector<uint8_t>> outgoingVideo; 
std::queue<std::vector<uint8_t>> incomingVideo; // May not need this after all....

// Functions
void TTYMonitor_main(void); //Controls sending the bytes to the xbee over UART.
void RFMonitor_main(void); //Monitors the global flags and RFOutGoingFIFO while updating RFIncomingFIFO;
void UIMonitor_main(void); //Thread to look for user input. 
void control_main(void); //Thread to maintain overall controll of the program and process all the buffers. 

// Classes
class VCR_threaded 
{ //Either records 72 bytes of video or plays video. NOTE: The play function loads data into a file, but plays it once an
	// end to the file has been received, to ensure that OMX player will actually play the file. 

	private:
		std::queue<RFPacketRequest> *_RFOutgoingFIFO;
		std::queue<TFPacketRequest> _cassetteTape; //Funny no? This holds either the incoming 72 bytes or the outgoing 72 bytes. 
		volatile bool _STOP, _RECORD, _PLAY, _threadSTOP, _POWER;
		std::mutex _m; // This keeps the dataraces away...
		std::thread _VCRThread;
	public:
		VCR_threaded() : _VCRThread()
		{
			_STOP = _RECORD = _PLAY = _threadSTOP = _POWER = false;
			_RFOutgoingFIFO = &RFOutgoingFIFO;
		}
		
		VCR_threaded(std::queue<RFPacketRequest> *out) : _VCRThread()
		{
			_STOP = _RECORD = _PLAY = _threadSTOP = _POWER = false;
			_RFOutgoingFIFO = out;
			_RFIncomingFIFO = in;
		}

		//This thread class is designed to operate in only 1 state at a time. This means that the class is either
		//recording video or viewing video, not both. So, the single buffer _cassetTape will get cleared every
		//time the state changes. The only purpose for having the _cassetTape buffer is to ensure that the thread 
		//can keep up with the data that is coming in. 

		char play(const RFPacketRequest& invid); // Supplies the packet containing the next 72 bytes of data.
		char record(const RFPacketRequest& outvid); // Requests 1 second of video, packet contains the address. will push out packets into the RF 
			//request buffer. 
		char stop(void); // Stops sending and/or receiving video. 
		void VCRMain(void); // This monitors the flags and responds to them. 
		void VCDIndicator(void); // Checks the status of the thread class. 
		void power(void)
		{	
			if (!(_POWER))
			{
				_POWER = true;
				_threadSTOP = false;
				_VCRThread = std::thread(&VCR_threaded::VCRMain, this);
			}

			else
			{
				_threadSTOP = true;
				if (_VCRThread.joinable()) _VCRThread.join();
				_POWER = false;
			}
		}
};
