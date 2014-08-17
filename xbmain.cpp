#include <iostream>
#include "xbeeDMapi.h"
#include "TTYserial.h"
#include <mutex>
#include <thread>
#include <vector>
#include <iterator>

/*
   Test 1 (17 Auguest, 2014)
   	The purpose of this test is to do a complete test of all classes/functions
	written thus far. 

	Functions under test / results:

	xbeeDMapi :
		rcvPkt
		(make/load)BCPkt
		(make/load)UnicastPkt
		ATNDPkt
	xbeeNeighbors :
		update
		neighborCount
		operator[]
		remove
		clear
*/

int main()
{
	std::cout << "Hello world." << std::endl;
	return 0;
}

