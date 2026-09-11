inc = -I ../src -I../../thread/src -I ../src -I../../container/src\
      -I../../file/src -I../../../diversalis/src\
      $(shell pkg-config --cflags freetype2)
CFLAGS = -Wfatal-errors -Wparentheses
src = $(wildcard *.c)
srcimp = $(wildcard imps/x11/*.c)
#srcemptyimp = $(wildcard imps/empty/*.c)
#srcemptyimp = $(wildcard imps/text/*.c)
obj = $(src:.c=.o)
objimp = $(srcimp:.c=.o)
objall = $(obj) $(objimp)

all: CFLAGS += -O3 -DNDEBUG
all: libpsyui.a

debug: CFLAGS += -DDEBUG -g3
debug: libpsyui.a

libpsyui.a: $(obj)
	ar rsv $@ *.o
	
%.o: %.c
	$(CC) $(CFLAGS) $(inc) -c $(src)
	$(CC) $(CFLAGS) $(inc) -c $(srcimp)
#	$(CC) $(CFLAGS) $(inc) -c $(srcemptyimp)	

.PHONY: clean	
clean:
	rm -f $(obj) $(objimp) libpsyui.a	
