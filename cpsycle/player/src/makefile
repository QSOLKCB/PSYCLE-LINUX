CC = gcc
CFLAGS :=
UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
CFLAGS += -framework CoreFoundation
endif
INC = -I../../script/src -I../../thread/src -I../../container/src -I../../file/src -I../../diversalis/src -I../../src -I../../audio/src -I../../dsp/src $(shell pkg-config --cflags lua)
LIBDIRS = -L ../../thread/src -L ../../script/src -L ../../container/src -L ../../dsp/src -L ../../audio/src -L ../../file/src
LDFLAGS = -no-pie -Wparentheses -laudio -lthread  -llilv-0 -ldsp -laudio -lscript -lfile -lm $(shell pkg-config --libs lua) -lpthread -ldl -lstdc++ -lcontainer
psyplayer: coniotermios.c player.c 
	$(CC) $(CFLAGS) $(INC) -g coniotermios.c player.c -o psyplayer $(LIBDIRS) $(LDFLAGS)
	
.PHONY: clean	
clean:
	rm -f psyplayer.o psyplayer 
