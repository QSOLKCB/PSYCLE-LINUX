CFLAGS = -Wfatal-errors -Wparentheses -Wmissing-field-initializers
INC = -I../../script/src -I../../thread/src -I../../luaui/src -I../../ui/src\
      -I../../container/src -I../../file/src -I../../diversalis/src\
      -I../../src -I../../audio/src -I../../dsp/src\
      $(shell pkg-config --libs --cflags lua)
LIBDIRS = -L ../../thread/src -L ../../script/src -L ../../container/src\
          -L ../../dsp/src -L ../../audio/src -L ../../file/src -L ../../src\
          -L ../../ui/src -L ../../luaui/src
src = $(wildcard *.c)
obj = $(src:.c=.o)

# if using lv2 add (for debian) -llilv-0 (RHEL?)
LDFLAGS = -no-pie -Wparentheses -laudio -lthread -ldsp -laudio -lscript -lfile\
          -lpsyui -lpsyluaui -lfontconfig -lfreetype -lGL -lX11 -lXft -lXext -lXmu\
          -lm $(shell pkg-config --libs --define-prefix lua) -lpthread -ldl\
          -lstdc++ -lcontainer


all: CFLAGS += -O3 -DNDEBUG
all: psycle

debug: CFLAGS += -DDEBUG -g3
debug: psycle

psycle: $(obj)	
	$(CC) $(CFLAGS) -o $@ $^ $(LIBDIRS) $(LDFLAGS)
		
%.o: %.c
	$(CC) $(CFLAGS) $(INC) -c $(src)
	

.PHONY: clean	
clean:
	rm -f $(obj) psycle	
