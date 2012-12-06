/*
 * MultiNBodyDomain.cpp
 *
 *  Created on: Oct 15, 2012
 *      Author: martin
 */

#include "MultiNBodyDomain.h"
#include "MultiNBodyWorld.h"

#ifdef USE_SDK
#include <helper_cuda.h>
#endif

MultiNBodyDomain::MultiNBodyDomain():Domain(),np(0)
{
	// TODO Auto-generated constructor stub
}

MultiNBodyDomain::~MultiNBodyDomain()
{
	// TODO Auto-generated destructor stub
	if(isActive()){
		if(h_pos){delete h_pos;h_pos=NULL;}
		if(h_mom){delete h_mom;h_mom=NULL;}
		if(h_force){delete h_force;h_force=NULL;}
		if(onGpu()){
			if(d_pos[0]){cudaFree(d_pos[0]);d_pos[0]=NULL;}
			if(d_pos[1]){cudaFree(d_pos[1]);d_pos[1]=NULL;}
			if(d_mom){cudaFree(d_mom);d_mom=NULL;}
			if(d_force){cudaFree(d_force);d_force=NULL;}
		}
	}
}

void MultiNBodyDomain::activate(){
	if(rank==theWorld->getRank())activated=true;
	if(isActive()){  // Only Allocate Memory on Activated Domains on Process
		//printf("Allocating CPU Data\n");
		h_pos=new float[4*np]; // x,y,z,mass
		h_mom=new float[4*np]; // px,py,pz,energy
		h_force=new float[4*np]; // fx,fy,fz,n/a - 4 not 4 for memory alignment

		if(onGpu()){
			//printf("Allocating GPU Data\n");

			int count;
			cudaGetDeviceCount(&count);
			dev=rank%count;
			cudaDeviceProp props;
			cudaGetDeviceProperties(&props, dev);
			// Debug - Display Compute level by card
			// Compute 1.3 incompatible with kernel printf
			//printf("Device Properties[%d]: Compute:%d.%d\n",dev,props.major,props.minor);

			cudaSetDevice(dev);

			cudaMalloc(&d_pos[0],4*np*sizeof(float));
			cudaMalloc(&d_pos[1],4*np*sizeof(float));
			cudaMalloc(&d_mom,4*np*sizeof(float));
			cudaMalloc(&d_force,4*np*sizeof(float));
		}
	}
}

void MultiNBodyDomain::init(){
	int setcfg=theWorld->getRank()%NBODY_NUM_CONFIGS;
	if(isActive())randomizeBodies(setcfg);
}

void MultiNBodyDomain::step(float indt,int di){
	if(isActive()){
		if(onGpu()) gpuIntegrateNBodySystem(indt,di);
		else integrateNBodySystem(indt, di);

		// To use force calculation only on GPU simply call:
		//integrateNBodySystem(indt);
		// Note: Thread block sizes not updated for arbitrary np in force calculator kernel call -- use power of 2

// ----DEBUG--- Print a particle position per domain for debugging
		//printf("{%f %f %f} ",h_pos[4*(np-1)],h_pos[4*(np-1)+1],h_pos[4*(np-1)+2]);
	}
}


// Much of the following is a stripped down version of the CUDA SDK NBody Demo

// Macros to simplify shared memory addressing
#define SX(i) sharedPos[i+blockDim.x*threadIdx.y]
// This macro is only used when multithreadBodies is true (below)
#define SX_SUM(i,j) sharedPos[i+blockDim.x*j]

struct SharedMemory
{
    __device__ inline operator       float4 *()
    {
        extern __shared__ int __smem[];
        return (float4 *)__smem;
    }

    __device__ inline operator const float4 *() const
    {
        extern __shared__ int __smem[];
        return (float4 *)__smem;
    }
};

__device__ float4 bodyBodyInteraction(float4 ai,float4 bi,float4 bj,float softeningSquared)
{
    float4 r;

    // r_ij  [3 FLOPS]
    r.x = bj.x - bi.x;
    r.y = bj.y - bi.y;
    r.z = bj.z - bi.z;

    // distSqr = dot(r_ij, r_ij) + EPS^2  [6 FLOPS]
    float distSqr = r.x * r.x + r.y * r.y + r.z * r.z;
    distSqr += softeningSquared;

    // invDistCube =1/distSqr^(3/2)  [4 FLOPS (2 mul, 1 sqrt, 1 inv)]
    float invDist = rsqrt(distSqr);
    float invDistCube =  invDist * invDist * invDist;

    // s = m_j * invDistCube [1 FLOP]
    float s = bj.w * invDistCube;

    // a_i =  a_i + s * r_ij [6 FLOPS]
    ai.x += r.x * s;
    ai.y += r.y * s;
    ai.z += r.z * s;

    return ai;
}

// This is the "tile_calculation" function from the GPUG3 article.
__device__ float4 gravitation(float4 iPos,float4 accel,float ssq)
{
    float4 *sharedPos = SharedMemory();

    // The CUDA 1.1 compiler cannot determine that i is not going to
    // overflow in the loop below.  Therefore if int is used on 64-bit linux
    // or windows (or long instead of long long on win64), the compiler
    // generates suboptimal code.  Therefore we use long long on win64 and
    // long on everything else. (Workaround for Bug ID 347697)
#ifdef _Win64
    unsigned long long j = 0;
#else
    unsigned long j = 0;
#endif

    // Here we unroll the loop to reduce bookkeeping instruction overhead
    // 32x unrolling seems to provide best performance

    // Note that having an unsigned int loop counter and an unsigned
    // long index helps the compiler generate efficient code on 64-bit
    // OSes.  The compiler can't assume the 64-bit index won't overflow
    // so it incurs extra integer operations.  This is a standard issue
    // in porting 32-bit code to 64-bit OSes.

#pragma unroll 32

    for (unsigned int counter = 0; counter < blockDim.x; counter++)
    {
        accel = bodyBodyInteraction(accel, iPos, SX(j++),ssq);
    }

    return accel;
}

// WRAP is used to force each block to start working on a different
// chunk (and wrap around back to the beginning of the array) so that
// not all multiprocessors try to read the same memory locations at
// once.
#define WRAP(x,m) (((x)<m)?(x):(x-m))  // Mod without divide, works on values from 0 up to 2m

__device__ float4 computeBodyAccel(float4 bodyPos,
                 float4 *positions,
                 int numBodies,float ssq)
{
    float4 *sharedPos = SharedMemory();

    float4 acc = {0.0f, 0.0f, 0.0f, 0.0f};

    int p = blockDim.x;
    int q = blockDim.y;
    int n = numBodies;
    int numTiles = n / (p * q);

    for (int tile = blockIdx.y; tile < numTiles + blockIdx.y; tile++)
    {
        sharedPos[threadIdx.x+blockDim.x*threadIdx.y] =
            positions[WRAP(blockIdx.x + q * tile + threadIdx.y, gridDim.x) * p + threadIdx.x] ;

        __syncthreads();

        // This is the "tile_calculation" function from the GPUG3 article.
        acc = gravitation(bodyPos, acc, ssq);

        __syncthreads();
    }

    // When the numBodies / thread block size is < # multiprocessors (16 on G80), the GPU is
    // underutilized.  For example, with a 256 threads per block and 1024 bodies, there will only
    // be 4 thread blocks, so the GPU will only be 25% utilized. To improve this, we use multiple
    // threads per body.  We still can use blocks of 256 threads, but they are arranged in q rows
    // of p threads each.  Each thread processes 1/q of the forces that affect each body, and then
    // 1/q of the threads (those with threadIdx.y==0) add up the partial sums from the other
    // threads for that body.  To enable this, use the "--p=" and "--q=" command line options to
    // this example. e.g.: "nbody.exe --n=1024 --p=64 --q=4" will use 4 threads per body and 256
    // threads per block. There will be n/p = 16 blocks, so a G80 GPU will be 100% utilized.

    // We use a bool template parameter to specify when the number of threads per body is greater
    // than one, so that when it is not we don't have to execute the more complex code required!

	SX_SUM(threadIdx.x, threadIdx.y).x = acc.x;
	SX_SUM(threadIdx.x, threadIdx.y).y = acc.y;
	SX_SUM(threadIdx.x, threadIdx.y).z = acc.z;

	__syncthreads();

	// Save the result in global memory for the integration step
	if (threadIdx.y == 0)
	{
		for (int i = 1; i < blockDim.y; i++)
		{
			acc.x += SX_SUM(threadIdx.x,i).x;
			acc.y += SX_SUM(threadIdx.x,i).y;
			acc.z += SX_SUM(threadIdx.x,i).z;
		}
	}

    return acc;
}

__global__ void calculateForce(float4 *pos,
                float4 *force,
                unsigned int deviceNumBodies, int totalNumBodies, float ssq)
{
    int index = blockIdx.x * blockDim.x + threadIdx.x;
    //printf("Index (%d)\n",index);  // Debug that Kernel did not crash if not using SDK
    if (index >= totalNumBodies)
    {
        return;
    }

    float4 position = pos[index];

    force[index]= computeBodyAccel(position, pos, totalNumBodies, ssq);
}

void MultiNBodyDomain::gpuComputeNBodyGravitation(){
	cudaMemcpy(d_pos[0],h_pos,4*np*sizeof(float),cudaMemcpyHostToDevice);

// Port Force Calculation Only
	cudaDeviceProp props;

	//unsigned int dev = 0;
	cudaGetDeviceProperties(&props, dev);

	int q=props.multiProcessorCount; // Use all multiprocsessors
	int p=min(props.maxThreadsDim[0],(int)ceil((float)np/(float)q));

	int pq=min(props.maxThreadsPerBlock,p*q);
	p=pq/q;

	//printf("Blocking: p=%d,q=%d,p*q=%d - np=%d\n",p,q,p*q,np);

	dim3 threads(p,q,1);
	dim3 grid((np + (p-1))/p, 1, 1);


	// execute the kernel:

	// When the numBodies / thread block size is < # multiprocessors
	// (16 on G80), the GPU is underutilized. For example, with 256 threads per
	// block and 1024 bodies, there will only be 4 thread blocks, so the
	// GPU will only be 25% utilized.  To improve this, we use multiple threads
	// per body.  We still can use blocks of 256 threads, but they are arranged
	// in q rows of p threads each.  Each thread processes 1/q of the forces
	// that affect each body, and then 1/q of the threads (those with
	// threadIdx.y==0) add up the partial sums from the other threads for that
	// body.  To enable this, use the "--p=" and "--q=" command line options to
	// this example.  e.g.: "nbody.exe --n=1024 --p=64 --q=4" will use 4
	// threads per body and 256 threads per block. There will be n/p = 16
	// blocks, so a G80 GPU will be 100% utilized.

	// We use a bool template parameter to specify when the number of threads
	// per body is greater than one, so that when it is not we don't have to
	// execute the more complex code required!
	int sharedMemSize = p * q * 4 * sizeof(float); // 4 floats for pos

	if (grid.x > 0)
	{
		calculateForce<<< grid, threads, sharedMemSize >>>((float4 *)d_pos[0], (float4 *)d_force,p,np,param.m_softeningSquared);
	}

#ifdef USE_SDK
	getLastCudaError("Kernel execution failed");
#endif

	cudaMemcpy(h_force,d_force,(4*np)*sizeof(float),cudaMemcpyDeviceToHost);
}

void MultiNBodyDomain::computeNBodyGravitation()
{
#ifdef OPENMP
    #pragma omp parallel for
#endif

    for (int i = 0; i < np; i++)
    {
        int indexForce = 4*i;

        float acc[3] = {0, 0, 0};

        // We unroll this loop 4X for a small performance boost.
        int j = 0;

        while (j < np)
        {
            bodyBodyInteraction(acc, &h_pos[4*i], &h_pos[4*j], param.m_softeningSquared);
            j++;
            bodyBodyInteraction(acc, &h_pos[4*i], &h_pos[4*j], param.m_softeningSquared);
            j++;
            bodyBodyInteraction(acc, &h_pos[4*i], &h_pos[4*j], param.m_softeningSquared);
            j++;
            bodyBodyInteraction(acc, &h_pos[4*i], &h_pos[4*j], param.m_softeningSquared);
            j++;
        }

        h_force[indexForce  ] = acc[0];
        h_force[indexForce+1] = acc[1];
        h_force[indexForce+2] = acc[2];
    }
}

void MultiNBodyDomain::bodyBodyInteraction(float accel[3], float posMass0[4], float posMass1[4], float softeningSquared)
{
    float r[3];

    // r_01  [3 FLOPS]
    r[0] = posMass1[0] - posMass0[0];
    r[1] = posMass1[1] - posMass0[1];
    r[2] = posMass1[2] - posMass0[2];

    // d^2 + e^2 [6 FLOPS]
    float distSqr = r[0] * r[0] + r[1] * r[1] + r[2] * r[2];
    distSqr += softeningSquared;

    // invDistCube =1/distSqr^(3/2)  [4 FLOPS (2 mul, 1 sqrt, 1 inv)]
    float invDist = (float)1.0 / (float)sqrt((double)distSqr);
    float invDistCube =  invDist * invDist * invDist;

    // s = m_j * invDistCube [1 FLOP]
    float s = posMass1[3] * invDistCube;

    // (m_1 * r_01) / (d^2 + e^2)^(3/2)  [6 FLOPS]
    accel[0] += r[0] * s;
    accel[1] += r[1] * s;
    accel[2] += r[2] * s;
}

void MultiNBodyDomain::integrateNBodySystem(float deltaTime,int di)
{
    if(onGpu()) gpuComputeNBodyGravitation();
    else computeNBodyGravitation();

    // Debug Force should match GPU vs CPU
    //printf("Force: %f %f %f [%d]\n",h_force[0+4*(np-1)],h_force[1+4*(np-1)],h_force[2+4*(np-1)],np-1);

#ifdef OPENMP
    #pragma omp parallel for
#endif

    for (int i = 0; i < np; ++i)
    {
        int index = 4*i;
        int indexForce = 3*i;


        float pos[3], vel[3], force[3];
        pos[0] = h_pos[index+0];
        pos[1] = h_pos[index+1];
        pos[2] = h_pos[index+2];
        float invMass = 1.0f/h_pos[index+3];

        vel[0] = h_mom[index+0];
        vel[1] = h_mom[index+1];
        vel[2] = h_mom[index+2];

        force[0] = h_force[index+0];//h_force[indexForce+0];
        force[1] = h_force[index+1];//h_force[indexForce+1];
        force[2] = h_force[index+2];//h_force[indexForce+2];

        // acceleration = force / mass;
        // new velocity = old velocity + acceleration * deltaTime
        vel[0] += (force[0] * invMass) * deltaTime;
        vel[1] += (force[1] * invMass) * deltaTime;
        vel[2] += (force[2] * invMass) * deltaTime;

        vel[0] *= param.m_damping;
        vel[1] *= param.m_damping;
        vel[2] *= param.m_damping;

        // new position = old position + velocity * deltaTime
        pos[0] += vel[0] * deltaTime;
        pos[1] += vel[1] * deltaTime;
        pos[2] += vel[2] * deltaTime;

        h_pos[index+0] = pos[0];
        h_pos[index+1] = pos[1];
        h_pos[index+2] = pos[2];

        h_mom[index+0] = vel[0]*h_pos[index+3];
        h_mom[index+1] = vel[1]*h_pos[index+3];
        h_mom[index+2] = vel[2]*h_pos[index+3];
    }
    //printf("Point 0: %f,%f,%f\n",h_pos[0],h_pos[1],h_pos[2]);
	((MultiNBodyWorld*)theWorld)->getMDB()->Buffer(h_pos,theWorld->getIter(),di);
}

__global__ void integrateBodies(float4* newPos,
                float4* oldPos,
                float4* vel,
                unsigned int deviceNumBodies,
                float deltaTime, float damping, float softeningSquared, int totalNumBodies)
{
    int index = blockIdx.x * blockDim.x + threadIdx.x;

    //printf("Index: %d\n",index); // Debug test for Kernel crash if not using SDK

    if (index >= totalNumBodies)
    {
        return;
    }

    float4 position = oldPos[index];

    float4 accel = computeBodyAccel(position, oldPos, totalNumBodies, softeningSquared);

    //Debug to ensure accel not NaN or 0
    //if(index==1)printf("Accel: %f %f %f at %f %f %f\n",accel.x,accel.y,accel.z,position.x,position.y,position.z);



    if (threadIdx.y == 0)
    {
        // acceleration = force \ mass;
        // new velocity = old velocity + acceleration * deltaTime
        // note we factor out the body's mass from the equation, here and in bodyBodyInteraction
        // (because they cancel out).  Thus here force == acceleration
        float4 velocity = vel[index];

        velocity.x += accel.x * deltaTime;
        velocity.y += accel.y * deltaTime;
        velocity.z += accel.z * deltaTime;

        velocity.x *= damping;
        velocity.y *= damping;
        velocity.z *= damping;

        // new position = old position + velocity * deltaTime
        position.x += velocity.x * deltaTime;
        position.y += velocity.y * deltaTime;
        position.z += velocity.z * deltaTime;

        // store new position and velocity
        newPos[index] = position;
        vel[index]    = velocity;
    }
}

void MultiNBodyDomain::gpuIntegrateNBodySystem(float deltaTime,int di)
{
    cudaDeviceProp props;

    //unsigned int dev = 0;
    cudaSetDevice(dev);
	cudaGetDeviceProperties(&props, dev);

	// Tile Size = p*q
	// Tiles must be full
	// np%(pq)=0

	int q=1;
	int p=32;
	while(np%((2*q))==0)q*=2; // Np divisibility first


	q=min(q,props.multiProcessorCount);
	//int p=np/q;//props.maxThreadsPerBlock/(props.multiProcessorCount));

	q=min(props.multiProcessorCount,np/p);


	int sharedMemSize = p * q * 4 * sizeof(float); // 4 floats for pos
	while(sharedMemSize > props.sharedMemPerBlock/2){
            q/=2;
	    sharedMemSize = p * q * 4 * sizeof(float);
	}

	int g=(np + (p-1))/(p);

	dim3 threads(p,q,1);
	dim3 grid(g, 1, 1);

//// ----DEBUG---- Print Thread Block and Memory to Identify Some Kernel Crash Problems
////	printf("p,q=%d,%d x %d mem=%d / %d\n",p,q,g,sharedMemSize,props.sharedMemPerBlock);

	// execute the kernel:

	// When the numBodies / thread block size is < # multiprocessors
	// (16 on G80), the GPU is underutilized. For example, with 256 threads per
	// block and 1024 bodies, there will only be 4 thread blocks, so the
	// GPU will only be 25% utilized.  To improve this, we use multiple threads
	// per body.  We still can use blocks of 256 threads, but they are arranged
	// in q rows of p threads each.  Each thread processes 1/q of the forces
	// that affect each body, and then 1/q of the threads (those with
	// threadIdx.y==0) add up the partial sums from the other threads for that
	// body.  To enable this, use the "--p=" and "--q=" command line options to
	// this example.  e.g.: "nbody.exe --n=1024 --p=64 --q=4" will use 4
	// threads per body and 256 threads per block. There will be n/p = 16
	// blocks, so a G80 GPU will be 100% utilized.

	// We use a bool template parameter to specify when the number of threads
	// per body is greater than one, so that when it is not we don't have to
	// execute the more complex code required!

	int currentRead=theWorld->getIter()%2;

	//cudaMemcpyAsync(h_pos,d_pos[currentRead],4*np*sizeof(float),cudaMemcpyDeviceToHost,0);
	cudaMemcpy(h_pos,d_pos[currentRead],4*np*sizeof(float),cudaMemcpyDeviceToHost);

	integrateBodies<<< grid, threads, sharedMemSize >>>
			((float4*)d_pos[1-currentRead],(float4*)d_pos[currentRead],(float4*)d_mom, np,
					deltaTime, param.m_damping, param.m_softeningSquared, np);

#ifdef USE_SDK
	// check if kernel invocation generated an error
	getLastCudaError("Kernel execution failed");
#endif

	cudaThreadSynchronize();
	//printf("iter %d domain %d point 0 %f %f %f\n",theWorld->getIter(),di,h_pos[0],h_pos[1],h_pos[2]);	
	((MultiNBodyWorld*)theWorld)->getMDB()->Buffer(h_pos,theWorld->getIter(),di);
}

void MultiNBodyDomain::randomizeBodies(int config){
	if(onGpu())	cudaMemset(d_force, 0, np*4*sizeof(float)); // Clear Force Vector

	switch (config)
	{
		default:
		case NBODY_CONFIG_RANDOM:
			{
				float scale = param.m_clusterScale * std::max<float>(1.0f, np / (1024.0f));
				float vscale = param.m_velocityScale * scale;

				int p = 0, v = 0;
				int i = 0;

				while (i < np)
				{
					float3 point;
					//const int scale = 16;
					point.x = rand() / (float) RAND_MAX * 2 - 1;
					point.y = rand() / (float) RAND_MAX * 2 - 1;
					point.z = rand() / (float) RAND_MAX * 2 - 1;
					float lenSqr = point.x*point.x+point.y*point.y+point.z*point.z;//dot(point, point);

					if (lenSqr > 1)
						continue;

					float3 velocity;
					velocity.x = rand() / (float) RAND_MAX * 2 - 1;
					velocity.y = rand() / (float) RAND_MAX * 2 - 1;
					velocity.z = rand() / (float) RAND_MAX * 2 - 1;
					lenSqr = velocity.x*velocity.x+velocity.y*velocity.y+velocity.z*velocity.z;//dot(velocity, velocity);

					if (lenSqr > 1)
						continue;

					h_pos[p++] = point.x * scale; // pos.x
					h_pos[p++] = point.y * scale; // pos.y
					h_pos[p++] = point.z * scale; // pos.z
					h_pos[p++] = 1.0f; // mass

					h_mom[v++] = h_pos[p]*velocity.x * vscale; // pos.x
					h_mom[v++] = h_pos[p]*velocity.y * vscale; // pos.x
					h_mom[v++] = h_pos[p]*velocity.z * vscale; // pos.x
					h_mom[v++] = (h_mom[v-1]*h_mom[v-1]+h_mom[v-2]*h_mom[v-2]+h_mom[v-3]*h_mom[v-3])/h_pos[p]; // energy

					i++;
				}
			}
			break;

		case NBODY_CONFIG_SHELL:
			{
				float scale = param.m_clusterScale;
				float vscale = scale * param.m_velocityScale;
				float inner = 2.5f * scale;
				float outer = 4.0f * scale;

				int p = 0, v=0;
				int i = 0;

				while (i < np)//for(int i=0; i < numBodies; i++)
				{
					float x, y, z;
					x = rand() / (float) RAND_MAX * 2 - 1;
					y = rand() / (float) RAND_MAX * 2 - 1;
					z = rand() / (float) RAND_MAX * 2 - 1;

					float3 point = {x, y, z};
					float len = sqrt(point.x*point.x+point.y*point.y+point.z*point.z);//normalize(point);
					point.x/=len;point.y/=len;point.z/=len;

					if (len > 1)
						continue;

					h_pos[p++] =  point.x * (inner + (outer - inner) * rand() / (float) RAND_MAX);
					h_pos[p++] =  point.y * (inner + (outer - inner) * rand() / (float) RAND_MAX);
					h_pos[p++] =  point.z * (inner + (outer - inner) * rand() / (float) RAND_MAX);
					h_pos[p++] = 1.0f;

					x = 0.0f; // * (rand() / (float) RAND_MAX * 2 - 1);
					y = 0.0f; // * (rand() / (float) RAND_MAX * 2 - 1);
					z = 1.0f; // * (rand() / (float) RAND_MAX * 2 - 1);

					float3 axis = {x, y, z};
					len=sqrt(axis.x*axis.x+axis.y*axis.y+axis.z*axis.z);//normalize(axis);
					axis.x/=len;axis.y/=len;axis.z/=len;

					if (1 - point.x*axis.x+point.y*axis.y+point.z*axis.z < 1e-6)  //dot(point, axis)
					{
						axis.x = point.y;
						axis.y = point.x;
						len=sqrt(axis.x*axis.x+axis.y*axis.y+axis.z*axis.z);//normalize(axis);
						axis.x/=len;axis.y/=len;axis.z/=len;
					}

					//if (point.y < 0) axis = scalevec(axis, -1);
					float3 vv = {(float)h_pos[4*i], (float)h_pos[4*i+1], (float)h_pos[4*i+2]};
					vv = make_float3(vv.y*axis.z-vv.z*axis.y,vv.z*axis.x-vv.x*axis.z,vv.x*axis.y-vv.y*axis.x);//cross(vv, axis);
					h_mom[v++] = h_pos[p] * vv.x * vscale;
					h_mom[v++] = h_pos[p] * vv.y * vscale;
					h_mom[v++] = h_pos[p] * vv.z * vscale;
					h_mom[v++] = (h_mom[v-1]*h_mom[v-1]+h_mom[v-2]*h_mom[v-2]+h_mom[v-3]*h_mom[v-3])/h_pos[p]; // energy

					i++;
				}
			}
			break;

		case NBODY_CONFIG_EXPAND:
			{
				float scale = param.m_clusterScale * np / (1024.f);

				if (scale < 1.0f)
					scale = param.m_clusterScale;

				float vscale = scale * param.m_velocityScale;

				int p = 0, v = 0;

				for (int i=0; i < np;)
				{
					float3 point;

					point.x = rand() / (float) RAND_MAX * 2 - 1;
					point.y = rand() / (float) RAND_MAX * 2 - 1;
					point.z = rand() / (float) RAND_MAX * 2 - 1;

					float lenSqr = point.x*point.x+point.y*point.y+point.z*point.z;//dot(point, point);

					if (lenSqr > 1)
						continue;

					h_pos[p++] = point.x * scale; // pos.x
					h_pos[p++] = point.y * scale; // pos.y
					h_pos[p++] = point.z * scale; // pos.z
					h_pos[p++] = 1.0f; // mass
					h_mom[v++] = h_pos[p] * point.x * vscale; // pos.x
					h_mom[v++] = h_pos[p] * point.y * vscale; // pos.x
					h_mom[v++] = h_pos[p] * point.z * vscale; // pos.x
					h_mom[v++] = (h_mom[v-1]*h_mom[v-1]+h_mom[v-2]*h_mom[v-2]+h_mom[v-3]*h_mom[v-3])/h_pos[p]; // energy

					i++;
				}
			}
			break;
	}

	if(onGpu()){
		cudaSetDevice(dev);
		cudaMemcpy(d_pos[0],h_pos,4*np*sizeof(float),cudaMemcpyHostToDevice);
		cudaMemcpy(d_mom,h_mom,4*np*sizeof(float),cudaMemcpyHostToDevice);
	}
}
