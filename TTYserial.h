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

#ifndef _TTYserial
#define _TTYserial

#define _POSIX_SOURCE 1
#define TRUE 1
#define FALSE 0

class TTYserial {
// ToDo: Need to add a begin function that takes does what the constructor currently does so that a return
// value can be checked for success. Also need a default constructor to call. Perhaps change constructor
// to simple default. 

	private:
		int BAUD;
		std::string PORT;
		termios old_ts, new_ts;
		bool OPENED;
		int fd;

	public:
		TTYserial(std::string, int);
		int available(void);
		bool sendbyte(unsigned char);
		unsigned char readbyte(void);
		bool status(void);
		~TTYserial();
		
};

#endif
