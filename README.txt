Setup Instructions

1. Go to libraries directory (lib) and run the configure script
		cd lib
		./configure
	This script runs the configuration for all the libraries with the proper 
	options so you don't have to.
	
2. Now set your linker path include the libraries either on your own or by 
executing the following command:

		export  LD_LIBRARY_PATH=$PWD/boost_1_52_0/stage/lib:$PWD/freeglut-2.6.0/install/lib:$PWD/glew-1.9.0/lib:\$LD_LIBRARY_PATH

===============================================================================

Note: This is for configuration of the Visualizer: to configure the 
Simulation, the cuda sdk must be installed to /usr/local/cuda
