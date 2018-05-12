TARGET_STATIC = libserver.a
TARGET_DYNAMIC = libserver.so

###############

SRC_SERVER = $(shell ls ./*.c ./*.cpp 2>/dev/null) ../ygopro-client/gframe/data_manager.cpp ../ygopro-client/gframe/deck_manager.cpp
INC_SERVER = $(shell ls ./*.h ./*.hpp 2>/dev/null)

INC_COMMON = $(shell ls ../common/*.h ../common/*.hpp 2>/dev/null)
HEADERS = $(INC_SERVER) $(INC_COMMON)


OUT = $(TARGET)
OBJ = $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(SRC_SERVER)))

# include directories
INCLUDES = -I ./ -I .. -I /usr/include/lua5.2/ -I /usr/include/freetype2/ -I ../ygopro-client/ocgcore/ -I ../ygopro-client/gframe/ -I /usr/include/irrlicht

# C compiler flags (-g -O2 -Wall)
CCFLAGS =   -O0 -g -fPIC -fstack-protector-all -Wall  -fnon-call-exceptions
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



