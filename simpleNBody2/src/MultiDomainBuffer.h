/*
 * MultiDomainBuffer.h
 * 
 * Utility class used for triple-buffering domain data.
 */

#ifndef MULTIDOMAINBUFFER_H_
#define MULTIDOMAINBUFFER_H_

#include <cstdlib>
#include <boost/thread.hpp>

// using triple-buffering
#define NUM_DOMAIN_BUFFERS 3

// currently 4 floats are used per particle: x, y, z, mass
#define FLOATS_PER_BODY 4

// structure to store the information for each domain
typedef struct _domainBuffer {
   int nIter; // the simulation iteration this correponds to

   // a pointer to the floats representing particle data, in the form of a float array
   // the number of elements is the same as the number of bodies*4, since 4 floats
   // are used to represent each particle
   float * pBuf; 
} domainBuffer;

// structure to store the information for the simulation world
typedef struct _worldBuffer {
   int nIter; // the simulation iteration we're tracking in the buffer

   // a pointer to all the buffers used to store domain information, in the form of a 
   // domainBuffer struct array
   // the number of elements is the same as the number of domains
   domainBuffer * pDoms; 
} worldBuffer;

// forward declaration for compiling purposes, overridden below
class MultiDomainBuffer;

// Typedef for the callback function ptr, which should have the following prototype:
// void fnName (MultiDomainBuffer *, worldBuffer *)
// when invoked, it will be passed a pointer to the MultiDomainBuffer, and a pointer to the 
// worldBuffer struct that was just completed
typedef void(*pBufferCompleteFn)(MultiDomainBuffer *, worldBuffer *);

class MultiDomainBuffer {
public:
   // constructor 
   // nBods: the number of bodies (particles) in each domain
   // nDoms: the number of domains in each world
   // all domains must have the same number of bodies
	MultiDomainBuffer(int nBods, int nDoms);
	~MultiDomainBuffer();

   // Function to buffer a new set of data
   // fBods: an array of floats, with FLOATS_PER_BODY*nBodies elements
   // nIter: the iteration of the simulation (0-based indexing)
   // nDom: the domain number (0-based indexing)
	void Buffer(float *fBods, int nIter, int nDom);

   // function to register a callback that will be invoked the next time
   // a simulation iteration step has been fully buffered
   // The buffer will be locked from the time the callback is invoked until the UnlockBuffer()
   // function is called.  The caller should call UnlockBuffer() as soon as he is finished
   // processing the completed buffer.
   void OnBufferComplete(pBufferCompleteFn);

   // function that should be called to unlock the completed buffer
   // this must be called before the OnBufferComplete callback function will be invoked again
   void UnlockBuffer();

   // accessor function to get the number of bodies per domain
   int getNBodies();

   // access function to get the number of domains per world
   int getNDomains();

private:
	int nBods; // number of bodies
	int nDoms; // number of domains
	worldBuffer * pWorldBuf; // array of world buffers
   pBufferCompleteFn callback; // callback function registered by the caller
   bool bufferLocked; 
   boost::mutex guard;
   
   // index variables for tracking the buffer states
   int completeBuffer, iterLockedBuffer, iterUnlockedBuffer; 

   // private helper function to test if a buffer is complete
   bool isBufferComplete(worldBuffer *pWorldBuf);

};

#endif /* MULTIDOMAINBUFFER_H_ */

