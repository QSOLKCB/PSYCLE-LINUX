inc = -I../../thread/src
CFLAGS = -Wfatal-errors -Wparentheses -Wmissing-field-initializers $(inc)
src = $(wildcard *.c)
obj = $(src:.c=.o)

all: CFLAGS += -O3 -DNDEBUG
all: libcontainer.a

debug: CFLAGS += -DDEBUG -g3
debug: libcontainer.a

libcontainer.a: $(obj)	
	ar rsv $@ $^

%.o : %.c
	$(CC) -c $(CFLAGS) -fPIC $(src)

.PHONY: clean	
clean:
	rm -f $(obj) libcontainer.a
