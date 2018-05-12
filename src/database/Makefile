TARGET_STATIC = libdatabase.a
TARGET_DYNAMIC = libdatabase.so
###############

SRC_DATABASE = $(shell ls ./*.c ./*.cpp 2>/dev/null)
INC_DATABASE = $(shell ls ./*.h ./*.hpp 2>/dev/null)

INC_COMMON = $(shell ls ../common/*.h ../common/*.hpp 2>/dev/null)
HEADERS = $(INC_DATABASE) $(INC_COMMON)


OUT = $(TARGET)
OBJ = $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(SRC_DATABASE)))

# include directories
INCLUDES = -I ./ -I .. -I ../trueskill/src

# C compiler flags (-g -O2 -Wall)
CCFLAGS =   -O0 -g -fpic -fstack-protector-all -Wall  -fnon-call-exceptions
CPPFLAGS =  -std=c++0x $(CCFLAGS) 
#-D_GLIBCXX_DEBUG

# compiler
CC = gcc
CPP = g++
CXX= g++
# library paths
LIBS = 

all: $(TARGET_DYNAMIC) $(TARGET_STATIC)
	

$(TARGET_DYNAMIC): $(OBJ)
	gcc -shared -o $@ $^ $(LIBS)

$(TARGET_STATIC): $(OBJ)
	ar -cvq $@ $^

.c.o: $(HEADERS)
	$(CC) $(INCLUDES) $(CCFLAGS) -c $< -o $@ 

.cpp.o: $(HEADERS)
	$(CPP) $(INCLUDES) $(CPPFLAGS) -c $< -o $@


clean: 
	rm $(TARGET_STATIC)
	rm $(TARGET_DYNAMIC)
	rm $(OBJ)
