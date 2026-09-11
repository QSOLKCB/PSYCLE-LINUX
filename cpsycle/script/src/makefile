CC = gcc
inc = -I../../container/src -I../../../diversalis/src -I../../thread/src -I../../file/src $(shell pkg-config --cflags lua)
src = $(wildcard *.c)
obj = $(src:.c=.o)

all: libscript.a

debug: libscript.a

libscript.a: $(obj)
	ar rsv $@ $^

%.o: %.c
	$(CC) -O2 -g3 -Wparentheses $(inc) -c $(src)

.PHONY: clean	
clean:
	rm -f $(obj) libscript.a
