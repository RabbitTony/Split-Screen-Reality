Split-Screen-Reality
====================

The code repository for the Split Screen Reality, USF, senior design project. 

This repository will contain the most recent and up to date code for the project.
The code includes a makefile and is intended to be run on linux (Raspbian). Once
the entire source has been downloaded, the user needs to enter the linux command
"make" while in the same directory as the source code and the linux compiler will 
automatically build and compile the source code. 

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
