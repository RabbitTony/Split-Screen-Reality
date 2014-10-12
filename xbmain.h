#include <iostream>
#include <thread>
#include <queue>
#include <vector>
#include <mutex>
#include <string>
#include "xbeeDMapi.h"

extern struct globalFlags {
	bool sendVideo;
	address64 addressForVideoRequestingNode;
	bool displayVideo;
	bool requestVideo;
	address64 addressToRequestVideoFrom;
	bool stopVideo;
};

const std::string infile = "inVideo";
const std::string outfile = "outVideo";
const int buttonForNode1 = 0 //need to change this when we wire it.
const int buttonForNode2 = 1 //need to change this when we wire it.
const int buttonForNode3 = 2 //need to change this when we wire it.

struct RFPacketRequest {
	int requestType;
	address64 addressForRequest;
	std::vector<uint8_t> payload;
};

xbeeNeighbors networkMap;
std::mutex RFOutgoingFIFOMutex, RFIncomingFIFOMutex;
std::queue<RFPacketRequest> RFOutgoingFIFO;
std::queue<RFPacketRequest> RFIncomingFIFO;

// Functions
void TTYMonitor_main(void); //Controls sending the bytes to the xbee over UART.
void RFMonitor_main(void); //Monitors the global flags and RFOutGoingFIFO while updating RFIncomingFIFO;
// Classes
