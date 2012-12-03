/*
 * multiNBodyWorld.h
 *
 *  Created on: Oct 9, 2012
 *      Author: martin
 */


#ifndef MULTINBODYWORLD_H_
#define MULTINBODYWORLD_H_

#include "World.h"
#include <cstdlib>

//added
extern float buffer[8][4096*4];
extern bool flag[8];
//end
class NBodyParams;

class MultiNBodyWorld: public World {
public:
	MultiNBodyWorld();
	virtual ~MultiNBodyWorld();

	virtual void init(MPI_Comm comm_main){
		World::init(comm_main);
		init();
	}

	virtual void init();
	virtual void loadInput(int argc, char** argv);
	virtual void run(){while(time<tlim){step(dt);time+=dt;iter++;}}

	virtual void step(float indt){
		if(comm_rank==0)printf("Stepping World It=%d at t=%f with dt=%e\n",iter,time,indt);
		for(int di=0;di<getNDom();di++)
            //added
        {
            domains[di]->step(indt,di);
            
                if(flag[di])
                
                printf("Domain %d: %f,%f,%f,%f\n",di,buffer[di][0],buffer[di][0*4+1],buffer[di][0*4+2],buffer[di][0*4+3]);
            
        
            
        }
        //we can send data out here and attach the iteration number
        //[data][iteration number]
            //end
	}

private:
	int numBodies;

	NBodyParams* demoParams;	// Set of Initial Condition Parameters from SDK Demo
	int nDemoParams;			// Number of Demo Parameters to Select From
	int nDoms;					// Number of Domains to Initialize from Command Line
	bool useGpu;				// Use of GPU flag from Command Line

};

#endif /* MULTINBODYWORLD_H_ */
