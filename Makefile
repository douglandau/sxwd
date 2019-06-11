

SRCS = sxwd.cc xwd.cc ppm.cc font.cc 
OBJS = $(SRCS:.cc=.o)
PROGS = sxwd 

LIBS = -lm -lX11

#CC=g++
CC=c++

XINCLUDE = -I/usr/X11R6/include
INCLUDE = -I/usr/local/include -I.
DEBUG_OPTS = -g
#COPTS = -O
CFLAGS = $(OPT) $(COPTS) $(DEBUG_OPTS) $(INCLUDE) $(XINCLUDE)
XLDFLAGS = -L/usr/X11R6/lib
LDFLAGS = -L/usr/local/lib 
LIBS = -lz -lm
XLIBS = -lXaw -lXt -lX11 
XLIBS = -lX11 
INSTALL=cp
INSTALLFLAGS=-p
BIN_DIR=$(TOPOROOT)/bin


all:	$(PROGS)

sxwd:	sxwd.o xwd.o ppm.o font.o
	$(CC) $(LDFLAGS) $(XLDFLAGS) -o sxwd sxwd.o xwd.o font.o ppm.o $(XLIBS) $(LIBS)

install:	$(PROGS)
	$(INSTALL) $(INSTALLFLAGS) sxwd $(BIN_DIR)


clean:
	$(RM) $(OBJS) *% ,* *.o *.BAK *.CKP core $(PROGS)

.INIT:	$(SRCS)

.KEEP_STATE:

%.o:	%.cc
	$(CC) -c $*.cc $(CFLAGS)

#xwd.o:	xwd.cc

# DO NOT DELETE THIS LINE -- make depend depends on it.
