PLUGDIRS :=\
			sdl2\
			alsa\
			alsamidi\
			jack\
			evjoystick
SUBDIRS := $(PLUGDIRS)
OUT_DIR := ./build
BUILD := 

MKDIR_P = mkdir -p

.PHONY: directories

all: directories $(SUBDIRS)	

debug: BUILD = debug
debug: directories $(SUBDIRS)	

$(SUBDIRS):
	$(MAKE) $(BUILD) -C $@

directories: ${OUT_DIR}

${OUT_DIR}:
	${MKDIR_P} ${OUT_DIR}

.PHONY: all $(SUBDIRS)

clean:
	rm -f $(addsuffix /*.o,$(PLUGDIRS))
	rm -f $(addsuffix /*.so,$(PLUGDIRS))
	rm -f build/*.so
