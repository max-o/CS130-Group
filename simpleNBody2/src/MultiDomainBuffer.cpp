
#include "MultiDomainBuffer.h"
#include "stdio.h"
MultiDomainBuffer::MultiDomainBuffer(int nBods, int nDoms)
{
// printf("** MultiDomainBuffer Constructor\n");
   this->nBods = nBods;
   this->nDoms = nDoms;

   this->pWorldBuf = new worldBuffer[NUM_DOMAIN_BUFFERS ];
   
   // allocate memory for the world buffers
   // each world buffer contains information for all the domains in the world
   // each domain contains information for all the bodies in the domain
   for(int bufferCounter = 0; bufferCounter < NUM_DOMAIN_BUFFERS; bufferCounter++)
   {
      this->pWorldBuf[bufferCounter].pDoms = new domainBuffer[nDoms];

      for(int domainCounter = 0; domainCounter < nDoms; domainCounter ++)
      {
         this->pWorldBuf[bufferCounter].pDoms[domainCounter].nIter = -1;
         this->pWorldBuf[bufferCounter].pDoms[domainCounter].pBuf = new float[FLOATS_PER_BODY*nBods];
      }
   }

   completeBuffer = 2;
   iterLockedBuffer = 0;
   iterUnlockedBuffer = 1;
   callback = NULL;
   bufferLocked = false;

   this->pWorldBuf[iterLockedBuffer].nIter = 0;
   this->pWorldBuf[iterUnlockedBuffer].nIter = 1;
   this->pWorldBuf[completeBuffer].nIter = -1;
}

MultiDomainBuffer::~MultiDomainBuffer()
{
   // delete all dynamically allocated data
   for(int bufferCounter = 0; bufferCounter < NUM_DOMAIN_BUFFERS; bufferCounter++)
   {
      for(int domainCounter = 0; domainCounter < nDoms; domainCounter ++)
      {
         delete this->pWorldBuf[bufferCounter].pDoms[domainCounter].pBuf;
      }

      delete this->pWorldBuf[bufferCounter].pDoms;
   }

   delete this->pWorldBuf;
}

void MultiDomainBuffer::Buffer(float *fBods, int nIter, int nDom)
{
//printf("buffer nIter %d, nDom %d\n", nIter, nDom);
   // does this invocation contain info from the simulation iteration that corresponds to the fixed
   // iteration buffer?
   if(nIter == this->pWorldBuf[iterLockedBuffer].nIter)
   {
      // establish a lock to make this thread-safe -- the buffer swap would not be thread-safe
      boost::mutex::scoped_lock l(guard);

      // copy the data to the buffer
      this->pWorldBuf[iterLockedBuffer].pDoms[nDom].nIter = nIter;
      memcpy(this->pWorldBuf[iterLockedBuffer].pDoms[nDom].pBuf, fBods, nBods*FLOATS_PER_BODY*sizeof(float));

      // this buffer is now complete?
      if(isBufferComplete(&this->pWorldBuf[iterLockedBuffer]))
      {
         // no one's watching or the buffer is still being written - drop the iteration frame
         if(NULL == this->callback || this->bufferLocked)
         {
            // swap the indices of the locked and unlocked buffers
            int tempIdx = iterLockedBuffer;
            iterLockedBuffer = iterUnlockedBuffer;
            iterUnlockedBuffer = tempIdx;

            // make sure that the unlockedBuffer starts accumulating from a later iteration
            this->pWorldBuf[iterUnlockedBuffer].nIter = this->pWorldBuf[iterLockedBuffer].nIter + 1;
         }

         // otherwise, notify the callback that a new buffer is available
         else
         {
            this->bufferLocked = true;
            callback(this, &this->pWorldBuf[iterLockedBuffer]);

            // cycle the buffers
            // lockedBuffer <- iterLockedBuffer (lock the currently complete buffer
            // iterLockedBuffer <- iterUnlockedBuffer (lock the unfixed buffer so that we can finish populating the next frame)
            // iterUnlockedBuffer <- lockedBuffer (use the previously complete buffer to start accumulating future iterations)
            int tempIdx = this->completeBuffer;
            
            this->completeBuffer = this->iterLockedBuffer;
            this->iterLockedBuffer = this->iterUnlockedBuffer;
            this->iterUnlockedBuffer = tempIdx;

            // make sure that the unlockedBuffer starts accumulating from a later iteration
            this->pWorldBuf[iterUnlockedBuffer].nIter = this->pWorldBuf[iterLockedBuffer].nIter + 1;
         }
      }
   } 
   
   // if this invocation contains information from a later iteration, we need to track it in the unfixed
   // iteration buffer
   else if(nIter >= this->pWorldBuf[iterUnlockedBuffer].nIter)
   {
      // establish a lock to make this thread-safe -- the changes to buffer iteration would not be thread safe
      boost::mutex::scoped_lock l(guard);

      // copy the data into our buffer
      this->pWorldBuf[iterUnlockedBuffer].pDoms[nDom].nIter = nIter;
      memcpy(this->pWorldBuf[iterUnlockedBuffer].pDoms[nDom].pBuf, fBods, nBods*FLOATS_PER_BODY*sizeof(float));

      // if this is for a later iteration than the iteration that's currently being populated into the
      // buffer, we need to update the whole buffer to indicate that the greatest iteration is being
      // aggregated
      if(nIter > this->pWorldBuf[iterUnlockedBuffer].nIter)
      {
         this->pWorldBuf[iterUnlockedBuffer].nIter = nIter;
      }
   }
}

void MultiDomainBuffer::OnBufferComplete(pBufferCompleteFn callback)
{
   this->callback = callback;
}

void MultiDomainBuffer::UnlockBuffer()
{
   this->bufferLocked = false;
}

bool MultiDomainBuffer::isBufferComplete(worldBuffer *pWorldBuf)
{
   // a complete buffer is one where every domain has data corresponding to
   // the same iteration as the buffer's iteration
   

	for(int domCounter = 0; domCounter < this->nDoms; domCounter++)
   {
	
	//printf("** comp world_it %d, buf_it %d\n", pWorldBuf->nIter, pWorldBuf->pDoms[domCounter].nIter);
      
    if(pWorldBuf->nIter != pWorldBuf->pDoms[domCounter].nIter)
      {
      	//printf("***** Not Completed\n");   
      	return false;
      }
   
  }
	for(int domCounter = 0; domCounter < this->nDoms; domCounter++)
	//printf("Point 0 in iter %d domain %d is %f %f %f\n",pWorldBuf->nIter,domCounter,pWorldBuf->pDoms[domCounter].pBuf[0],pWorldBuf->nIter,domCounter,pWorldBuf->pDoms[domCounter].pBuf[1],pWorldBuf->nIter,domCounter,pWorldBuf->pDoms[domCounter].pBuf[2]);
 //printf("***** Completed\n");	
//printf("Point 0 in iter %d domain %d",pWorldBuf->nIter,domCounter);
   return true;
}

int MultiDomainBuffer::getNBodies()
{
   return this->nBods;
}

int MultiDomainBuffer::getNDomains()
{
   return this->nDoms;
}

/*
// callback function used for unit testing
void unitTestCallback (MultiDomainBuffer* mdb, worldBuffer* pBuf)
{
   fprintf(stdout, "callback invoked for iter %d\n", pBuf->nIter);
   mdb->UnlockBuffer();
}
*/

/*
// simple unit tests
int main(int argc, char* argv[])
{
   float pts[2*FLOATS_PER_BODY] = {1.0, 2.0, 3.0, 1.0,
                     2.0, 4.0, 8.0, 1.0};

   MultiDomainBuffer* mdb = new MultiDomainBuffer(2, 2);

   mdb->OnBufferComplete(unitTestCallback);

   mdb->Buffer(pts, 0, 0);
   mdb->Buffer(pts, 0, 1);
   // callback should fire for iter 0

   mdb->Buffer(pts, 1, 0);
   mdb->Buffer(pts, 1, 1);
   // callback should fire for iter 1

   mdb->Buffer(pts, 2, 0);
   mdb->Buffer(pts, 3, 0);
   mdb->Buffer(pts, 4, 0);
   mdb->Buffer(pts, 5, 0);
   mdb->Buffer(pts, 2, 1);
   // callback should fire for iter 2

   mdb->Buffer(pts, 5, 1);
   // callback should fire for iter 5

   return 0;
}


*/
