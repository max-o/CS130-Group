/*
 * Domain.h
 *
 *  Created on: Oct 1, 2012
 *      Author: martin
 */

#ifndef DOMAIN_H_
#define DOMAIN_H_


class World;

class Domain {
public:
	Domain():
		theWorld(NULL),activated(false),rank(-1),gpu(false){}
	Domain(World* inWorld, int inRank=-1, bool useGpu=false):
		theWorld(inWorld),activated(false),rank(inRank),gpu(useGpu){}

	virtual ~Domain(){};

	virtual void activate(){};
	virtual void init()=0;

	bool isActive(){return activated;}
	bool onGpu(){return gpu;}

	int getRank(){return rank;}

	virtual void run(){};
	virtual void step(float dt=0,int di=0){};


protected:

	bool activated; //
	bool gpu;
    int rank;

	World* theWorld;

};

#endif //DOMAIN_H
