CFLAGS = -Wfatal-errors -Wparentheses -Wmissing-field-initializers
inc = -I../../luaui/src -I../../ui/src -I../../thread/src -I../../script/src -I../../dsp/src -I../../file/src -I../../container/src -I../../../diversalis/src $(shell pkg-config --cflags lua) -I/usr/include -I/usr/include/lilv-0/
src = $(wildcard *.c)
obj = $(src:.c=.o)

all: CFLAGS += -O3 -DNDEBUG
all: libaudio.a

debug: CFLAGS += -DDEBUG -g3
debug: libaudio.a

libaudio.a: $(obj)
	g++ $(CFLAGS) -c $(inc) plugin_interface.cpp
	ar rsv $@ $^ plugin_interface.o

%.o: %.c
	$(CC) $(CFLAGS) $(inc) -c $(src)

.PHONY: clean	
clean:
	rm -f $(obj) libaudio.a
