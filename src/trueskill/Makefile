# ex.
# make - makes the solution with debug settings
# make BUILD=release - makes the solution with optimizations turned on
# make clean - cleans the build

CC := g++ # this is the main compiler
SRCDIR := src
BUILDDIR := build
TARGET := bin/runner
 
SRCEXT := cpp
SOURCES := $(shell find $(SRCDIR) -type f -name "*.$(SRCEXT)" ! -name "python_wrapper.cpp")
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.o))
CFLAGS := -g -DDEBUG -O0 -fpic -std=c++11  # -Wall
ifeq ($(BUILD), release)
CFLAGS := -Os
endif
LIB := -pthread #-lmongoclient -L lib -lboost_thread-mt -lboost_filesystem-mt -lboost_system-mt
INC := -I include

$(TARGET): main.cpp libtrueskill.a libtrueskill.so
	@echo " Linking..."
	@echo " $(CC) $^ -o $(TARGET) $(LIB)"; $(CC) $^ -I $(SRCDIR) -o $(TARGET) $(LIB)

libtrueskill.so: $(OBJECTS)
	gcc -shared -o $@ $^ $(LIB)
libtrueskill.a : $(OBJECTS)
	ar -cvq $@ $^ 

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(BUILDDIR)
	@echo " $(CC) $(CFLAGS) $(INC) -c -o $@ $<"; $(CC) $(CFLAGS) $(INC) -c -o $@ $<

clean:
	@echo " Cleaning..."; 
	@echo " $(RM) -r $(BUILDDIR) $(TARGET)"; $(RM) -r $(BUILDDIR) $(TARGET)
	-rm *.a *.so

# Tests
tester:
	$(CC) $(CFLAGS) test/tester.cpp $(INC) $(LIB) -o bin/tester

# Spikes
ticket:
	$(CC) $(CFLAGS) spikes/ticket.cpp $(INC) $(LIB) -o bin/ticket

.PHONY: clean
