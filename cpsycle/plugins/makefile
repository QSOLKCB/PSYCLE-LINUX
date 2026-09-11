HELPERDIRS := ../container/src ../dsp/src
PLUGDIRS :=\
			alk_muter/src\
			arguru-compressor/src\
			arguru-distortion/src\
			arguru-goaslicer/src\
			arguru-reverb/src\
			arguru-synth-2f/src\
			arguru-xfilter/src\
			bexphase/src\
			compressor/src\
			crasher/src\
			delay/src\
			distortion/src\
			druttis_eq3/src\
			druttis_feedme/src\
			druttis_koruz/src\
			druttis_phantom/src\
			druttis_slicit/src\
			druttis_pluckedstring/src\
			dw_eq/src\
			dw_granulizer/src\
			dw_iopan/src\
			dw_tremolo/src\
			gainer/src\
			gverb/src\
			graue_softsat/src\
			haas/src\
			jm_drums/src\
			jme_blitz12/src\
			jme_blitzn/src\
			jme_gamefx13/src\
			jme_gamefxn/src\
			legasynth/src\
			moreamp_eq/src\
			negative/src\
			ninereeds_7900/src\
			phaser/src\
			pooplog_autopan/src\
			pooplog_delay/src\
			pooplog_delay_light/src\
			pooplog_filter/src\
			pooplog-synth/src\
			pooplog-synth-light/src\
			pooplog-synth-ultralight/src\
			schorus/src\
			stk-plugged/src\
			stk-reverbs/src\
			stk-shakers/src\
			surround/src\
			ring_modulator/src\
			wahwah/src\
			yezar_freeverb/src\
			zephod_super_fm/src\
			flanger/src\
			filter_2_poles/src\
			druttis_sublime/src\
			fluid_synth_sf2_player/src
SUBDIRS := $(HELPERDIRS) $(PLUGDIRS)
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