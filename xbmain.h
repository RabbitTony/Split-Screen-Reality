#ifndef XBMAIN
#define XBMAIN

#include <queue>
#include <vector>
#include <ctime>
#include "stopwatch.h"
#include "xbeeDMapi.h"
#include <string>
#include <cstdint>

//Global variables, flags, FIFOs, etc. 
std::string modem = "/dev/ttyAMA0";
address64 lastNeighbor, currentNeighbor, nextNeighbor;
int neighborIndex = 0;
int baud = 57600;
xbeeNeighbors NMAP;
stopwatch NMAP_stopwatch, videoBuffer_stopwatch;
std::queue<std::vector<uint8_t> > videoBuffer;
const uint8_t& SSRPT_videoPacket = 0x01;
const uint8_t& SSRPT_videoRequest = 0x02;
const uint8_t& SSRPT_videoStop = 0x03;
const uint8_t& SSRPT_ping = 0x04;
const uint8_t& SSRPT_pingResponse = 0x05;
bool STOP = false;
bool START = false;
bool TTYStarted = false;
bool TTYStartFailed = false;

//Functions and Classes

void TTYMonitor_main(void);
void slaveMain(std::string m);
void masterMain(std::string m);

class SSRPacketCreator 
{
private:
	std::vector<uint8_t> _p;
	uint8_t _type;
public:
	SSRPacketCreator() { clear(); }
	SSRPacketCreator(uint8_t type) {
		clear();
		create(type); 
	}

	bool clear(void) {
		_type = 0;
		if (!(_p.empty())) _p.clear();
	}

	bool create(uint8_t type); //Clear and set type.
	bool load(std::vector<uint8_t> p); //Add payload to what is currently stored. 
	bool check(void); //Ensure total size is less than or equal to 72 bytes. 
	std::vector<uint8_t> get(void); //Get the vector to send out as an XB packet. 
};

#endif
