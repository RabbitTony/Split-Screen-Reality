Split-Screen-Reality
====================

The code repository for the Split Screen Reality, USF, senior design project. 

This repository will contain the most recent and up to date code for the project.
The code includes a makefile and is intended to be run on linux (Raspbian). Once
the entire source has been downloaded, the user needs to enter the linux command
"make" while in the same directory as the source code and the linux compiler will 
automatically build and compile the source code. This allows quick downloading and
installing of the source code for the xbee to the raspberry pi. 

USAGE
=====

The following steps outline the way this repository can be used to create the 
neccessary files on the Raspbery Pi for the xbee and serial classes are:

	1) From https://github.com/aatyler/Split-Screen-Reality download the entire 
	   repository with the "Download Zip" link/button on the right hand side.

	2) Using the SD card from the R-pi, unzip the file from (1) into the desired
	   directory.

	3) Using the shell/command line/terminal window, move to the directory from (2)
	   and enter the linux command "make" 

		3.a) The linux make command will automatically compile the source
		     code into a file called "xbmain" which will be the exectuable. 
		     This executable will contain the most current test that we have
		     for the project. For example, the last test is to send a single
		     broadcast message to all xbee's nearby. As the code for this 
		     project grows, xbmain will eventually be the command that is used
		     to run the wireless part of the project. 

Source Code Documentation
=========================

This section will outline the classes and how they function. This documentation
will be kept as upto date as possible, though, will most likely be forgotten until
the end of the project and the final version is being documented. 


#xbeeDMapi:#

* Is a class that prepares packets for transmission and interprets received 
  packets. In its header two lists are defined, inBytes & outBytes. These are
  byte streams for ingoing and outgoing bytes.

* The class is designed to be thread safe and to utilize the inBytes and outBytes
  buffers, which have been globally defined in xbeeDMapi.h. The purpose for this is
  to allow any function one wants to make the calls to the serial port and load/empty
  the buffers. 

* Each API frame type has a make/load method. For example, to prepare a broad-
  cast frame you would do xb.makeBCpkt() followed by xb.loadBCpkt(data). 

* The send method takes the last loaded packet and puts the bytes in the outgoing
  buffer. 

* In order to use this class, the class/function responsible for talking to the xbee
  on the serial port must write its data to the buffers defined in xbeeDMapi.h. These
  buffers are of the type std::list. The class is written to be thread safe, so
  threading a worker to monitor the serial port is possible. The mutexes used in all
  of the functions are also defined in xbeeDMapi.h, called inBytesMutex and
  outBytesMutex. 

##Methods##

**Constructor**

The constructor initializes the the variables, but does nothing else. 
*example: xbeeDMapi xb;*

**pktAvailable()**

This checks the inBytes (std::list buffer, defined in xbeeDMapi.h) and 
looks for complete packets. Once it knows that there is a complete packet in buffer
it will return true, otherwise it will return false. 
*example: if (xb.pktAvailable()) { // do stuff }*

**rcvPkt(rcvdPacket &pkt)** 

This function moves the data from the inBytes buffer (a std::list) to the packet
passed in and returns the packet type as a uint8_t. The pkt structure is defined in
xbeeDMapi.h. It's attributes will be examined later, in detail. 
*example: if (xb.rcvPkt(rcvdPacket &pkt) { //do stuff}*

**zeroPktStruct(rcvdPacket &pkt)**

This function ensures all values inside of the packet are zeroed. This is also done
when rcvPkt is called, but, is added for good measure. 

**makeBCPkt(uint8_t fID = 0x01)**

This function preps the header for the broadcast packet (address of
0x000000000000FFFE). The frame ID is defaulted to 0x01. 
*example: xb.makeBCPkt(0x03);*

**loadBCPkt(const std::vector<uint8_t> &data)**

This function takes a vector as an input from which it copies the data to the
packet's buffer and calculates the checksum. 

**makeUnicastPkt(const address64 &dest, uint8_t fID = 0x01)**

This function is the same concept as loadBCPkt, dest is the address of the
desitnation node. The address64 class will be explained later in detail. The fID is
defaulted to 0x01, but can be changed as desired. 

**loadUnicastPkt(const std::vector<uint8_t> &data)**

This function is exactly the same as loadBCPkt. Infact, internally, it just passes
the data to the loadBCPkt function. It is included to make the code more readable and
understandable. 

**ATNDPkt()**

Calling this function makes and loads an AT command packet. It already sets up the
packet to be a "ND" (network discover) packet. 

**sendPkt()**

If a data packet is being sent, then once make/load methods have been called, this
function transfers all data from the internal buffer and pushes it out (with
escaping) to the outBytes buffer. 

#TTYserial:#

* A class for dealing with the serial port. It is called with a port and baudrate. 

* It works similar to arduino's serial library. TTY serial provides an available(),
  sendbyte(), and readbyte() function that do as their names suggest. 

This section will be expanded to have a reference for all functions / variables contained with
in the classes listed above, but at a later date. 
