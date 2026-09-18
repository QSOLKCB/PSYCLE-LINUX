CC = g++
INC = -I ../../ -I ../../psycle
LIBDIRS =
# The Phase 5 persistence regression has reproduced an intermittent teardown
# SIGSEGV when this historical native module is unloaded after FluidSynth
# instances are destroyed. Keep only this module resident for the process
# lifetime; do not broaden the host loader policy for unrelated plugins.
LDFLAGS = -lfluidsynth -shared -Wl,-z,nodelete
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
