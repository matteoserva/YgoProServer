TARGET = ygopro-server

###############
VERSION=0
include makefile.autoincr.inc

SRC = $(shell ls server/*.c server/*.cpp 2>/dev/null)
#SRC += $(shell ls ygopro/gframe/lzma/*.c ygopro/gframe/lzma/*.cpp 2>/dev/null)
SRC += ygopro-client/gframe/data_manager.cpp ygopro-client/gframe/deck_manager.cpp # ygopro/gframe/replay.cpp
#SRC += $(shell ls ygopro/ocgcore/*.c ygopro/ocgcore/*.cpp 2>/dev/null)

OUT = $(TARGET)
OBJ = $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(SRC)))

.PHONY:	client ocgcore libclzma makefile.autoincr.inc server/Config.o


# include directories
INCLUDES = -I ./server/ -I ./trueskill/src/ -I /usr/include/lua5.2/ -I /usr/include/freetype2/ -I ./ygopro-client/ocgcore/ -I ./ygopro-client/gframe/ -I /usr/include/irrlicht

# C compiler flags (-g -O2 -Wall)
CCFLAGS =   -O0 -g -fstack-protector-all -Wall  -fnon-call-exceptions
CPPFLAGS =  -std=c++11 $(CCFLAGS) 
# -D_GLIBCXX_DEBUG

# compiler
CC = gcc
CPP = g++
CXX= g++
# library paths
LIBS = 

# compile flags
LIBTRUESKILL = trueskill/libtrueskill.a

LDFLAGS = -levent  -lsqlite3 -levent_pthreads ygopro-client/bin/debug/libocgcore.a $(LIBTRUESKILL) -llua5.2 #-lduma  #ygopro/bin/debug/libclzma.a

#LDFLAGS = -levent  -lsqlite3 -levent_pthreads -L./ygopro-client/bin/debug/ -locgcore -llua5.2
LDFLAGS += -lmysqlcppconn



server: $(OUT)


server/Config.o: makefile.autoincr.inc server/Config.cpp
	$(CPP) $(INCLUDES) $(CPPFLAGS) -c server/Config.cpp -o $@ -DVERSION=${VERSION}

ocgcore:
	$(MAKE) -C ygopro-client/build/ ocgcore

trueskill: $(LIBTRUESKILL)

$(LIBTRUESKILL):
	$(MAKE) -C trueskill

libclzma:
	make -C ygopro/build/ clzma

ygopro-replay-decompresser:
	$(CPP) $(INCLUDES) $(CPPFLAGS) -o ygopro-replay-decompresser others/decompress.cpp ygopro-client/bin/debug/libclzma.a 

all: client server

$(OUT): $(OBJ) ocgcore $(LIBTRUESKILL) #libclzma
	$(CPP) $(CPPFLAGS) -o $(OUT) $(OBJ) $(LDFLAGS) -rdynamic

.c.o:
	$(CC) $(INCLUDES) $(CCFLAGS) -c $< -o $@ 

.cpp.o:
	$(CPP) $(INCLUDES) $(CPPFLAGS) -c $< -o $@


clean:	client-clean server-clean

client-clean:
	make -C ygopro-client/build/ clean

server-clean:
	rm -f $(OBJ) $(OUT)
	$(MAKE) -C ygopro-client/build/ clean
	$(MAKE) -C trueskill clean

NEWVERS = `expr $(VERSION) + 1`
makefile.autoincr.inc:
	@mv makefile.autoincr.inc makefile.autoincr.inc.bak || echo VERSION=0 > makefile.autoincr.inc.bak
	@sed "s/^VERSION[ \t]*=[ \t]*[0-9]*/VERSION = $(NEWVERS)/" makefile.autoincr.inc.bak > makefile.autoincr.inc
	@rm makefile.autoincr.inc.bak

