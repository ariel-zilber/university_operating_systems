################################### parameters #########################################################################

# compilers
CC=g++
CXX=g++
RANLIB=ranlib

# sources
LIBSRC=uthreads.cpp Thread.cpp

# headers
HEADERS= Thread.h

# object files
LIBOBJ=$(LIBSRC:.cpp=.o)

# compilations flags
INCS=-I.
CFLAGS = -Wall -std=c++11 -g $(INCS)
CXXFLAGS = -Wall -std=c++11 -g $(INCS)

# libray compilation parameters
UTHREADS_LIB = libuthreads.a
TARGETS = $(UTHREADS_LIB)

### tar related parameters
TAR=tar
TARFLAGS=-cvf
TARNAME=ex2.tar
TARSRCS=$(LIBSRC) $(HEADERS) Makefile README

############################################ build rules ###############################################################

all: $(TARGETS)

$(TARGETS): $(LIBOBJ)
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@

clean:
	$(RM) $(TARGETS) $(UTHREADS_LIB) $(OBJ) $(LIBOBJ) *~ *core

depend:
	makedepend -- $(CFLAGS) -- $(SRC) $(LIBSRC)

tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)