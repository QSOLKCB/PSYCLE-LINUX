CC = gcc -Wfatal-errors
inc = $(shell pkg-config --cflags lua) -I../../luaui/src -I../../ui/src -I../../thread/src -I../../script/src -I../../dsp/src -I../../file/src -I../../container/src -I../../../diversalis/src
src = $(wildcard *.c)
obj = $(src:.c=.o)

all: libpsyluaui.a

debug: libpsyluaui.a

libpsyluaui.a: $(obj)	
	ar rsv $@ $^

%.o : %.c
	$(CC) -O2 -g3 $(inc) -c $(src)

.PHONY: clean	
clean:
	rm -f $(obj) libpsyluaui.a
