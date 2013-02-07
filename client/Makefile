## This compiles the client###
## Ignore this file	  ###

IRRLICHT_SOURCE_DIR = ./irrlicht-1.7.2+dfsg1/source/Irrlicht/

SRC = $(shell ls ../ygopro/gframe/*.c ../ygopro/gframe/*.cpp 2>/dev/null)
SRC += $(shell ls ../ygopro/ocgcore/*.c ../ygopro/ocgcore/*.cpp 2>/dev/null)
SRC += $(shell ls ../ygopro/gframe/lzma/*.c ../ygopro/gframe/lzma/*.cpp 2>/dev/null)

TARGET = ygopro-client
OUT = ../$(TARGET)
OBJ = $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(SRC)))

# include directories
INCLUDES =  -I /usr/include/lua5.2/ -I /usr/include/irrlicht/ -I /usr/include/freetype2/  -I ../ygopro/ocgcore/ -I ../ygopro/gframe/ -I ./

# C compiler flags (-g -O2 -Wall)
CCFLAGS = -O0 -g
CPPFLAGS = -std=c++0x -O0 -g 
# compiler
CC = gcc
CPP = g++

# library paths
LIBS = 

# compile flags
LDFLAGS = -levent -llua5.2 -lGL -lGLU -lsqlite3  -lfreetype  -levent_pthreads -lIrrlicht

default: $(OUT)

$(OUT): $(OBJ)                                                                                                                                                                               
	$(CPP) -o $(OUT) $(OBJ) $(LDFLAGS)


.c.o:
	$(CC) $(INCLUDES) $(CCFLAGS) -c $< -o $@

.cpp.o:
	$(CPP) $(INCLUDES) $(CPPFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(OUT) 
