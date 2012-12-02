/*
 * multiNBodyWorld.cpp
 *
 *  Created on: Oct 9, 2012
 *      Author: martin
 */

#include "MultiNBodyWorld.h"
#include "MultiNBodyDomain.h"
#include <string.h>
#include <math.h>

MultiNBodyWorld::MultiNBodyWorld()
{
	// TODO Auto-generated constructor stub

	// Default Setup
	numBodies = 4096;
	nDoms=1;
	useGpu=false;
	tlim=1.0;
	dt=1e-3;

	// Demo Conditions from SDK Exampels
	nDemoParams=7;
	demoParams=new NBodyParams[nDemoParams];
	demoParams[0]=NBodyParams( 1.54f,8.0f,0.1f,1.0f,1.0f,0.0f, -2.0f, -100.0f);
	demoParams[1]=NBodyParams( 0.68f, 20.0f, 0.1f, 1.0f, 0.8f, 0.0f, -2.0f, -30.0f);
	demoParams[2]=NBodyParams( 0.16f, 1000.0f, 1.0f, 1.0f, 0.07f, 0.0f, 0.0f, -1.5f);
	demoParams[3]=NBodyParams( 0.16f, 1000.0f, 1.0f, 1.0f, 0.07f, 0.0f, 0.0f, -1.5f);
	demoParams[4]=NBodyParams( 0.32f, 276.0f, 1.0f, 1.0f, 0.07f, 0.0f, 0.0f, -5.0f);
	demoParams[5]=NBodyParams( 0.32f, 272.0f, 0.145f, 1.0f, 0.08f, 0.0f, 0.0f, -5.0f);
	demoParams[6]=NBodyParams( 6.040000f, 0.000000f, 1.000000f, 1.000000f, 0.760000f, 0.0f, 0.0f, -50.0f);
}

MultiNBodyWorld::~MultiNBodyWorld()
{
	// TODO Auto-generated destructor stub
	if(demoParams)delete[] demoParams;
}

void MultiNBodyWorld::init(){
	iter=0;
	time=0.0;

	int n_proc;
	MPI_Comm_size(comm_main,&n_proc);
	sqrt(3.0);
	int dom_edge=ceil(exp(log( ((double)nDoms)/3.0 ))); // Ceil 3rd-root of #-processors - 3d Domain array

	for(int di=0;di<nDoms;di++){
		int dom_rank=di%n_proc; // Boring but important if there were more domains that Processors
		int parami=di%nDemoParams;
		NBodyParams initParams=demoParams[parami];
		int k=di/(dom_edge*dom_edge); // Init domains in dom_edge^3 cube
		int ij=di%(dom_edge*dom_edge);
		int j=ij/(dom_edge);
		int i=ij%(dom_edge);
		initParams.m_x+=10.0*i*initParams.m_clusterScale;
		initParams.m_y+=10.0*j*initParams.m_clusterScale;
		initParams.m_z+=10.0*k*initParams.m_clusterScale;

		domains.push_back((Domain*)new MultiNBodyDomain(this,initParams,dom_rank,numBodies,useGpu));
	}
	for(int di=0;di<domains.size();di++){
		domains[di]->activate();
		domains[di]->init();
		//printf("Dom[%d] on Rank[%d]: Active=%s UseGPU=%s \n",di,comm_rank,domains[di]->isActive()?"T":"F",domains[di]->onGpu()?"T":"F");
	}
}

void MultiNBodyWorld::loadInput(int argc, char **argv)
{
	MPI_Comm_rank(MPI_COMM_WORLD,&comm_rank); // Default ranks for single output - overwritten in init


	for(int ai=1;ai<argc;ai++){

	        if(strcmp("-help",argv[ai])==0){
			if(comm_rank==0)
				printf("Running Multiple NBody Simulations: \n"
						"\t-help This message\n"
						"\t-use_gpu Enable GPU Processing\n"
						"\t-np=# Number of Particles\n"
						"\t-nd=# Number of Domains\n"
						"\t-dt=# Timestep\n"
						"\t-tl=# Time Limit (end)\n");
			exit(0);
		}
		if(strcmp("-use_gpu",argv[ai])==0)useGpu=true;

		char name[80];
		int val;
		sscanf(argv[ai],"-%2s=%d",name,&val);

		if(strcmp(name,"np")==0){
			int np=32*((val+31)/32);
			if(np!=val)printf("WARNING: Number of Particles set to %d so that np%32=0\n",np);
			numBodies=np;
		}
		if(strcmp(name,"nd")==0)nDoms=val;

		float fval;
		sscanf(argv[ai],"-%2s=%f",name,&fval);
		if(strcmp(name,"dt")==0)this->dt=fval;
		if(strcmp(name,"tl")==0)this->tlim=fval;

	}
	if(comm_rank==0)printf("Setting MultiNBodyWorld to %d Bodies and %d Domains %susing GPU to T=%f\n",numBodies,nDoms,useGpu?"":"not ",tlim);
}
