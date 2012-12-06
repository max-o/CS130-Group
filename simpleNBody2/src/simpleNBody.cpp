/*
 * simpleNBody.cpp
 *
 *  Created on: Oct 1, 2012
 *      Author: martin
 */

#include <stdio.h>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include "MultiNBodyWorld.h"
#include "MultiDomainBuffer.h"
#include <mpi.h>
// Socket includes
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <vector>

using namespace std;

// 5 waiting visualizers seems reasonable
#define QUEUESIZE 5

boost::mutex visLock;				// Lock on visualizer vector
vector<int> registeredVisualizers;	// visualizer vector
int numParticlesPerWorld;			// number of particles to send each cycle
int numParticlesPerDomain;
int numDomains;
int listenSocket;

void cleanup()
{
	// close sockets
	close(listenSocket);
	
	// Still need to lock: but if we can't get it we already have it
	// (Thread can be interrupted while holding the lock)
	visLock.try_lock();
	for(vector<int>::iterator i = registeredVisualizers.begin(); 
		i != registeredVisualizers.end(); i++)
	{
		close(*i);
		registeredVisualizers.erase(i);
	}
	visLock.unlock();
}

void sendDomainData(uint8_t *buf)
{
	bool nope = false;
	for(vector<int>::iterator i = registeredVisualizers.begin(); i != registeredVisualizers.end(); i++)
	{
		uint8_t *sendBuf = buf;
		int connectFD = *i;
		int bytesWritten = 0;
		int bytesToWrite = 3 * numParticlesPerDomain * 4 * sizeof(float);
		while((bytesWritten = send(connectFD, sendBuf, bytesToWrite, MSG_NOSIGNAL)) < bytesToWrite)
		{
			if(bytesWritten < 0)
			{
				// Socket closed: remove from visualizer list
				close(connectFD);
				registeredVisualizers.erase(i);
				i--;
				nope = true;
				fprintf(stderr, "Visualizer %d closed connection\n", connectFD);
				break;
			}
			sendBuf += bytesWritten;
			bytesToWrite -= bytesWritten;
		}
	}
}

void sendCallback(MultiDomainBuffer *mdb, worldBuffer *wBuf)
{
	uint8_t *domBuf = (uint8_t *)(wBuf->pDoms);
	visLock.lock();
	for(int i = 0; i < numDomains; i++)
	{
		sendDomainData(domBuf);
		domBuf++;
	}
	visLock.unlock();
	mdb->UnlockBuffer();
}

void registerVisualizer(int connectFD)
{
	int bytesWritten = 0;
	string np = boost::lexical_cast<string>(numParticlesPerWorld);
	int bytesToWrite = np.length();
	if((bytesWritten = send(connectFD, np.c_str(), bytesToWrite, MSG_NOSIGNAL)) == bytesToWrite)
	{
		// numParticlesPerWorld sent successfully
		int bytesRead = 0;
		char acknowledgement[4];
		char ackConf[4] = "ACK";
		int bytesToRead = 4;
		if((bytesRead = read(connectFD, acknowledgement, bytesToRead)) == bytesToRead)
		{
			if(!strncmp(acknowledgement, ackConf, 4))
			{
				// Acknowledgement recieved
				// Visualizer connected and ready
				visLock.lock();
				registeredVisualizers.push_back(connectFD);
				for(int i = 0; i < registeredVisualizers.size(); i++)
					fprintf(stderr, "Visualizer %d in queue\n",
						 registeredVisualizers[i]);
				visLock.unlock();
				return;
			}
		}
	}
	// Could not read/write bytes from/to visualizer
	close(connectFD);
}

void registrationThreadRoutine()
{
	// Initialize listenPort to 12345
	string listenPort = "12345";
	struct addrinfo hints, *ptr;
	int rv;
	int connectFD;
	struct addrinfo *servinfo;

	try
	{
		// Fill in the hint struct
		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;
	
		// Get the candidate socket info structs
		if ((rv = getaddrinfo(NULL, listenPort.c_str(), &hints, &servinfo)) != 0)
		{
			string s = gai_strerror(rv);
			fprintf(stderr, "getaddrinfo: %s at socket %d\n", s.c_str(), listenSocket);
			throw 'l';
		}
	
		// Iterate through socket info struct list until one is found 
		// which creates a socket and connects
		for (ptr = servinfo; ptr != NULL; ptr = ptr->ai_next)
		{
			listenSocket = socket(ptr->ai_family, ptr->ai_socktype,
								  ptr->ai_protocol);
			if (listenSocket == -1)
			{
				continue;
			}
		
			if (bind(listenSocket, ptr->ai_addr, ptr->ai_addrlen) == -1)
			{
				close(listenSocket);
				continue;
			}
		
			break;
		}
	
		if (ptr == NULL)
		{
			fprintf(stderr, "Could not create the socket\n");
			throw 'l';
		}
	
		// Listen on the acquired socket
		if (listen(listenSocket, QUEUESIZE) == -1)
		{
			fprintf(stderr, "Listening failed on socket %d\n", listenSocket);
			throw 'l';
		}

		// Enter the while loop that handles recieving Visualizers 
		// and adding them to the list to handle them
		while (1)
		{
			// Wait for a connection to be made from a client via the 
			// listening socket
			connectFD = accept(listenSocket, ptr->ai_addr, &(ptr->ai_addrlen));
		
			// If the file decriptor of the connection is valid
			if (connectFD >= 0)
			{
				registerVisualizer(connectFD);
			}
		}
	}
	catch(boost::thread_interrupted thrdEx)
	{
		cleanup();
		return;
	}
	catch(char ex)
	{
		// Listen socket exception
		// No point in simulating if nobody's able to listen
		if(ex == 'l')
		{
			abort();
		}
	}
}



int main(int argc, char** argv)
{
	// Initialize boost threads
	boost::thread registrationThread = boost::thread(registrationThreadRoutine);

	// Initialize MPI
	int rank, numprocs;
	MPI_Comm comm_main;
	MPI_Init(&argc,&argv);
	MPI_Comm_dup(MPI_COMM_WORLD,&comm_main);
	MPI_Comm_rank(comm_main,&rank);

	// Startup
	if(rank==0)	printf("-------------CS130 - Simple NBbody 1.0-------------\n");

	// Initialization Block - Must Destroy theWorld before MPI_Finalize
	MultiNBodyWorld* theWorld=new MultiNBodyWorld();

	theWorld->loadInput(argc,argv);
	theWorld->init(comm_main);
	
	// Get info about number of particles in the world
	MultiDomainBuffer *mdb = theWorld->getMDB();
	numParticlesPerDomain = mdb->getNBodies();
	numDomains = mdb->getNDomains();
	numParticlesPerWorld = numParticlesPerDomain * numDomains;
	
	// Register send callback to domain buffer
	mdb->OnBufferComplete(sendCallback);

	MPI_Barrier(comm_main);

	// Run Block
	if(rank==0)printf("Running NBody Systems\n");
	theWorld->run();

	// Cleanup
	MPI_Barrier(comm_main);
	delete theWorld;

	MPI_Finalize();

	// End registration of PVs and clean up.
	registrationThread.interrupt();
	return (EXIT_SUCCESS);
}











