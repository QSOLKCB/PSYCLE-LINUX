CC = g++
INC = -I ../../ -I ../../psycle
LIBDIRS =
LDFLAGS = -lfluidsynth -shared
OUTPUTFILE = fluidsynth.so
OUTPUTDIR = ../../build/
src = $(wildcard *.cpp)
obj = $(src:.cpp=.o)

all: CFLAGS = -O3 -DNDEBUG
all: $(OUTPUTFILE)

debug: CFLAGS = -DDEBUG -g3
debug: $(OUTPUTFILE)

$(OUTPUTFILE): $(obj)
	mkdir -p $(OUTPUTDIR)
	$(CC) $(CFLAGS) $(LIBDIRS) $(INC) -o $(OUTPUTDIR)$@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CC) $(CFLAGS) $(INC) -c $< -o $@ -fPIC

.PHONY: clean
clean:
	rm -f $(obj) $(OUTPUTFILE) $(OUTPUTDIR)$(OUTPUTFILE)
