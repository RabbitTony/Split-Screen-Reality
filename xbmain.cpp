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

int main(int argc, char* argv[])
{
	//Process command-line arguments

	//Start threads

	//Clean up and exit

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

	return;
}

void RFMonitor_main(void)
{
	//Start running. 

	return;
}

void UIMonitor_main(void)
{
	//Start running. 

	return;
}

char VCR_threaded::play(const RFPacketRequest& invid)
{
	//If recording, fail
	//Set playing and store current request. 

	return 0;
}

char VCR_threaded::record(const RFPacketRequest& outvid)
{
	//If playing, fail
	//Set recording and store current request. 

	return 0;
}

void VCR_threaded::VCRMain(void)
{
	
	//Looping
	//if playing and full video recieved, play the segment
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

