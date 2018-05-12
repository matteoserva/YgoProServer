TARGET = build/ygopro-newserver


###############


SRC_MAIN = $(shell ls main/*.c main/*.cpp 2>/dev/null)
SRC_TESTS = $(shell ls tests/*.c tests/*.cpp 2>/dev/null)
SRC = $(SRC_MAIN)  $(SRC_TESTS)

INC_COMMON = $(shell ls common/*.h common/*.hpp 2>/dev/null)
INC_SERVER = $(shell ls server/*.h server/*.hpp 2>/dev/null)





OUT = $(TARGET)
OBJ = $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(SRC)))



# include directories
INCLUDES = -I . -I ./tests/ -I ./database -I ./server/ -I ./common -I ./manager -I ./trueskill/src/ -I /usr/include/lua5.2/ -I /usr/include/freetype2/ -I ./ygopro-client/ocgcore/ -I ./ygopro-client/gframe/ -I /usr/include/irrlicht

# C compiler flags (-g -O2 -Wall)
CCFLAGS =   -O0 -g -fstack-protector-all -Wall  -fnon-call-exceptions
CPPFLAGS =  -std=c++0x $(CCFLAGS) 
#-D_GLIBCXX_DEBUG

# compiler
CC = gcc
CPP = g++
CXX= g++
# library paths
LIBS = 



LDFLAGS = -levent  -lsqlite3 -levent_pthreads ygopro-client/bin/debug/libocgcore.a $(LIBTRUESKILL) -llua5.2 -lpthread 


LDFLAGS += -lmysqlcppconn
SUBLIBS = common server database manager trueskill
SUBLIBS-CLEAN = $(patsubst %,%-clean,$(SUBLIBS))
SUBLIBS_ARCHIVES = server/libserver.a  manager/libmanager.a database/libdatabase.a trueskill/libtrueskill.a common/libcommon.a 


.PHONY:	client ocgcore libclzma  $(SUBLIBS) $(SUBLIBS-CLEAN)


$(OUT): $(OBJ) build $(SUBLIBS)
	$(CPP) -I . -I tests -I server -I manager -I database -I common $(CPPFLAGS) -o $(OUT) $(OBJ) -Lbuild $(SUBLIBS_ARCHIVES)  $(LDFLAGS) -rdynamic -Wl,--rpath=.
#	$(CPP) -I . -I tests -I server -I manager -I database -I common $(CPPFLAGS) -o $(OUT) $(OBJ) -Lbuild -lmanager -lserver -ldatabase -ltrueskill -lcommon $(LDFLAGS) -rdynamic -Wl,--rpath=.

build:
	mkdir build

ocgcore: build
	$(MAKE) -C ygopro-client/build/ ocgcore
	cp ygopro-client/bin/debug/libocgcore.a build/libocgcore.a

libclzma: build
	make -C ygopro-client/build/ clzma
	cp ygopro-client/bin/debug/libclzma.a build/libclzma.a

build/ygopro-replay-decompresser: others/decompress.cpp libclzma build
	$(CPP) $(INCLUDES) $(CPPFLAGS) -o build/ygopro-replay-decompresser others/decompress.cpp build/libclzma.a 


	
libraries: common database server manager trueskill ocgcore libclzma



.c.o:
	$(CC) $(INCLUDES) $(CCFLAGS) -c $< -o $@ 

.cpp.o:
	$(CPP) $(INCLUDES) $(CPPFLAGS) -c $< -o $@


server: build ocgcore
	$(MAKE) -C server
	cp server/libserver.so build/libserver.so
	cp server/libserver.a build/libserver.a
	
server-clean:
	-$(MAKE) -C server clean	


common: build
	$(MAKE) -C common
	cp common/libcommon.so build/libcommon.so
	cp common/libcommon.a build/libcommon.a
	
common-clean:
	-$(MAKE) -C common clean	


database: build
	$(MAKE) -C database
	cp database/libdatabase.so build/libdatabase.so
	cp database/libdatabase.a build/libdatabase.a
	
database-clean:
	-$(MAKE) -C database clean	

manager: build
	$(MAKE) -C manager
	cp manager/libmanager.so build/libmanager.so
	cp manager/libmanager.a build/libmanager.a
	
manager-clean:
	-$(MAKE) -C manager clean
	
trueskill: build
	$(MAKE) -C trueskill
	cp trueskill/libtrueskill.so build/libtrueskill.so
	cp trueskill/libtrueskill.a build/libtrueskill.a
	
trueskill-clean:
	-$(MAKE) -C trueskill clean

clean:	client-clean server-clean common-clean database-clean manager-clean trueskill-clean
	-rm -r build
	-rm -f $(OBJ) $(OUT)	

client:
	$(MAKE) -C ygopro-client/build/
	
client-clean:
	$(MAKE) -C ygopro-client/build/ clean


	
	

