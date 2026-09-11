CC = g++
INC = -I ../../ -I ../../../dsp/src
LDFLAGS = -lcontainer -ldsp -shared
LIBDIRS = -L ../../../container/src -L ../../../dsp/src
OUTPUTFILE = pooplog-fm-laboratory.so
OUTPUTDIR = ../../build/
src = $(wildcard *.cpp)
obj = $(src:.cpp=.o)

all: CFLAGS = -O3 -DNDEBUG
all: $(OUTPUTFILE)

debug: CFLAGS = -DDEBUG -g3
debug: $(OUTPUTFILE)

$(OUTPUTFILE): $(obj)
	$(CC) $(CFLAGS) $(LIBDIRS) $(INC) -o $(OUTPUTDIR)$@ $^ $(LDFLAGS)
	
%.o: %.cpp
	$(CC) $(CFLAGS) $(INC) -c $(src) -fPIC
	
.PHONY: clean	
clean:
	rm -f $(obj) $(OUTPUTFILE)

