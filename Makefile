TARGET = ygopro-server

###############

SRC = $(shell ls server/*.c server/*.cpp 2>/dev/null)
SRC += $(shell ls ygopro/gframe/lzma/*.c ygopro/gframe/lzma/*.cpp 2>/dev/null)
SRC += ygopro/gframe/data_manager.cpp ygopro/gframe/deck_manager.cpp ygopro/gframe/replay.cpp
SRC += $(shell ls ygopro/ocgcore/*.c ygopro/ocgcore/*.cpp 2>/dev/null)

OUT = $(TARGET)
OBJ = $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(SRC)))

.PHONY:	client


# include directories
INCLUDES = -I ./server/ -I /usr/include/lua5.2/ -I /usr/include/freetype2/ -I ./ygopro/ocgcore/ -I ./ygopro/gframe/  -I /usr/include/irrlicht/

# C compiler flags (-g -O2 -Wall)
CCFLAGS =  -O0 -g 
CPPFLAGS = -std=c++0x -O0 -g

# compiler
CC = gcc
CPP = g++

# library paths
LIBS = 

# compile flags
LDFLAGS = -levent -llua5.2 -lsqlite3 -levent_pthreads

default: client server
server: $(OUT)



$(OUT): $(OBJ)                                                                                                                                                                               
	$(CPP) -o $(OUT) $(OBJ) $(LDFLAGS)

.c.o:
	$(CC) $(INCLUDES) $(CCFLAGS) -c $< -o $@

.cpp.o:
	$(CPP) $(INCLUDES) $(CPPFLAGS) -c $< -o $@

clean:	client-clean server-clean

client:
	make -C client/

client-clean:
	make -C client/ clean

server-clean:
	rm -f $(OBJ) $(OUT)
