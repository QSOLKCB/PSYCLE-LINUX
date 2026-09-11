INC = $(shell pkg-config --libs --cflags sdl2)\
      -I../../dsp/src -I../../container/src
src = $(wildcard *.c)
obj = $(src:.c=.o)
OUTPUTFILE = libpsysdl2.so
OUTPUTDIR = ../build/
CFLAGS = $(INC) -Wfatal-errors -O3
LDFLAGS = $(shell pkg-config --libs --define-prefix sdl2)\
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
	rm -f $(obj) libpsysdl2.so
