#! /usr/bin/make

.NULL: .exe

SOURCES = $(wildcard *.cpp) #$(wildcard *.c)
HEADERS = $(wildcard *.h)
TARGETS = $(basename $(SOURCES))
OBJECTS = ../../Common/InitShader.o

uname := $(shell sh -c uname)
#uname := $(shell sh -c 'uname -s 2>/dev/null || echo not')

CXXOPTS= -Wall -O3 -g
CXXDEFS= -D_DEBUG
CXXINCS= -I../../include

# Linux
ifneq (,$(findstring Linux,$(uname)))
	LDLIBS = -lglut -lGL -lXmu -lX11  -lm
	LDDIRS = -L../../lib/linux
	LDLIBS = -lassimp -lGLEW -lglut -lGL -lXmu -lX11  -lm -Wl,-rpath,. #-rpath,../../lib/linux
endif

# windows MinGW
ifneq (,$(findstring MINGW,$(uname)))
	LDDIRS= -L../../lib/win32
	LDLIBS= -lassimp -lfreeglut_static -lglew32 -lopengl32 -lgdi32 -lwinmm

	CXXDEFS += -DFREEGLUT_STATIC -DGLEW_STATIC
	CXXOUT=
	# -Wl --subsystem,windows
endif

#windows VC
#mac OSX (Darwin)


CXXFLAGS= $(CXXOPTS) $(CXXDEFS) $(CXXINCS)
LDFLAGS= $(LDOPTS) $(LDDIRS) $(LDLIBS)

DIRT = $(wildcard *.o *.i *~ */*~ *.log *.exe)

#-----------------------------------------------------------------------------

.PHONY: Makefile gnatidread.h

default all: $(TARGETS)

$(TARGETS): $(OBJECTS)

%: %.cpp gnatidread.h
	$(CXX) $(CXXFLAGS) $@.cpp $(OBJECTS) $(LDFLAGS) -o $(CXXOUT)$@

%: %.c
	gcc $(CXXFLAGS) $^ $(LDFLAGS) -o $(CXXOUT)$@

clean:
	$(RM) $(DIRT)

rmtargets:
	$(RM) $(TARGETS) $(OBJECTS)

clobber: clean rmtargets
