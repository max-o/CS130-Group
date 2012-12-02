/*
 * World.h
 *
 *  Created on: Oct 1, 2012
 *      Author: martin
 */

#ifndef WORLD_H_
#define WORLD_H_

#include <mpi.h>
#include <vector>
#include "Domain.h"

class World {

public:
	World(){};
	virtual ~World(){for(int di=0;di<domains.size();di++)delete domains[di];};

    virtual void init(){
    	MPI_Comm incomm_main;
    	MPI_Comm_dup(MPI_COMM_WORLD,&incomm_main);
    	init(incomm_main);
    }
    virtual void init(MPI_Comm incomm_main){
    	comm_main=incomm_main;
    	MPI_Comm_rank(comm_main,&comm_rank);
    }

    virtual void loadInput(int argc, char** argv)=0;

	virtual void run(){};
	virtual void step(float indt){};

	int getIter(){return iter;}
	float getTime(){return time;}
	float getTlim(){return tlim;}
	float getDt(){return dt;}
	MPI_Comm getComm(){return comm_main;}
	int getRank(){return comm_rank;}

	virtual int getNDom(){return domains.size();}
	virtual Domain* getDomain(int di){return domains[di];}

protected:
    // MPI Communicator Information
    MPI_Comm comm_main;
    int comm_rank;

    // Simulation Controls
	int iter;
	float time;
	float tlim;
	float dt;

	// Computational Domains
	std::vector<Domain*> domains;
};


#endif /* WORLD_H_ */

