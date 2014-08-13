#ifndef header_xbeeDMapi
#define header_xbeeDMapi

#include <iostream>
#include <cstdint>
#include <exception>
#include <string>
#include <vector>
#include <iterator>
#include <list> 
#include <mutex>

extern std::mutex inBytesMutex, outBytesMutex;
extern std::list<uint8_t> inBytes;
extern std::list<uint8_t> outBytes;

class address64 
{

	private:
		uint64_t address64bit;
		uint8_t address8bytes[8];

	public:
		address64();
		address64(uint64_t);
		address64(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t);

		address64& operator=(const uint64_t&);
		address64& operator=(const address64&);
		uint8_t& operator[](const int);
		bool operator==(const address64&)const;
		bool operator!=(const address64&)const;

		void split(); //go from 64bit address to 8 1-byte addresses
		void combine(); //go from 8 1-byte addresses to 1 64bit address
};

class xb_except : public std::exception 
{

	private:
		std::string errstr;
	public:
		xb_except(std::string str) {errstr = str;}

		virtual const char* what() const throw()
		{
			return errstr.c_str();
		}

		virtual ~xb_except() throw() {}
};

struct rcvdPacket 
{
	address64 from;
	uint8_t length;
	uint8_t pType;
	int txRetryCount;
	uint8_t deliveryStatus;
	uint8_t receiveOpts;

	bool nopkts;
	bool badlength;
	bool badchecksum;
	std::vector<uint8_t> data;
};

const address64 BCadr(0x000000000000ffff); //Broadcast address
const uint8_t StDelim = 0x7E; //Start deliminator

//These constants are the API ID's, basically tell the frame type. 

const uint8_t APIid_TR = 0x10; // Transmit request
const uint8_t APIid_RP = 0x90; // Receive packet
const uint8_t APIid_ATC = 0x08; // AT command
const uint8_t APIid_ATCR = 0x88; // AT command response
const uint8_t APIid_TS = 0x8B; // Transmit Status
const uint8_t APIid_MS = 0x8A; // Modem Status


class xbeeDMapi 
{ 

	private:
		std::vector<uint8_t> pktBytes;
		std::list<uint8_t> rcvdBytes;
		int _processedPktCount;  

		//The following variables are the byte fields required for the packets. 
		//The appropriate functions will fill them in and use them as neccessary. 

		uint8_t _startDelim;
		uint8_t _lengthMSB;
		uint8_t _lengthLSB;
		uint8_t _frameID;
		uint8_t _frameType;
		address64 _destAdr;
		uint8_t _R1;
		uint8_t _R2;
		uint8_t _bcRad;
		uint8_t _txOpts;
		std::vector<uint8_t> _payLoad;
		uint8_t _chkSum;
		uint8_t _packetType;
		char _ATCmd[2];

		bool _pMade;
		bool _pLoaded;

		
		void clearPktData();

	
	public:
		bool pktAvailable(); // Check for available packets & move data from inbytes into rcvdBytes. 
		uint8_t rcvPkt(rcvdPacket&); // process a packet from rcvdBytes and return the type. . 
		void zeroPktStruct(rcvdPacket&); //Set all fields to zero for a packet struct. 
		bool makeBCPkt(uint8_t fID = 0x01); // Prep a broadcast packet (IE fill in the byte fields)
		bool loadBCPkt(const std::vector<uint8_t>&); // Load data into the broadcast packet;
		bool makeUnicastPkt(const address64&, uint8_t fID = 0x01); //Make a unicast packet and address it.
		bool loadUnicastPkt(const std::vector<uint8_t>&); //Load the RF data into the packet. 
		bool sendPkt(); // Send the packet from the internal storage to the outbytes vector. 
		// The control monitoring for ACK messages will not be handled in this class but 
		// rather by the controlling function / process.
		bool ATNDPkt(uint8_t fID = 0x01);
		int processedPktCount() { return _processedPktCount;} 
		xbeeDMapi();
};

class xbeeNeighbors
{
	// This class keeps tracks of the 64-bit addresses of all other nodes in an 
	// xbee network. It does not depend on the type of network, this class would
	// work equally well for Zigbee, 802.15.4, and DigiMesh. So long as all
	// addressing uses a 64-bit address type. 

	private:
		std::<uint8_t>list neighbors;

	public:
		bool update(const address64&);
		int neighborCount();
		address64& operator[](const int);
		bool remove(int);
		bool clear();
}


void outDebug(void);
void inDebug(void);
void inDebugLoopback(void);

#endif
