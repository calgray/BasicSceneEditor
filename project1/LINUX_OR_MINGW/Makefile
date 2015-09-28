#! /usr/bin/make

#uname := $(shell sh -c 'uname -s 2>/dev/null || echo not')
uname := $(shell sh -c uname)

CXXOPTS= -Wall -O3 -g
CXXDEFS=
CXXINCS= -I../../include

# Linux
ifneq (,$(findstring Linux,$(uname)))
	LDLIBS= -lglut -lGL -lXmu -lX11  -lm
	LDDIRS= -L../../lib/linux
	LDLIBS= -lassimp -lGLEW -lglut -lGL -lXmu -lX11  -lm -Wl,-rpath,. #-rpath,../../lib/linux
	BINDIR= bin/linux/
endif

#Windows MinGW
ifneq (,$(findstring MINGW,$(uname)))
	LDDIRS= -L../../lib/win32
	LDLIBS= -lassimp -lfreeglut_static -lglew32 -lopengl32 -lgdi32 -lwinmm
	# -Wl --subsystem,windows
	CXXDEFS += -DFREEGLUT_STATIC -DGLEW_STATIC
	BINDIR= bin/mingw/
endif

#Windows VC
#mac OSX (Darwin)

#-PLATFORM DEPENDANT---------------------------------------------------------------

SOURCES= $(wildcard *.cpp) 
#$(wildcard *.c)
#TARGETS = $(basename $(BINDIR)$(SOURCES))
TARGETS= $(SOURCES:%.cpp=$(BINDIR)%)
OBJECTS = ../../Common/InitShader.o

CXXFLAGS= $(CXXOPTS) $(CXXDEFS) $(CXXINCS)
LDFLAGS= $(LDOPTS) $(LDDIRS) $(LDLIBS)

DIRT = $(wildcard *.o *.i *~ */*~ *.log *.exe)

#-----------------------------------------------------------------------------

default all: $(BINDIR) $(TARGETS)

$(BINDIR):
	mkdir -p $(BINDIR)

$(TARGETS): $(OBJECTS)

$(BINDIR)%: %.cpp
	$(CXX) $(CXXFLAGS) $< $(OBJECTS) $(LDFLAGS) -o $@

#$(BINDIR)%: %.c
#	gcc $(CXXFLAGS) $^ $(LDFLAGS) -o $@

clean:
	$(RM) $(DIRT)

rmtargets:
	$(RM) $(TARGETS) $(OBJECTS)

clobber: clean rmtargets
