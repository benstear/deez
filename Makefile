CC = g++
CFLAGS = -c -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE  -std=c++0x -DOPENSSL  -pthread
LDFLAGS = -pthread -lz -lcurl -lcrypto 

GIT_VERSION := $(shell git describe --dirty --always --tags)
SOURCES := $(shell find . -name '*.cc' -not -path "./run/*" -not -path "./Java/*")
OBJECTS = $(SOURCES:.cc=.o)
EXECUTABLE = deez
LIB = libdeez
TESTEXE = deeztest

debug: CFLAGS += -g -fno-omit-frame-pointer
debug: $(SOURCES) $(EXECUTABLE)

superdebug: CFLAGS += -g -O0 -fno-inline
superdebug: $(SOURCES) $(EXECUTABLE)

profile: CFLAGS += -g -pg -O3
profile: LDFLAGS += -pg
profile: $(SOURCES) $(EXECUTABLE)

all: CFLAGS += -O3 -DNDEBUG 
all: $(SOURCES) $(EXECUTABLE)

gprofile: LDFLAGS += -ltcmalloc -lprofiler
gprofile: CFLAGS += -g
gprofile: $(SOURCES) $(EXECUTABLE)

test: CFLAGS += -O3 -DNDEBUG -std=c++11
test: $(SOURCES) $(TESTEXE)

testdebug: CFLAGS += -g -std=c++11
testdebug: $(SOURCES) $(TESTEXE)

lib: CFLAGS += -O3 -DNDEBUG -fpic -DDEEZLIB
lib: $(SOURCES) $(LIB)

libdebug: CFLAGS += -g -fpic -DDEEZLIB
libdebug: $(SOURCES) $(LIB)

$(TESTEXE): OBJECTS := $(subst ./Main.o,,$(OBJECTS))
$(TESTEXE): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

$(EXECUTABLE): OBJECTS := $(subst ./Test.o,,$(OBJECTS))
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

$(LIB): OBJECTS := $(subst ./Test.o,,$(OBJECTS))
$(LIB): $(OBJECTS) 
	$(CC) $(OBJECTS) $(LDFLAGS) -fpic -shared -o $@.so
	rm -rf $(LIB).a
	ar rvs $(LIB).a $(OBJECTS) 

.cc.o:	
	$(CC) $(CFLAGS) -DVER=\"$(GIT_VERSION)\" $< -o $@

clean:
	find . -name '*.o' -delete
	rm -rf $(EXECUTABLE) $(TESTEXE)
	rm -rf gmon.out* 
