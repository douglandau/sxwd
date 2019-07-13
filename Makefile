

SRCS = sxwd.cc xwd.cc ppm.cc font.cc 
OBJS = $(SRCS:.cc=.o)
PROGS = sxwd 

CC=c++
DEBUG_OPTS = -g
#COPTS = -O
CFLAGS = $(OPT) $(COPTS) $(DEBUG_OPTS) $(INCLUDE) $(XINCLUDE)
LIBS = -lz 
INSTALL=cp
INSTALLFLAGS=-p
BIN_DIR=$(TOPOROOT)/bin

all:	rgb.h $(PROGS) 

rgb.h:
	 ./rgb2c.pl X11/etc/rgb.txt  > rgb.h

sxwd:	sxwd.o xwd.o ppm.o font.o
	$(CC) $(LDFLAGS) -o sxwd sxwd.o xwd.o font.o ppm.o $(LIBS)


install:	$(PROGS)
	$(INSTALL) $(INSTALLFLAGS) sxwd $(BIN_DIR)

clean:
	$(RM) $(OBJS) *% ,* *.o rgb.h *.BAK *.CKP core $(PROGS)

.INIT:	$(SRCS)

.KEEP_STATE:

%.o:	%.cc
	$(CC) -c $*.cc $(CFLAGS)


