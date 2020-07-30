################################### parameters #########################################################################

# compilers
CC=g++
CXX=g++
RANLIB=ranlib

# sources
LIBSRC=osm.cpp

# object files
LIBOBJ=$(LIBSRC:.cpp=.o)

# extra files
PNG_FILES=$(wildcard *.png)

# compilations flags
INCS=-I.
CFLAGS = -Wall -std=c++11 -g $(INCS)
CXXFLAGS = -Wall -std=c++11 -g $(INCS)

# libray compilation parameters
OSMLIB = libosm.a
TARGETS = $(OSMLIB)


### tar related parameters
TAR=tar
TARFLAGS=-cvf
TARNAME=ex1.tar
TARSRCS=$(LIBSRC) Makefile README  $(HEADERS) $(PNG_FILES)


############################################ build rules################################################################

all: $(TARGETS)

$(TARGETS): $(LIBOBJ)
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@

clean:
	$(RM) $(TARGETS) $(OSMLIB) $(OBJ) $(LIBOBJ) *~ *core

depend:
	makedepend -- $(CFLAGS) -- $(SRC) $(LIBSRC)

tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)