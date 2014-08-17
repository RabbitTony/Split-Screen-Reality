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

	private:
		int BAUD;
		std::string PORT;
		termios old_ts, new_ts;
		bool OPENED;
		int fd;

	public:
		TTYserial() { OPENED = false; }
		bool begin(std::string, int);
		bool end(void);
		int available(void);
		bool sendbyte(unsigned char);
		unsigned char readbyte(void);
		bool status(void);
		~TTYserial();
		
};

#endif
