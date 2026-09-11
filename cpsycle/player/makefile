SUBDIRS := ../script/src ../thread/src ../container/src ../file/src ../dsp/src ../audio/src ../player/src

all: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@	

.PHONY: all $(SUBDIRS)

.PHONY: all clean $(SUBDIRS)
clean:	
	$(MAKE) clean -C ../script/src
	$(MAKE) clean -C ../thread/src
	$(MAKE) clean -C ../container/src
	$(MAKE) clean -C ../file/src	
	$(MAKE) clean -C ../dsp/src
	$(MAKE) clean -C ../audio/src	
	$(MAKE) clean -C ../player/src	
