SUBDIRS := ../script/src ../thread/src ../container/src ../file/src ../ui/src ../luaui/src ../dsp/src ../audio/src ../host/src 
BUILD := 

all: $(SUBDIRS)

debug: BUILD = debug
debug: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) $(BUILD) -C $@

.PHONY: all clean $(SUBDIRS)
clean:	
	$(MAKE) clean -C ../script/src
	$(MAKE) clean -C ../thread/src
	$(MAKE) clean -C ../container/src
	$(MAKE) clean -C ../file/src
	$(MAKE) clean -C ../ui/src
	$(MAKE) clean -C ../luaui/src
	$(MAKE) clean -C ../dsp/src
	$(MAKE) clean -C ../audio/src	
	$(MAKE) clean -C ../host/src	
