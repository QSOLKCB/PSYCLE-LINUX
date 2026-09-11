inc = $(shell pkg-config --libs --cflags alsa)\
      -I../../dsp/src -I../../container/src
src = $(wildcard *.c)
obj = $(src:.c=.o)
OUTPUTFILE = libpsyalsa.so
OUTPUTDIR = ../build/
CFLAGS = -Wfatal-errors $(inc)
LDFLAGS = $(shell pkg-config --libs --cflags alsa)\
          -lcontainer -ldsp -shared
LIBDIRS = -L ../../container/src -L ../../dsp/src

all: CFLAGS += -O3 -DNDEBUG
all: $(OUTPUTFILE)

debug: CFLAGS += -DDEBUG -g3
debug: $(OUTPUTFILE)

$(OUTPUTFILE): $(obj)
	$(CC) $(CFLAGS) $(LIBDIRS) $(INC) -o $(OUTPUTDIR)$@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -fPIC $(src)

.PHONY: clean	
clean:
	rm -f $(obj) libpsyalsa.so
