TARGET_STATIC = libmanager.a
TARGET_DYNAMIC = libmanager.so
###############

SRC_MANAGER = $(shell ls ./*.c ./*.cpp 2>/dev/null)
INC_MANAGER = $(shell ls ./*.h ./*.hpp 2>/dev/null)

INC_COMMON = $(shell ls ../common/*.h ../common/*.hpp 2>/dev/null)
INC_DATABASE = $(shell ls ../database/*.h ../database/*.hpp 2>/dev/null)
HEADERS = $(INC_MANAGER) $(INC_COMMON) $(INC_DATABASE)


OUT = $(TARGET)
OBJ = $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(SRC_MANAGER)))

# include directories
INCLUDES = -I ./ -I .. -I ../database

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

