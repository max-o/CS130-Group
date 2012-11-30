Updated!
Installation of libraries on Ubuntu
1. go to the lib/ directory and configure and make the files
	- in the freeglut directory,
		- configure, then make
			./configure
			make all
			make install all
	- in the glew directory,
		- EVEN EASIER! JUST MAKE AND INSTALL!
			make all
			make install all
	- in the boost directory
		- More complicated: tell the installer which libraries to install
		then run the installer
			./bootstrap.sh --with-libraries=thread
			./b2
			(Won't be installed on your system, just into that directory)
2. head over to the PV directory and just make it
			make
	- There is a shortcut to the Release version of the built Visualizer. To run it, type 
			./PV
	- if there are any problems with getting it all set up let me know and I'll take a look. 
	- I'm on Ubuntu 12.04 Precise Pangolin so if y'alls is using another distro, PRAY that it works...

