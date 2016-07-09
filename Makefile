TARGET = ygopro-server

###############

include makefile.autoincr.inc

SRC = $(shell ls server/*.c server/*.cpp 2>/dev/null)
#SRC += $(shell ls ygopro/gframe/lzma/*.c ygopro/gframe/lzma/*.cpp 2>/dev/null)
SRC += ygopro-client/gframe/data_manager.cpp ygopro-client/gframe/deck_manager.cpp # ygopro/gframe/replay.cpp
#SRC += $(shell ls ygopro/ocgcore/*.c ygopro/ocgcore/*.cpp 2>/dev/null)

OUT = $(TARGET)
OBJ = $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(SRC)))

.PHONY:	client ocgcore libclzma update-buildnum server/Config.o


# include directories
INCLUDES = -I ./server/ -I /usr/include/lua5.2/ -I /usr/include/freetype2/ -I ./ygopro-client/ocgcore/ -I ./ygopro-client/gframe/ -I /usr/include/irrlicht

# C compiler flags (-g -O2 -Wall)
CCFLAGS =   -O0 -g -fstack-protector-all -Wall -wd858  -fnon-call-exceptions
CPPFLAGS =  -std=c++0x $(CCFLAGS) -D_GLIBCXX_DEBUG

# compiler
CC = icc
CPP = icpc
CXX=icpc
# library paths
LIBS = 

# compile flags
LDFLAGS = -levent  -lsqlite3 -levent_pthreads ygopro-client/bin/debug/libocgcore.a -llua5.2 #-lduma  #ygopro/bin/debug/libclzma.a

#LDFLAGS = -levent  -lsqlite3 -levent_pthreads -L./ygopro-client/bin/debug/ -locgcore -llua5.2
LDFLAGS += -lmysqlcppconn

server: $(OUT)


server/Config.o: update-buildnum server/Config.cpp
	$(CPP) $(INCLUDES) $(CPPFLAGS) -c server/Config.cpp -o $@ -DVERSION=${VERSION}

ocgcore:
	$(MAKE) -C ygopro-client/build/ ocgcore

libclzma:
	make -C ygopro/build/ clzma

all: client server

$(OUT): $(OBJ) ocgcore #libclzma
	$(CPP) $(CPPFLAGS) -o $(OUT) $(OBJ) $(LDFLAGS)

.c.o:
	$(CC) $(INCLUDES) $(CCFLAGS) -c $< -o $@ 

.cpp.o:
	$(CPP) $(INCLUDES) $(CPPFLAGS) -c $< -o $@


clean:	client-clean server-clean

client-clean:
	make -C ygopro-client/build/ clean

server-clean:
	rm -f $(OBJ) $(OUT)
#	$(MAKE) -C ygopro-client/build/ clean


NEWVERS = `expr $(VERSION) + 1`
update-buildnum:
	@mv makefile.autoincr.inc makefile.autoincr.inc.bak
	@sed "s/^VERSION[ \t]*=[ \t]*[0-9]*/VERSION = $(NEWVERS)/" makefile.autoincr.inc.bak > makefile.autoincr.inc
	@rm makefile.autoincr.inc.bak

