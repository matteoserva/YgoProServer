TARGET_STATIC = libcommon.a
TARGET_DYNAMIC = libcommon.so

###############
VERSION=0
include makefile.autoincr.inc

SRC_COMMON = $(shell ls ./*.c ./*.cpp 2>/dev/null)

INC_COMMON = $(shell ls ./*.h ./*.hpp 2>/dev/null)


OUT = $(TARGET)
OBJ = $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(SRC_COMMON)))

.PHONY:	makefile.autoincr.inc


# include directories
INCLUDES = -I ./ -I ..

# C compiler flags (-g -O2 -Wall)
CCFLAGS =   -O0 -g -fpic -fstack-protector-all -Wall  -fnon-call-exceptions
CPPFLAGS =  -std=c++0x $(CCFLAGS) 
#-D_GLIBCXX_DEBUG

# compiler
CC = gcc
CPP = g++
CXX= g++
# library paths
LIBS = -lpthread


all: $(TARGET_DYNAMIC) $(TARGET_STATIC)
	

$(TARGET_DYNAMIC): $(OBJ)
	gcc -shared -o $@ $^ $(LIBS)

$(TARGET_STATIC): $(OBJ)
	ar -cvq $@ $^ 


Config.o: makefile.autoincr.inc Config.cpp
	$(CPP) $(INCLUDES) $(CPPFLAGS) -c Config.cpp -o $@ -DVERSION=${VERSION}




%.c.o: $(INC_COMMON)
	$(CC) $(INCLUDES) $(CCFLAGS) -c $< -o $@ 

%.cpp.o: $(INC_COMMON)
	$(CPP) $(INCLUDES) $(CPPFLAGS) -c $< -o $@


clean: 
	rm $(TARGET_STATIC)
	rm $(TARGET_DYNAMIC)
	rm $(OBJ)

NEWVERS = `expr $(VERSION) + 1`
makefile.autoincr.inc:
	@mv makefile.autoincr.inc makefile.autoincr.inc.bak || echo VERSION=0 > makefile.autoincr.inc.bak
	@sed "s/^VERSION[ \t]*=[ \t]*[0-9]*/VERSION = $(NEWVERS)/" makefile.autoincr.inc.bak > makefile.autoincr.inc
	@rm makefile.autoincr.inc.bak

