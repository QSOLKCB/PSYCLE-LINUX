CC = g++
INC = -I ../../ -I ../../psycle -I ../../../dsp/src
LDFLAGS = -ldsp -shared
LIBDIRS = -L ../../../dsp/src
OUTPUTFILE = legasynth-303.so
OUTPUTDIR = ../../build/
src = $(wildcard *.cpp) \
	  $(wildcard lib/*.cpp) \
	  $(wildcard 303/*.cpp)
obj = $(src:.cpp=.o)

all: CFLAGS = -O3 -DNDEBUG
all: $(OUTPUTFILE)

debug: CFLAGS = -DDEBUG -g3
debug: $(OUTPUTFILE)

$(OUTPUTFILE): $(obj)
	$(CC) $(CFLAGS) $(LIBDIRS) $(INC) -fPIC -o $(OUTPUTDIR)$@ $^ $(LDFLAGS)
	
%.o: %.cpp
	$(CC) $(CFLAGS) $(INC) -c $^ -fPIC -o $@	
	
.PHONY: clean	
clean:
	rm -f $(obj) $(OUTPUTFILE)

