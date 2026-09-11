HOSTDIR := host
PLUGDIR := plugins
DRIVERDIR := driver
PLAYERDIR := player
BUILD := 

all: host plugins drivers player

debug: BUILD = debug
debug: host plugins drivers

host-debug: BUILD = debug
host-debug: host drivers

host:
	$(MAKE) $(BUILD) -C $(HOSTDIR)
	cp ./$(HOSTDIR)/src/psycle ./psycle

plugins-debug: BUILD = debug
plugins-debug: plugins

plugins:
	$(MAKE) -k $(BUILD) -C $(PLUGDIR)
	
drivers:
	$(MAKE) -k $(BUILD) -C $(DRIVERDIR)
	
player-debug: BUILD = debug
player-debug: player

player:
	$(MAKE) $(BUILD) -C $(PLAYERDIR)
	cp ./$(PLAYERDIR)/src/psyplayer ./psyplayer

.PHONY: all host plugins drivers player clean clean-host clean-plugins clean-drivers

clean:
	$(MAKE) -C $(HOSTDIR) clean
	$(MAKE) -C $(PLUGDIR) clean
	$(MAKE) -C $(PLAYERDIR) clean
	rm -f psycle
	rm -f psyplayer

clean-host:
	$(MAKE) -C $(HOSTDIR) clean
	rm -f psycle

clean-plugins:
	$(MAKE) -C $(PLUGDIR) clean
	
clean-player:
	$(MAKE) -C $(PLAYERDIR) clean
	rm -f psyplayer

clean-drivers:
	$(MAKE) -C $(DRIVERDIR) clean
	rm -f psyplayer
