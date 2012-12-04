# -*- mode: Makefile; -*-
# -----------------------------------------
# project PV


export PATH := /opt/wx/2.8/bin:$(PATH)
export LD_LIBRARY_PATH := /opt/wx/2.8/lib:$(LD_LIBRARY_PATH)

_WX = /home/gr/projects/gui/codeblocks/wx
_WX.LIB = $(_WX)/lib
_WX.INCLUDE = $(_WX)/include

_CB = /home/gr/projects/gui/codeblocks/cb/src
_CB.INCLUDE = $(_CB)/include
_CB.LIB = $(_CB)/devel



CFLAGS_C = $(filter-out -include "sdk.h",$(CFLAGS))

# -----------------------------------------

# MAKE_DEP = -MMD -MT $@ -MF $(@:.o=.d)

BOOST_INC_PATH = ../lib/boost_1_52_0
BOOST_LIB_PATH = ../lib/boost_1_52_0/stage/lib

GLUT_INC_PATH = ../lib/freeglut-2.6.0/install/include
GLUT_LIB_PATH = ../lib/freeglut-2.6.0/install/lib

GLEW_INC_PATH = ../lib/glew-1.9.0/include
GLEW_LIB_PATH = ../lib/glew-1.9.0/lib

CFLAGS = -Wall 
INCLUDES = -I$(BOOST_INC_PATH) -I$(GLUT_INC_PATH) -I$(GLEW_INC_PATH)
LDFLAGS =  -s -L$(BOOST_LIB_PATH) -L$(GLUT_LIB_PATH) -L$(GLEW_LIB_PATH)
RCFLAGS = 
LDLIBS = $(T_LDLIBS) -lGL -lglut -lX11 -lGLEW -lboost_thread -lboost_system -lstdc++

LINK_exe = gcc -o $@ $^ $(LDFLAGS) $(LDLIBS)
LINK_con = gcc -o $@ $^ $(LDFLAGS) $(LDLIBS)
LINK_dll = gcc -o $@ $^ $(LDFLAGS) $(LDLIBS) -shared
LINK_lib = rm -f $@ && ar rcs $@ $^
COMPILE_c = gcc $(CFLAGS_C) -o $@ -c $< $(MAKEDEP) $(INCLUDES)
COMPILE_cpp = g++ $(CFLAGS) -o $@ -c $< $(MAKEDEP) $(INCLUDES)
COMPILE_rc = windres $(RCFLAGS) -J rc -O coff -i $< -o $@ -I$(dir $<)

%.o : %.c ; $(COMPILE_c)
%.o : %.cpp ; $(COMPILE_cpp)
%.o : %.cxx ; $(COMPILE_cpp)
%.o : %.rc ; $(COMPILE_rc)
.SUFFIXES: .o .d .c .cpp .cxx .rc

all: all.before all.targets all.after

all.before :
	-
all.after : $(FIRST_TARGET)
	
all.targets : Debug_target Release_target 

clean :
	rm -fv $(clean.OBJ)
	rm -fv $(DEP_FILES)
	rm -fr *~

.PHONY: all clean distclean

# -----------------------------------------
# Debug_target

Debug_target.BIN = bin/Debug/PV
Debug_target.OBJ = InitShader.o particles.o 
DEP_FILES += InitShader.d particles.d 
clean.OBJ += $(Debug_target.BIN) $(Debug_target.OBJ)

Debug_target : Debug_target.before $(Debug_target.BIN) Debug_target.after_always
Debug_target : CFLAGS += -g  -Os
Debug_target : INCLUDES += 
Debug_target : RCFLAGS += 
Debug_target : LDFLAGS +=   
Debug_target : T_LDLIBS = 
ifdef LMAKE
Debug_target : CFLAGS -= -O1 -O2 -g -pipe
endif

Debug_target.before :
	
	
Debug_target.after_always : $(Debug_target.BIN)
	
$(Debug_target.BIN) : $(Debug_target.OBJ)
	$(LINK_con)
	

# -----------------------------------------
# Release_target

Release_target.BIN = bin/Release/PV
Release_target.OBJ = InitShader.o particles.o 
DEP_FILES += InitShader.d particles.d 
clean.OBJ += $(Release_target.BIN) $(Release_target.OBJ)

Release_target : Release_target.before $(Release_target.BIN) Release_target.after_always
Release_target : CFLAGS += -O2  -Os
Release_target : INCLUDES += 
Release_target : RCFLAGS += 
Release_target : LDFLAGS += -s   
Release_target : T_LDLIBS = 
ifdef LMAKE
Release_target : CFLAGS -= -O1 -O2 -g -pipe
endif

Release_target.before :
	
	
Release_target.after_always : $(Release_target.BIN)
	
$(Release_target.BIN) : $(Release_target.OBJ)
	$(LINK_exe)
	

# -----------------------------------------
ifdef MAKE_DEP
-include $(DEP_FILES)
endif
