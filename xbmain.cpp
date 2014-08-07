#include <iostream>
#include <list>
#include <vector>
#include <iterator>
#include <cstdint>
#include <string>
#include <queue>
#include "xbeeDMapi.h"

int main(void)
{
	xbeeDMapi xb;

	std::vector<uint8_t> vtest;

	vtest.push_back(0x68);
	vtest.push_back(0x69);
	vtest.push_back(0x7E);

	xb.makeBCpkt();
	xb.loadBCpkt(vtest);
	xb.sendpkt();

	outDebug();

	inDebugLoopback();

	if (xb.pktAvailable()) std::cout << "Found packet\n";
	rcvdPacket p;
	xb.zeroPktStruct(p);

	inDebug();
	uint8_t rv = xb.rcvPkt(p);
	if (rv)
	{
		printf("Rv: %x\n", rv);
		std::cout << "Found packet of type: " << std::hex << p.pType << std::endl;
		std::cout << "data byte numbers: " << p.data.size() << "\n";
		std::cout << "With data of:\n";
		std::vector<uint8_t>::iterator it = p.data.begin();
		while(it != p.data.end())
		{
			printf(":%x:\n", *it);
			it++;
		}

		for (int i = 0; i < 8; i++) printf("---%x---\n", p.from[i]);

	}
	else
	{
		std::cout << "Badlength: " << p.badlength << "\n";
		std::cout << "Badchecksum: " << p.badchecksum << "\n";
		std::cout << "nopkts: " << p.nopkts << "\n";
	}

	std::cout << "the number of bytes in outbytes is: " << outBytes.size() << std::endl;

	return 0;
}

