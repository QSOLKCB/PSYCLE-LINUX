inc = -I../../container/src -I../../../diversalis/src
src = $(wildcard *.c)
obj = $(src:.c=.o)

all: libfile.a

debug: libfile.a

libfile.a: $(obj)
	ar rsv $@ $^

%.o: %.c
	$(CC) -O2 -g3 $(inc) -Wparentheses -c $(src)

.PHONY: clean	
clean:
	rm -f $(obj) libfile.a
