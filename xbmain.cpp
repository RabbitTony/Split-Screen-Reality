#include <iostream>
#include "xbeeDMapi.h"
#include "TTYserial.h"
#include <thread>
#include <mutex>
#include <vector>
#include <string>
#include <cstdint>
#include <list>
#include <queue>
#include <iterator>

volatile bool threadstart;
volatile bool threadend;
volatile bool threadgotdata;
volatile bool threadsentdata;

void w_tty(void);

int main(void)
{
	xbeeDMapi xb;

	rcvdPacket p;
	xb.zeroPktStruct(p);

	xb.makeBCpkt();
	std::vector<uint8_t> d;
	d.push_back(0x68);
	d.push_back(0x69);
	xb.loadBCpkt(d);
	xb.sendpkt();

	threadstart = false;
	threadend = false;
	threadgotdata = false;
	threadsentdata = false;

	std::thread t(w_tty);

	t.join();

	if (threadsentdata) std::cout << "The data was sent.\n" << "outBytes length: " << outBytes.size() << "\n";

	return 0;
}

void w_tty(void)
{
	TTYserial tty;
	tty.begin("/dev/ttyUSB0", 38400);
	while (!(threadstart)) {}

	outBytesMutex.lock();
	std::list<uint8_t>::iterator iter = outBytes.begin();
	while(iter != outBytes.end())
	{
		tty.sendbyte((unsigned char) *iter);
		iter = outBytes.erase(iter);
	}
	outBytesMutex.unlock();

	threadend = true;
	threadsentdata = true;

	return;
}
