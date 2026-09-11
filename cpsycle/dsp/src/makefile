CFLAGS = -Wfatal-errors -Wparentheses -Wmissing-field-initializers
inc = -I../../container/src -I../../../diversalis/src
src = $(wildcard *.c)
obj = $(src:.c=.o)

all: CFLAGS += -O3 -DNDEBUG
all: libdsp.a

debug: CFLAGS += -DDEBUG -g3
debug: libdsp.a

libdsp.a: $(obj)
	ar rsv $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(inc) -fPIC -c $(src) 

.PHONY: clean	
clean:
	rm -f $(obj) libdsp.a
