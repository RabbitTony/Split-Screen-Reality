#include <iostream>
#include <cstdint>
#include <exception>
#include <string>
#include <vector>
#include <iterator>
#include <queue>
#include <mutex>
#include "xbeeDMapi.h"

std::mutex inBytesMutex, outBytesMutex;
std::list<uint8_t> inBytes;
std::list<uint8_t> outBytes;

address64::address64()
{
	address64bit = 0x00;
	for (int i = 0; i < 8; i++)
	{
		address8bytes[i] = 0x00;
	}
}

address64::address64(uint64_t a64)
{
	address64bit = a64;
	split();
}

address64::address64(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5, uint8_t b6, uint8_t b7, uint8_t b8)
{

	address8bytes[0] = b1;
	address8bytes[1] = b2;
	address8bytes[2] = b3;
	address8bytes[3] = b4;
	address8bytes[4] = b5;
	address8bytes[5] = b6;
	address8bytes[6] = b7;
	address8bytes[7] = b8;
	combine();
}

address64& address64::operator=(const uint64_t &a64)
{
	address64bit = a64;
	split();
	return *this;
}

address64& address64::operator=(const address64 &rhs)
{
	if (this == &rhs) return *this;

	this->address64bit = rhs.address64bit;
	for (int i = 0; i < 8; i++)
	{
		this->address8bytes[i] = rhs.address8bytes[i];
	}

	combine();

	return *this;
}

uint8_t& address64::operator[](const int index)
{
	//To keep the programm running, an out of bounds error will default to 
	//the least significant byte after throwing an appropriate exception.
	if (index < 0 || index > 7)
	{
		throw xb_except("address64[] index out of bounds, operation ignored.");
	}

	return address8bytes[index];
}

bool address64::operator==(const address64 &rhs) const
{
	int i = 0;
	bool result = true;
	while (i < 8 && result == true)
	{
		if (address8bytes[i] != rhs.address8bytes[i]) result = false;
		i++;
	}

	return result;
}

bool address64::operator!=(const address64 &rhs) const
{

	return ! (*this == rhs);
}

void address64::split()
{
	address8bytes[0] = (address64bit >> 56);
	address8bytes[1] = (address64bit >> 48);
	address8bytes[2] = (address64bit >> 40);
	address8bytes[3] = (address64bit >> 32);
	address8bytes[4] = (address64bit >> 24);
	address8bytes[5] = (address64bit >> 16);
	address8bytes[6] = (address64bit >>  8);
	address8bytes[7] = (address64bit >>  0);
}

void address64::combine()
{
	address64bit = 0x00;
	
	address64bit |= ((uint64_t)address8bytes[0] << 56);
	address64bit |= ((uint64_t)address8bytes[1] << 48);
	address64bit |= ((uint64_t)address8bytes[2] << 40);
	address64bit |= ((uint64_t)address8bytes[3] << 32);
	address64bit |= ((uint64_t)address8bytes[4] << 24);
	address64bit |= ((uint64_t)address8bytes[5] << 16);
	address64bit |= ((uint64_t)address8bytes[6] <<  8);
	address64bit |= ((uint64_t)address8bytes[7] <<  0);
}

xbeeDMapi::xbeeDMapi()
{
		_processedPktCount = 0;
		clearPktData();
}
void xbeeDMapi::clearPktData()
{
	_startDelim = 0;
	_lengthMSB = 0;
	_lengthLSB = 0;
	_frameType = 0;
	_frameID = 0x00;
	_destAdr = 0x00;
	_R1 = 0;
	_R2 = 0;
	_bcRad = 0;
	_txOpts = 0;
	if (!(_payLoad.empty())) _payLoad.clear();
	_chkSum = 0;
	_packetType = 0;
	if(!(pktBytes.empty())) pktBytes.clear();
	_pMade = false;
	_pLoaded = false;
}


bool xbeeDMapi::pktAvailable()
{	// NOTE: I am using so many differnt mutex locks/unlocks so that the TTY thread has a chance to cut in. 
	bool result = false;
	if (_processedPktCount > 0) result = true;
	
	//Now we are going to check to make sure we have enough bytes in inBytes for a complete packet. 
	if (inBytes.size() <= 5)
	{
		return result;
	}

	//The following section checks the inBytes buffer for a complete packet
	// and if it finds one, it moves it to the rcvdBytes buffer for later processing.
	
	std::list<uint8_t>::iterator i1, i2;
	inBytesMutex.lock();
	i1 = inBytes.begin();
	inBytesMutex.unlock();

	inBytesMutex.lock();
	while (*i1 != 0x7E && i1 != inBytes.end()) 
	{
		i1 = inBytes.erase(i1); // Drop all bytes that don't have the proper starting value. IE find frame beginning.
	}
	inBytesMutex.unlock();
	inBytesMutex.lock();
	if (i1 == inBytes.end())
	{
		inBytesMutex.unlock();
		return result; //We don't have a packet start deliminator anywhere in the buffer. 	
	}
	inBytesMutex.unlock(); 

	//Now we have the start of the packet, now find the end, if it exists. 

	uint8_t frameLength;
	inBytesMutex.lock();
	i2 = i1; // i1 = 0x7E
	i2++; //Now looking at length-MSB, need to account for escape characters

	if (i2 == inBytes.end()) return false;
	else if (*i2 == 0x7D)
	{
		i2++;
		if (i2 == inBytes.end())
		{
			inBytesMutex.unlock();
			return false;	
		} 

		i2++;
		if (i2 == inBytes.end())
		{
			inBytesMutex.unlock();
			return false;	
		}
	}
	else i2++;

	if (i2 == inBytes.end()) 
	{
		inBytesMutex.unlock();
		return false;
	}

	else if (*i2 == 0x7D)
	{
		i2++;
		if (i2 != inBytes.end())
		{
			frameLength = *i2 ^ 0x20;
		}
		else 
		{
			inBytesMutex.unlock();
			return false;
		}
	}
	else
	{
		frameLength = *i2;
	}
	inBytesMutex.unlock();

	inBytesMutex.lock();
	i2++; //This is where the length amount actually starts, now we need to account for escape characters here as well
	int num = 0;

	if (i2 == inBytes.end()) 
	{
		inBytesMutex.unlock();
		return false;
	}

	while (num < frameLength)
	{
			if (*i2 == 0x7d)
			{
				if (i2 == inBytes.end())
				{
					inBytesMutex.unlock();
					return false;
				}

				i2++;
				if (i2 == inBytes.end())
				{
					inBytesMutex.unlock();
					return false;
				}

				i2++;
			}
			else
			{
				if (i2 == inBytes.end())
				{
					inBytesMutex.unlock();
					return false;
				}
				i2++;
			}

			num++;
	}

	// Now we should be looking at the checksum byte.
	if (*i2 == 0x7D)
	{
		i2++;
		if (i2 == inBytes.end())
		{
			inBytesMutex.unlock();
			return result;
		}
	}

	i2++; // <----- This is one byte past the checksum, and it is okay for it to be the end of the buffer because
	// it is a full packet. 
	inBytesMutex.unlock();


	//Now, we know that we have a complete packet in the inBytes buffer. 

	_processedPktCount++;
	result = true;

	inBytesMutex.lock();
	//Now we are going to move the data from inBytes to rcvdBytes. 
	std::list<uint8_t>::iterator j = i1;
	while(j != i2)
	{
		rcvdBytes.push_back(*j);
		j = inBytes.erase(j);
	}
	inBytesMutex.unlock();
	return result;
}

uint8_t xbeeDMapi::rcvPkt(rcvdPacket &pkt)
{
	if (_processedPktCount <= 0)
	{
		pkt.nopkts = true;
		return 0;
	}
	uint8_t APIframeID;
	std::vector<uint8_t> tbuffer;
	zeroPktStruct(pkt);

	if (rcvdBytes.size() <= 2) //Some how there were two start bytes next to one another.
	{
		_processedPktCount--;
		pkt.nopkts = true;
		return 0;
	}

	std::list<uint8_t>::iterator it = rcvdBytes.begin();
	while(*it != 0x7E && it != rcvdBytes.end()) it = rcvdBytes.erase(it); //Double check...

	if (it == rcvdBytes.end())
	{
		_processedPktCount--;
		pkt.badlength = true;
		return 0;
	}

	it = rcvdBytes.erase(it); // we are now at the length-MSB, gotta check for escape characters.

	if (it == rcvdBytes.end())
	{
		pkt.badlength = true;
		return 0;
	}

	if (*it == 0x7D)
	{
		it = rcvdBytes.erase(it);
		if (it == rcvdBytes.end())
		{
			pkt.badlength = true;
			return 0;
		}
		it = rcvdBytes.erase(it); // We don't care about the MSB because we want packets smaller
		// than the buffer size of 202 bytes anyhow, which means 1 byte can express all possible sizes allowed. 
		if (it == rcvdBytes.end())
		{
			pkt.badlength = true;
			return 0;
		}
	}
	else it = rcvdBytes.erase(it);
	
	if (it == rcvdBytes.end())
	{
		pkt.badlength = true;
		_processedPktCount--;
		return 0;
	}

	//Now we are at the length LSB.
	uint8_t length = 0x00;
	
	if (*it == 0x7D)
	{
		it = rcvdBytes.erase(it);
		if (it == rcvdBytes.end())
		{
			pkt.badlength = true;
			_processedPktCount--;
			return 0;
		}

		if (*it == 0x7E) // Possible if there was an error in available();
		{
			pkt.badlength = true;
			_processedPktCount--;
			return 0;
		}

		length = *it ^ 0x20;
		it = rcvdBytes.erase(it);
		if (it == rcvdBytes.end() || *it == 0x7E)
		{
			_processedPktCount--;
			pkt.badlength = true;
			return 0;
		}

	}
	else
	{
		length = *it;
		it = rcvdBytes.erase(it);
	}

	// At this point we are at the API frame id byte. 
	if (it != rcvdBytes.end() || *it != 0x7E)
	{
		_processedPktCount--;
		pkt.nopkts = false;
		pkt.badlength = false;

		//Now verify the checksum and move the main information to a temp. buffer. 

		if (*it == 0x7D) //we need to grab the frame type...
		{
			it = rcvdBytes.erase(it);
			APIframeID = *it ^ 0x20;
			tbuffer.push_back(APIframeID);
			it = rcvdBytes.erase(it);
		}

		else
		{
			APIframeID = *it;
			tbuffer.push_back(APIframeID);
			it = rcvdBytes.erase(it);
		}

		pkt.pType = APIframeID;

		//Now load the rest of the packet into the buffer, the binary sum of which 
		//should be equal to 0xFF for a proper checksum. 
		while(*it != 0x7E && it != rcvdBytes.end())
		{
			if (*it == 0x7D)
			{
				uint8_t b = 0x00;
				it = rcvdBytes.erase(it);
				b = *it ^ 0x20;
				tbuffer.push_back(b);

			}

			else
			{
				tbuffer.push_back(*it);
			}

			it = rcvdBytes.erase(it);
		}

		uint8_t checksumVerify = 0x00;
		for(std::vector<uint8_t>::iterator cit = tbuffer.begin(); cit != tbuffer.end(); cit++)
		{
			checksumVerify += *cit;
		}

		if (checksumVerify != 0xFF)
		{
			pkt.badchecksum = true;
			return 0;
		}
		else pkt.badchecksum = false;
	}
	else
	{
		pkt.badlength = true;
		while(*it != 0x7E && it != rcvdBytes.end()) it = rcvdBytes.erase(it); //Drop the bad packet.
		return 0;
	}

	//At this point we have the packet in the temp. buffer and we know that the length and checksum are ok.
	//IE: We have a complete frame inside of the tbuffer vector and we now need to process it. (de-escaping has been completed)

	/////////////////////////////////////////////////////////////////////

	//NOTE: The first byte in tBuffer is the frame time. 
	if (APIframeID == APIid_RP) //Receive packet
	{
		//Source address
		pkt.from[0] = tbuffer[1];
		pkt.from[1] = tbuffer[2];
		pkt.from[2] = tbuffer[3];
		pkt.from[3] = tbuffer[4];
		pkt.from[4] = tbuffer[5];
		pkt.from[5] = tbuffer[6];
		pkt.from[6] = tbuffer[7];
		pkt.from[7] = tbuffer[8];
		//Receive options
		pkt.receiveOpts = tbuffer[11];
		//Data length
		pkt.length = (uint8_t)(tbuffer.size() - 13);
		//Load the data:
		for (int i = 12; i < (tbuffer.size() -1); i++)
		{
			pkt.data.push_back(tbuffer[i]);
		}

		return APIframeID;
	}

	else if (APIframeID == APIid_ATCR) // AT command response.
	{
		if (tbuffer[2] == 0x4E && tbuffer[3] == 0x44) // "ND"
		{
			pkt.ATCmd[0] = tbuffer[2];
			pkt.ATCmd[1] = tbuffer[3];
			pkt.from[0] = tbuffer[7];
			pkt.from[1] = tbuffer[8];
			pkt.from[2] = tbuffer[9];
			pkt.from[3] = tbuffer[10];
			pkt.from[4] = tbuffer[11];
			pkt.from[5] = tbuffer[12];
			pkt.from[6] = tbuffer[13];
			pkt.from[7] = tbuffer[14];
			return APIframeID;
		}

		else return 0xFF;
	}

	else if (APIframeID == APIid_TS) //Transmit status
	{
		pkt.txRetryCount = tbuffer[4];
		pkt.deliveryStatus = tbuffer[5];
		return APIframeID;
	}
		
	else return 0xFF;
}

void xbeeDMapi::zeroPktStruct(rcvdPacket &pkt)
{
	pkt.from = 0x00;
	pkt.length = 0x00;
	pkt.pType = 0x00;
	pkt.txRetryCount = 0x00;
	pkt.deliveryStatus = 0x00;
	pkt.receiveOpts = 0x00;
	pkt.nopkts = false;
	pkt.badlength = false;
	pkt.badchecksum = false;
	pkt.ATCmd[0] = pkt.ATCmd[1] = 0x00;
	if (!pkt.data.empty()) pkt.data.clear();
}

bool xbeeDMapi::makeBCPkt(uint8_t fID)
{
	clearPktData();
	if(!(pktBytes.empty())) pktBytes.clear();
	_pMade = false;
	_pLoaded = false;

	_startDelim = 0x7E;
	_lengthMSB = 0x00;
	_lengthLSB = 0x0E;
	_frameType = 0x10;
	_frameID = fID;
	_destAdr = BCadr; //broadcast address constant defined in xbeeDMapi.h
	_R1 = 0xFF;
	_R2 = 0xFE;
	_bcRad = 0x00;
	_txOpts = 0x00;
	if(!(_payLoad.empty())) _payLoad.clear();
	_chkSum = 0xFF - (0x10 + fID + 0xFF + 0xFF + 0xFF + 0xFE);
	_ATCmd[0] = 0;
	_ATCmd[1] = 0;

	_pMade = true;
	return true;
}

bool xbeeDMapi::loadBCPkt(const std::vector<uint8_t> &pktData)
{
	if (_pMade == false) return false;
	if(pktData.size() >= 180) return false;
	_lengthLSB += (uint8_t)pktData.size();
	if (pktData.empty()) return true;
	if (!(_payLoad.empty())) _payLoad.clear();
	for (int i = 0; i < pktData.size(); i++)
	{
		_payLoad.push_back(pktData[i]);
		_chkSum -= pktData[i];
	}

	_pLoaded = true;
	return true;
}

bool xbeeDMapi::makeUnicastPkt(const address64 &dest, uint8_t fID)
{
	clearPktData();
	if(!(pktBytes.empty())) pktBytes.clear();
	_pMade = false;
	_pLoaded = false;

	_startDelim = 0x7E;
	_lengthMSB = 0x00;
	_lengthLSB = 0x0E;
	_frameType = 0x10;
	_frameID = fID;
	_destAdr = dest;
	_R1 = 0xFF;
	_R2 = 0xFE;
	_bcRad = 0x00;
	_txOpts = 0x00;
	if(!(_payLoad.empty())) _payLoad.clear();
	_chkSum = 0xFF - (0x10 + fID + 0xFF + 0xFE + _destAdr[0] + _destAdr[1] + _destAdr[2]
			+ _destAdr[3] + _destAdr[4] + _destAdr[5] + _destAdr[6] + _destAdr[7]);
	_ATCmd[0] = 0;
	_ATCmd[1] = 0;

	_pMade = true;
	return true;
}

bool xbeeDMapi::loadUnicastPkt(const std::vector<uint8_t> &pktData)
{
	_pLoaded = true;
	return loadBCPkt(pktData);
}

bool xbeeDMapi::sendPkt()
{
	if (_pMade == false || _pLoaded == false) 
	{
		return false;
	}

	if (!(pktBytes.empty())) pktBytes.clear();

	if (_frameType == 0x10)
	{
		pktBytes.push_back(_startDelim);
		pktBytes.push_back(_lengthMSB);
		pktBytes.push_back(_lengthLSB);
		pktBytes.push_back(_frameType);
		pktBytes.push_back(_frameID);
		pktBytes.push_back(_destAdr[0]);
		pktBytes.push_back(_destAdr[1]);
		pktBytes.push_back(_destAdr[2]);
		pktBytes.push_back(_destAdr[3]);
		pktBytes.push_back(_destAdr[4]);
		pktBytes.push_back(_destAdr[5]);
		pktBytes.push_back(_destAdr[6]);
		pktBytes.push_back(_destAdr[7]);
		pktBytes.push_back(_R1);
		pktBytes.push_back(_R2);
		pktBytes.push_back(_bcRad);
		pktBytes.push_back(_txOpts);
		for(std::vector<uint8_t>::iterator iter = _payLoad.begin(); iter != _payLoad.end(); iter++)
		{
			pktBytes.push_back(*iter);
		}
		pktBytes.push_back(_chkSum);
	}

	else if (_frameType == 0x08)
	{
		pktBytes.push_back(_startDelim);
		pktBytes.push_back(_lengthMSB);
		pktBytes.push_back(_lengthLSB);
		pktBytes.push_back(_frameType);
		pktBytes.push_back(_frameID);
		pktBytes.push_back((uint8_t)_ATCmd[0]);
		pktBytes.push_back((uint8_t)_ATCmd[1]);
		pktBytes.push_back(_chkSum);
	}

	else return false;

	outBytesMutex.lock(); // Now move the packet bytes into the outgoing buffer and do escaping when needed. 
	for (std::vector<uint8_t>::iterator iter = pktBytes.begin(); iter != pktBytes.end(); iter++)
	{
		if (*iter == 0x7E || *iter == 0x7D || *iter == 0x11 || *iter == 0x13)
		{
			if(iter != pktBytes.begin())
			{
				outBytes.push_back(0x7D);
				outBytes.push_back(*iter ^ 0x20);
			}
			else
			{
				outBytes.push_back(*iter);
			}
		}
		else
		{
			outBytes.push_back(*iter);
		}
	}
	outBytesMutex.unlock();

	return true;
}

bool xbeeDMapi::ATNDPkt(uint8_t fID)
{
	clearPktData();

	_startDelim = 0x7E;
	_lengthMSB = 0x00;
	_lengthLSB = 0x04;
	_frameType = 0x08;
	_frameID = fID;
	_ATCmd[0] = 'N';
	_ATCmd[1] = 'D';
	_chkSum = 0xFF - (_frameType + _frameID + (uint8_t)_ATCmd[0] + (uint8_t)_ATCmd[1]);

	_pLoaded = true;
	_pMade = true;
	return true;
}

bool xbeeNeighbors::update(const address64 &adr)
{
	if (_neighbors.empty()) 
	{
		_neighbors.push_back(adr);
		return true;
	}

	bool check = true;
	std::vector<address64>::iterator it = _neighbors.begin();
	while(check == true && it != _neighbors.end())
	{
		if(adr == *it) check == false;
		it++;
	}

	if (check) _neighbors.push_back(adr);
	else return false;

	return true;
}

int xbeeNeighbors::neighborCount()
{
	return _neighbors.size();
}

int xbeeNeighbors::numberOfNeighbor(const address64 &adr)
{
	if (_neighbors.empty()) return -1;

	int num = 0;
	bool found = false;
	std::vector<address64>::iterator it = _neighbors.begin();
	while (it != _neighbors.end())
	{
		if (adr == *it) 
		{
			return num;
			found = true;
		}
		num++;
		it++;
	}

	if (found == false) return -1;
	return -2;
}

address64& xbeeNeighbors::operator[](const int index)
{
	
	if (_neighbors.size() <= 0)
	{
		address64 ra;
		ra = 0x00;
	}
	if (index < 0) return _neighbors[0];
	if (index >= _neighbors.size()) return _neighbors[_neighbors.size()-1];

	return _neighbors[index];
}

bool xbeeNeighbors::remove(int n)
{
	if (n < 0 || n >= _neighbors.size()) return false;
	_neighbors.erase(_neighbors.begin() + n);
	return true;
}

bool xbeeNeighbors::clear()
{
	_neighbors.clear();
	return true;
}

///////////////////////////////////////// Debugging functions ////////////////////////////////

void outDebug(void)
{
	if (outBytes.empty())
	{
		std::cout << "outBytes buffer is empty.\n";
		return;
	}

	else
	{
		std::cout << "Hexidecimal contents of outBytes(total of " << outBytes.size() << " bytes) is:\n";
		for (std::list<uint8_t>::iterator it = outBytes.begin(); it != outBytes.end(); it++)
		{
			printf("%x\n",*it);
		}
	}

	return;
}

void inDebug(void)
{
	if (inBytes.empty())
	{
		std::cout << "inBytes buffer is empty.\n";
		return;
	}

	else
	{
		std::cout << "Hexidecimal contents of inBytes is:\n";
		for (std::list<uint8_t>::iterator it = inBytes.begin(); it != inBytes.end(); it++)
		{
			printf("%x\n",*it);
		}
	}

	return;
}

void inDebugLoopback(void)
{
	for (std::list<uint8_t>::iterator it = outBytes.begin(); it != outBytes.end(); it++)
	{
		inBytes.push_back(*it);
	}
	return;
}
