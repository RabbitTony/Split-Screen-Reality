#include <iostream>
#include <string>
#include <termios.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include "TTYserial.h"
#include <string.h>

#define _POSIX_SOURCE 1
#define FALSE 0
#define TRUE 1

bool TTYserial::begin(std::string modem, int baud)
{
	if (OPENED == true) return OPENED;

	fd = open(modem.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK); // Open the port

	if (fd <= 0) return OPENED; //Return on error. Opened is still false. 

	OPENED = true;

	BAUD = baud;
	PORT = modem;


	tcgetattr(fd, &old_ts); // Read the old terminal settings to restore later
	memset(&new_ts, 0, sizeof(new_ts));
	new_ts.c_cflag = CS8|CLOCAL|CREAD|(new_ts.c_cflag & ~CSIZE);
	
	if (baud == 1200)
	{
		std::cout << "Setting 1200 baud\n";
		cfsetospeed(&new_ts, B1200);
		cfsetispeed(&new_ts, B1200);
	}
	else if (baud == 9600)
	{
		cfsetospeed(&new_ts, B9600);
		cfsetispeed(&new_ts, B9600);
	}
	else if (baud == 19200)
	{
		cfsetospeed(&new_ts, B19200);
		cfsetispeed(&new_ts, B19200);
		new_ts.c_cflag |= B19200;
	}
	else if (baud == 38400)
	{
		new_ts.c_cflag |= B38400;
	}
	else if (baud == 57600)
	{
		cfsetospeed(&new_ts, B57600);
		cfsetispeed(&new_ts, B57600);
	}
	else if (baud == 115200)
	{
		cfsetospeed(&new_ts, B115200);
		cfsetispeed(&new_ts, B115200);
	}
	else
	{
		cfsetospeed(&new_ts, B9600);
		cfsetispeed(&new_ts, B9600);
		BAUD = 9600;
	}

	new_ts.c_iflag = 0;
	new_ts.c_oflag = 0;
	new_ts.c_lflag = 0;

	new_ts.c_cc[VINTR]    = 0;     /* Ctrl-c */ 
	new_ts.c_cc[VQUIT]    = 0;     /* Ctrl-\ */
	new_ts.c_cc[VERASE]   = 0;     /* del */
	new_ts.c_cc[VKILL]    = 0;     /* @ */
	new_ts.c_cc[VEOF]     = 4;     /* Ctrl-d */
	new_ts.c_cc[VTIME]    = 0;     /* inter-character timer unused */
	new_ts.c_cc[VMIN]     = 1;     /* blocking read until 1 character arrives */
	new_ts.c_cc[VSWTC]    = 0;     /* '\0' */
	new_ts.c_cc[VSTART]   = 0;     /* Ctrl-q */ 
	new_ts.c_cc[VSTOP]    = 0;     /* Ctrl-s */
	new_ts.c_cc[VSUSP]    = 0;     /* Ctrl-z */
	new_ts.c_cc[VEOL]     = 0;     /* '\0' */
	new_ts.c_cc[VREPRINT] = 0;     /* Ctrl-r */
	new_ts.c_cc[VDISCARD] = 0;     /* Ctrl-u */
	new_ts.c_cc[VWERASE]  = 0;     /* Ctrl-w */
	new_ts.c_cc[VLNEXT]   = 0;     /* Ctrl-v */
	new_ts.c_cc[VEOL2]    = 0;     /* '\0' */

	new_ts.c_cc[VTIME] = 0;
	new_ts.c_cc[VMIN] = 1;

	// Now flush the modem port and apply the new settings.

	tcflush(fd, TCIFLUSH);
	if (tcsetattr(fd, TCSANOW, &new_ts) == -1)
	{
		OPENED = false;
		std::cout << "There was a problem setting the port settings.\n";
		close(fd);
		return OPENED;
	}
	else
	{
		return OPENED;
	}		
}

int TTYserial::available(void)
{
	int bytes = 0;
	ioctl(fd, FIONREAD, &bytes);
	return bytes;
}

bool TTYserial::sendbyte(unsigned char byte)
{
	int n  = -1;
	n = write(fd, &byte, 1);
	if (n != 1) return false;
	return true;
}

unsigned char TTYserial::readbyte(void)
{
	if (OPENED)
	{
		char byte = 0;
		read(fd, &byte, 1);
		return byte;
	}

	else return 0;
}

bool TTYserial::status(void)
{
	return OPENED;
}

bool TTYserial::end(void)
{
	if (OPENED == true)
	{
		tcsetattr(fd,TCSANOW, &old_ts);
		close(fd);
		return true;
	}

	else return false;

	return false; // in case of errors or un-forseen problems.
}

TTYserial::~TTYserial()
{
	tcsetattr(fd, TCSANOW, &old_ts);
	close(fd);
}
