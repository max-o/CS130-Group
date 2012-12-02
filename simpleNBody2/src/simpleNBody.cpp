/*
 * simpleNBody.cpp
 *
 *  Created on: Oct 1, 2012
 *      Author: martin
 */

#include <stdio.h>
#include "MultiNBodyWorld.h"
#include <mpi.h>

int main(int argc, char** argv)
{
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

	MPI_Barrier(comm_main);

	// Run Block
	if(rank==0)printf("Running NBody Systems\n");
	theWorld->run();

	// Cleanup
	MPI_Barrier(comm_main);
	delete theWorld;

	MPI_Finalize();
	return (EXIT_SUCCESS);
}











