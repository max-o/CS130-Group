################################################################################
#
# Copyright 1993-2012 NVIDIA Corporation.  All rights reserved.
#
# NOTICE TO USER:   
#
# This source code is subject to NVIDIA ownership rights under U.S. and 
# international Copyright laws.  
#
# NVIDIA MAKES NO REPRESENTATION ABOUT THE SUITABILITY OF THIS SOURCE 
# CODE FOR ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT EXPRESS OR 
# IMPLIED WARRANTY OF ANY KIND.  NVIDIA DISCLAIMS ALL WARRANTIES WITH 
# REGARD TO THIS SOURCE CODE, INCLUDING ALL IMPLIED WARRANTIES OF 
# MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE.   
# IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL, 
# OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS 
# OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE 
# OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE 
# OR PERFORMANCE OF THIS SOURCE CODE.  
#
# U.S. Government End Users.  This source code is a "commercial item" as 
# that term is defined at 48 C.F.R. 2.101 (OCT 1995), consisting  of 
# "commercial computer software" and "commercial computer software 
# documentation" as such terms are used in 48 C.F.R. 12.212 (SEPT 1995) 
# and is provided to the U.S. Government only as a commercial end item.  
# Consistent with 48 C.F.R.12.212 and 48 C.F.R. 227.7202-1 through 
# 227.7202-4 (JUNE 1995), all U.S. Government End Users acquire the 
# source code with only those rights set forth herein.
#
################################################################################
#
# Makefile project only supported on Mac OSX and Linux Platforms)
#
################################################################################

#dbg = 1

USE_SDK = TRUE

# OS Name (Linux or Darwin)
OSUPPER = $(shell uname -s 2>/dev/null | tr [:lower:] [:upper:])
OSLOWER = $(shell uname -s 2>/dev/null | tr [:upper:] [:lower:])

# Flags to detect 32-bit or 64-bit OS platform
OS_SIZE = $(shell uname -m | sed -e "s/i.86/32/" -e "s/x86_64/64/")
OS_ARCH = $(shell uname -m | sed -e "s/i386/i686/")

# Flags to detect either a Linux system (linux) or Mac OSX (darwin)
DARWIN = $(strip $(findstring DARWIN, $(OSUPPER)))

# Location of the CUDA Toolkit binaries and libraries
CUDA_PATH       ?= /usr/local/cuda
CUDA_INC_PATH   ?= $(CUDA_PATH)/include
CUDA_LIB_PATH   ?= $(CUDA_PATH)/lib
CUDA_BIN_PATH   ?= $(CUDA_PATH)/bin

# Only needed when SDK functionality used
# --Local Install Only --  CUDA_SDK_PATH   ?= $(HOME)/NVIDIA_CUDA_Samples/C
CUDA_SDK_PATH ?= /usr/local/cuda/samples

MPI_PATH        ?= /usr/lib/openmpi
MPI_LIB_PATH    ?= $(MPI_PATH)/lib
MPI_INC_PATH    ?= $(MPI_PATH)/include
LD_MPI_FLAGS    ?= -lmpi -lmpi_cxx
#MPI_INC_PATH = /usr/include/mpich2
#MPI_LIB_PATH = /usr/lib
#LD_MPI_FLAGS = -lmpich -lmpichcxx

#Boost stuff
BOOST_PATH        ?= ../boostLibs
BOOST_LIB_PATH    ?= $(BOOST_PATH)/stage/lib
BOOST_INC_PATH    ?= $(BOOST_PATH)/
LD_BOOST_FLAGS    ?= -lboost_thread -lboost_system

# Common binaries
NVCC            ?= $(CUDA_BIN_PATH)/nvcc $(USE_DEBUG)
GCC             ?= g++ $(USE_DEBUG)

# Extra user flags
EXTRA_NVCCFLAGS ?=
EXTRA_LDFLAGS   ?=
EXTRA_CCFLAGS   ?= #-std=c++0x

# CUDA code generation flags
GENCODE_SM10    := -gencode arch=compute_10,code=sm_10
GENCODE_SM13    := -gencode arch=compute_13,code=sm_13
GENCODE_SM20    := -gencode arch=compute_20,code=sm_20
GENCODE_SM30    := -gencode arch=compute_30,code=sm_30
GENCODE_FLAGS   :=  $(GENCODE_SM10) $(GENCODE_SM20) #$(GENCODE_SM30)



# OS-specific build flags
ifneq ($(DARWIN),) 
      LDFLAGS   := -Xlinker -rpath $(CUDA_LIB_PATH) -rpath $(MPI_LIB_PATH) -L$(CUDA_LIB_PATH) -L$(MPI_LIB_PATH) -L$(BOOST_LIB_PATH) -lcudart -lcufft $(LD_MPIFLAGS) $(LD_BOOST_FLAGS)
      CCFLAGS   := -arch $(OS_ARCH) 
else
  ifeq ($(OS_SIZE),32)
      LDFLAGS   := -L$(CUDA_LIB_PATH) -lcudart -lcufft -L$(MPI_LIB_PATH) $(LD_MPIFLAGS) -L$(BOOST_LIB_PATH) $(LD_BOOST_FLAGS)
      CCFLAGS   := -m32
  else
      LDFLAGS   := -L$(CUDA_LIB_PATH)64 -lcudart -lcufft -L$(MPI_LIB_PATH) $(LD_MPIFLAGS) -L$(BOOST_LIB_PATH) $(LD_BOOST_FLAGS)
      CCFLAGS   := -m64
  endif
endif

ifneq ($(USE_SDK),)
  CCFLAGS +=  -DUSE_SDK
  NVCCFLAGS += -DUSE_SDK
endif


# OS-architecture specific flags
ifeq ($(OS_SIZE),32)
      NVCCFLAGS += -m32
else
      NVCCFLAGS += -m64
endif

# OpenGL specific libraries 
ifneq ($(DARWIN),)
    # Mac OSX specific libraries and paths to include
    LIBPATH_OPENGL  := -L$(CUDA_SDK_PATH)/common/lib/$(OSLOWER) -L/System/Library/Frameworks/OpenGL.framework/Libraries -framework GLUT -lGL -lGLU -l$(CUDA_SDK_PATH)/common/lib/$(OSLOWER)/libGLEW.a
else
    # Linux specific libraries and paths to include
    LIBPATH_OPENGL  := -L$(CUDA_SDK_PATH)/common/lib/$(OSLOWER)/$(OS_ARCH) -L/usr/X11R6/lib -lGL -lGLU -lX11 -lXi -lXmu -lglut -lGLEW 
endif

# Debug build flags
ifeq ($(dbg),1)
      CCFLAGS   += -g
      NVCCFLAGS += -g -G
      TARGET    := debug
else
      TARGET    := release
endif


# Common includes and paths for CUDA
INCLUDES      := -I$(CUDA_INC_PATH) -I. -I.. -I./src_nbody -I./src -I$(MPI_INC_PATH) -I$(BOOST_INC_PATH)

ifeq ($(USE_SDK),TRUE) #GL uses SDK Libraries
INCLUDES      += -I$(CUDA_SDK_PATH)/common/inc -I$(CUDA_SDK_PATH)/../shared/inc -I$(MPI_INC_PATH) -I$(BOOST_INC_PATH)
LDFLAGS       += $(LIBPATH_OPENGL)
endif

# Target rules
all: build

build: simpleNBody

cuobj/MultiNBodyDomain.o: src/MultiNBodyDomain.cu src/MultiNBodyDomain.h src/Domain.h src/World.h
	$(NVCC) $(NVCCFLAGS) $(EXTRA_NVCCFLAGS) $(GENCODE_FLAGS) $(INCLUDES) -o $@ -c $<

ccobj/MultiNBodyWorld.o: src/MultiNBodyWorld.cpp src/MultiNBodyWorld.h src/Domain.h src/World.h
	$(GCC) $(CCFLAGS) $(EXTRA_CCFLAGS) $(INCLUDES) -o $@ -c $<

ccobj/simpleNBody.o: src/simpleNBody.cpp
	$(GCC) $(CCFLAGS) $(EXTRA_CCFLAGS) $(INCLUDES) -o $@ -c $<

simpleNBody: ccobj/simpleNBody.o ccobj/MultiNBodyWorld.o cuobj/MultiNBodyDomain.o
	$(GCC) $(CCFLAGS) -o bin/$@ $+ $(LDFLAGS) $(EXTRA_LDFLAGS) $(LD_MPI_FLAGS) $(LD_BOOST_FLAGS)

run: build
	./bin/nbody 

clean:
	rm -f bin/simpleNBody c?obj/*.o

conf:

	mkdir cuobj ccobj bin
