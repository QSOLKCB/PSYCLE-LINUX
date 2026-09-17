CC = g++
INC = -I ../../ -I ../../psycle
LIBDIRS =
# FluidSynth on current Linux distributions may use OpenMP internally while
# loading/rendering SoundFonts.  libgomp keeps a worker pool alive beyond the
# lifetime of an individual synth instance, so unloading this plugin with
# dlclose() can unmap the OpenMP-backed dependency graph while those workers
# still exist.  Keep this one historical native module resident for the process
# lifetime instead of broadening the host loader policy for every plugin.
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
