/*
 * MultiNBodyDomain.h
 *
 *  Created on: Oct 15, 2012
 *      Author: martin
 */

#ifndef MULTINBODYDOMAIN_H_
#define MULTINBODYDOMAIN_H_

#include "Domain.h"
#include <stdio.h>

enum NBodyConfig
{
    NBODY_CONFIG_RANDOM=0,
    NBODY_CONFIG_SHELL,
    NBODY_CONFIG_EXPAND,
    NBODY_NUM_CONFIGS
};

struct NBodyParams {
	float m_clusterScale;
	float m_velocityScale;
	float m_softening;
	float m_softeningSquared;
	float m_damping;
	float m_x;
	float m_y;
	float m_z;

	NBodyParams(){};
	NBodyParams(float clusterScale, float velocityScale, float softening,float damping, float pointSize, float x, float y, float z):
					m_clusterScale(clusterScale),
					m_velocityScale(velocityScale),
					m_softening(softening),
					m_damping(damping),
					m_x(x),
					m_y(y),
					m_z(z) {m_softeningSquared=m_softening*m_softening;}
	void print()
	{
		printf("{ %f, %f, %f, %f, %f, %f, %f},\n",
			   m_clusterScale, m_velocityScale,
			   m_softening, m_damping,  m_x, m_y, m_z);
	}
	void randomizeBodies(NBodyConfig config);
};



class MultiNBodyDomain : public Domain {
public:
	MultiNBodyDomain();//:Domain(),np(0);
	MultiNBodyDomain(World* inWorld,  NBodyParams& inparam, int inRank=-1, int innp=0, bool useGpu=false):
		Domain(inWorld,inRank,useGpu),param(inparam),np(innp){}
	virtual ~MultiNBodyDomain();
	virtual void activate();
	virtual void setParams(NBodyParams* inparam){param=*inparam;}
	virtual void init();

	virtual void step(float indt);

private:

	void randomizeBodies(int config);
	void computeNBodyGravitation();
	void gpuComputeNBodyGravitation();
	void bodyBodyInteraction(float accel[3], float posMass0[4], float posMass1[4], float softeningSquared);
	void integrateNBodySystem(float deltaTime);
	void gpuIntegrateNBodySystem(float deltaTime);

	// Host Pointers
	float* h_pos;		//Particle Positions Array on Host
	float* h_mom;		//Particle Momentum Array on Host
	float* h_force;		//Particle Force Array on Host

	// Device Pointers
	float* d_pos[2];	//Particle Position Arrays on Device - Old/New Buffers
	float* d_mom;		//Particle Momentum Array on Device
	float* d_force;		//Particle Force Array on Device

	int np; 			// Number of Particles
	NBodyParams param; 	// Current Parameter Set

	unsigned int dev; 	// Selected CUDA Device
};

#endif /* MULTINBODYDOMAIN_H_ */
