/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "defaultlang.h"
#include <stdio.h>

/* implementation */
void make_translator_default(psy_Dictionary* rv)
{
	assert(rv);
	
	psy_dictionary_set(rv, "lang", "en");
	/* driver */	
	psy_dictionary_set(rv, "drv.version", "Version");
	psy_dictionary_set(rv, "drv.vendor", "Vendor");
	psy_dictionary_set(rv, "drv.name", "Name");
	psy_dictionary_set(rv, "drv.rate", "Samplerate");
	psy_dictionary_set(rv, "drv.latency", "Latency (ms)");
	psy_dictionary_set(rv, "drv.dither", "Dither");
	psy_dictionary_set(rv, "drv.bitdepth", "Bit Depth");
	psy_dictionary_set(rv, "drv.numperiods", "Number of Periods");
	psy_dictionary_set(rv, "drv.periodsize", "Period size");
	psy_dictionary_set(rv, "drv.indevice", "Input Device");
	psy_dictionary_set(rv, "drv.outdevice", "Output Device");
	psy_dictionary_set(rv, "drv.device", "Device");
	/* file */
	psy_dictionary_set(rv, "file.showall", "Show all");
	psy_dictionary_set(rv, "file.file", "File:");
	psy_dictionary_set(rv, "file.song", "Song");
	psy_dictionary_set(rv, "file.disk_op", "Disk op.");
	psy_dictionary_set(rv, "file.load", "Load");
	psy_dictionary_set(rv, "file.save", "Save");
	psy_dictionary_set(rv, "file.delete", "Delete");
	psy_dictionary_set(rv, "file.mkdir", "Make dir.");
	psy_dictionary_set(rv, "file.mklist", "Make list.");
	psy_dictionary_set(rv, "file.refresh", "Refresh");
	psy_dictionary_set(rv, "file.export", "Export");
	psy_dictionary_set(rv, "file.new", "New");
	psy_dictionary_set(rv, "file.render", "Render");
	psy_dictionary_set(rv, "file.loadsong", "Load Song");
	psy_dictionary_set(rv, "file.savesong", "Save Song");
	psy_dictionary_set(rv, "file.plugin-filesave", "Plugin File Load");
	psy_dictionary_set(rv, "file.plugin-fileload", "Plugin File Save");
	psy_dictionary_set(rv, "file.select-playlist", "Add song to playlist");
	psy_dictionary_set(rv, "file.exit", "Exit");
	/* undo */
	psy_dictionary_set(rv, "undo.undo", "Undo");
	psy_dictionary_set(rv, "undo.redo", "Redo");
	/* play */	
	psy_dictionary_set(rv, "play.loop", "Loop");
	psy_dictionary_set(rv, "play.record-notes", "Record Notes");
	psy_dictionary_set(rv, "play.play", "Play");
	psy_dictionary_set(rv, "play.stop", "Stop");
	psy_dictionary_set(rv, "play.song", "Song");
	psy_dictionary_set(rv, "play.sel", "Sel");
	psy_dictionary_set(rv, "play.beats", "Beats");
	psy_dictionary_set(rv, "play.files", "Files");
	/* metronome */	
	psy_dictionary_set(rv, "metronome.metronome", "Metronome");
	psy_dictionary_set(rv, "metronome.configure", "(Configure)");
	psy_dictionary_set(rv, "metronome.precount", "Precount");
	/* main */	
	psy_dictionary_set(rv, "main.machines", "Machines");	
	psy_dictionary_set(rv, "main.patterns", "Patterns");
	psy_dictionary_set(rv, "main.pattern", "Pattern");
	psy_dictionary_set(rv, "main.samples", "Samples");
	psy_dictionary_set(rv, "main.instrument", "Instrument");
	psy_dictionary_set(rv, "main.instruments", "Instruments");
	psy_dictionary_set(rv, "main.scripts", "...");
	psy_dictionary_set(rv, "main.help", "?");
	psy_dictionary_set(rv, "main.settings", "");
	psy_dictionary_set(rv, "main.properties", "Properties");
	psy_dictionary_set(rv, "main.exit", "Exit");
	psy_dictionary_set(rv, "main.kbd", "Kbd");
	psy_dictionary_set(rv, "main.terminal", "Terminal");
	psy_dictionary_set(rv, "main.floated", "This view is floated");
	/* help */	
	psy_dictionary_set(rv, "help.help", "Help");
	psy_dictionary_set(rv, "help.about", "About");
	psy_dictionary_set(rv, "help.greetings", "Greetings");
	psy_dictionary_set(rv, "help.licence", "Licence");
	psy_dictionary_set(rv, "help.contributors-credits", "Contributors / Credits");
	psy_dictionary_set(rv, "help.ok", "OK");
	psy_dictionary_set(rv, "help.extract", "Extract Help");
	psy_dictionary_set(rv, "help.combine", "Combine Help");
	/* greetings */	
	psy_dictionary_set(rv, "greetings.thanks", "Thanks! / ");
	psy_dictionary_set(rv, "greetings.wantstothank",
		"Psycledelics, the Community, wants to thank the following people "
		"for their contributions in the developement of Psycle");
	psy_dictionary_set(rv, "greetings.showargurus", "Show Original Arguru's Greetings");
	psy_dictionary_set(rv, "greetings.showcurrent", "Show Current Greetings");
	/* machinebar */
	psy_dictionary_set(rv, "machinebar.gear", "Gear Rack");
	psy_dictionary_set(rv, "machinebar.dock", "Param Rack");
	psy_dictionary_set(rv, "machinebar.editor", "Editor");
	psy_dictionary_set(rv, "machinebar.cpu", "CPU");
	psy_dictionary_set(rv, "machinebar.midi", "MIDI");
	psy_dictionary_set(rv, "machinebar.load", "Load");
	psy_dictionary_set(rv, "machinebar.waveeditor", "Wave Editor");
	/* trackbar */	
	psy_dictionary_set(rv, "trackbar.tracks", "Tracks");
	/* octavebar */	
	psy_dictionary_set(rv, "octavebar.octave", "Octave");
	/* edit */	
	psy_dictionary_set(rv, "edit.cut", "Cut");
	psy_dictionary_set(rv, "edit.copy", "Copy");
	psy_dictionary_set(rv, "edit.crop", "Crop");
	psy_dictionary_set(rv, "edit.delete", "Delete");
	psy_dictionary_set(rv, "edit.paste", "Paste");
	psy_dictionary_set(rv, "edit.mixpaste", "MixPaste");
	psy_dictionary_set(rv, "edit.clear", "Clear");
	psy_dictionary_set(rv, "edit.duplicate", "Duplicate");
	psy_dictionary_set(rv, "edit.clone", "Clone");
	psy_dictionary_set(rv, "edit.transpose", "Transport note");
	psy_dictionary_set(rv, "edit.transposeblockinc", "+1");
	psy_dictionary_set(rv, "edit.transposeblockdec", "-1");
	psy_dictionary_set(rv, "edit.transposeblockinc12", "+12");
	psy_dictionary_set(rv, "edit.transposeblockdec12", "-12");	
	psy_dictionary_set(rv, "edit.blockinterpolate", "Interpolate (Linear)");
	psy_dictionary_set(rv, "edit.blockinterpolatecurve", "Interpolate (Curve)");
	psy_dictionary_set(rv, "edit.blocksetinstr", "Change Instrument");
	psy_dictionary_set(rv, "edit.blocksetmachine", "Change Machine");
	psy_dictionary_set(rv, "edit.blockswingfill", "Swing Fill Block");
	psy_dictionary_set(rv, "edit.trackswingfill", "Swing Fill Track");
	psy_dictionary_set(rv, "edit.searchreplace", "Search and Replace");
	psy_dictionary_set(rv, "edit.importpsb", "Import (psb)");
	psy_dictionary_set(rv, "edit.exportpsb", "Export (psb)");
	psy_dictionary_set(rv, "edit.properties", "Pattern Properties");
	psy_dictionary_set(rv, "edit.tracknames", "Pattern Track names");
	/* timebar */
	psy_dictionary_set(rv, "timebar.tempo", "Tempo");
	/* lpb */	
	psy_dictionary_set(rv, "lpb.lines-per-beat", "Lines per beat");
	/* channelmapping */
	psy_dictionary_set(rv, "channelmapping.autowire", "Autowire");
	psy_dictionary_set(rv, "channelmapping.unselect-all", "Unselect all");
	psy_dictionary_set(rv, "channelmapping.remove-connection-with-right-click",
		"Remove connection with right click");
	/* render */
	psy_dictionary_set(rv, "render.render", "Render");
	psy_dictionary_set(rv, "render.configure", "Configure");
	psy_dictionary_set(rv, "render.file", "File");
	psy_dictionary_set(rv, "render.sequence-positions", "Sequence positions");
	psy_dictionary_set(rv, "render.quality", "Quality");
	psy_dictionary_set(rv, "render.dither", "Dither");
	psy_dictionary_set(rv, "render.selection", "Selection");
	psy_dictionary_set(rv, "render.record", "Record");
	psy_dictionary_set(rv, "render.enable", "enable");
	psy_dictionary_set(rv, "render.from", "from");
	psy_dictionary_set(rv, "render.to", "to");	
	psy_dictionary_set(rv, "render.save-wave", "Save Wave");
	psy_dictionary_set(rv, "render.entire-song", "The Entire Song");
	psy_dictionary_set(rv, "render.songsel", "The Song Selection");
	psy_dictionary_set(rv, "render.samplerate", "Samplerate");
	psy_dictionary_set(rv, "render.bitdepth", "Bit depth");
	psy_dictionary_set(rv, "render.none", "None");
	psy_dictionary_set(rv, "render.pdf", "Prob. Distribution");
	psy_dictionary_set(rv, "render.triangular", "Triangular");
	psy_dictionary_set(rv, "render.rectangular", "Rectangular");
	psy_dictionary_set(rv, "render.gaussian", "Gaussian");
	psy_dictionary_set(rv, "render.noise-shaping", "Noise-shaping");
	psy_dictionary_set(rv, "render.high-pass-contour", "High-Pass Contour");
	psy_dictionary_set(rv, "render.none", "none");
	psy_dictionary_set(rv, "render.output-path", "Output path");
	psy_dictionary_set(rv, "render.save-each-unmuted", "Save each unmuted");
	/* export */
	psy_dictionary_set(rv, "export.export-module", "Export Module");
	psy_dictionary_set(rv, "export.export-midifile", "Export Midi File");
	psy_dictionary_set(rv, "export.export-lyfile", "Export LilyPond File");
	/* gear */
	psy_dictionary_set(rv, "gear.clone", "Clone");
	psy_dictionary_set(rv, "gear.delete", "Delete");
	psy_dictionary_set(rv, "gear.properties", "Properties");
	psy_dictionary_set(rv, "gear.parameters", "Parameters");
	psy_dictionary_set(rv, "gear.exchange", "Exchange");
	psy_dictionary_set(rv, "gear.show-master", "Show Master");
	psy_dictionary_set(rv, "gear.connecttomaster", "Connect To Master");
	psy_dictionary_set(rv, "gear.create-replace", "Create/Replace");
	psy_dictionary_set(rv, "gear.mute-unmute", "Mute/Unmute");
	psy_dictionary_set(rv, "gear.effects", "Effects");
	psy_dictionary_set(rv, "gear.instruments", "Instruments");
	psy_dictionary_set(rv, "gear.generators", "Generators");
	psy_dictionary_set(rv, "gear.waves", "Waves");
	psy_dictionary_set(rv, "gear.labelgenerator", "Machines:Generator");
	psy_dictionary_set(rv, "gear.labeleffect", "Machines:Effect");
	psy_dictionary_set(rv, "gear.labelinstruments", "Instruments");
	psy_dictionary_set(rv, "gear.labelsamples", "Sampled sounds");
	/* cpu */	
	psy_dictionary_set(rv, "cpu.performance", "CPU Performance");
	psy_dictionary_set(rv, "cpu.resources", "Windows Resources");
	psy_dictionary_set(rv, "cpu.ram", "Physical Memory(RAM)");
	psy_dictionary_set(rv, "cpu.ram", "Memoria fisica");
	psy_dictionary_set(rv, "cpu.swap", "Page File (Swap)");
	psy_dictionary_set(rv, "cpu.vram", "Virtual Memory");
	psy_dictionary_set(rv, "cpu.audio-threads", "Audio threads");
	psy_dictionary_set(rv, "cpu.total", "Total (time)");
	psy_dictionary_set(rv, "cpu.machines", "Machines");
	psy_dictionary_set(rv, "cpu.routing", "Routing");	
	/* songproperties */
	psy_dictionary_set(rv, "songprop.title", "Song Title");
	psy_dictionary_set(rv, "songprop.credits", "Credits");
	psy_dictionary_set(rv, "songprop.speed", "Speed");
	psy_dictionary_set(rv, "songprop.tempo", "Tempo");
	psy_dictionary_set(rv, "songprop.lpb", "Lines Per Beat");
	psy_dictionary_set(rv, "songprop.tpb", "Ticks Per Beat");
	psy_dictionary_set(rv, "songprop.etpb", "Extra Ticks Per Beat");
	psy_dictionary_set(rv, "songprop.realtempo", "Real Tempo");
	psy_dictionary_set(rv, "songprop.realtpb", "Real Ticks Per Beat");
	psy_dictionary_set(rv, "songprop.extcomments", "Extended Comments");
	psy_dictionary_set(rv, "songprop.preventedit", "Prevent edit");
	/* settings */
	psy_dictionary_set(rv, "settings.event-input", "Event Input");
	psy_dictionary_set(rv, "settings.audio-drivers", "Audio Drivers");
	psy_dictionary_set(rv, "settings.configure", "Configure");	
	psy_dictionary_set(rv, "settings.compatibility", "Compatibility");
	psy_dictionary_set(rv, "settings.choose-font", "Choose Font");	
	psy_dictionary_set(rv, "settings.none", "None");
	psy_dictionary_set(rv, "settings.from", "from");
	psy_dictionary_set(rv, "settings.to", "to");
	/* settings.global */	
	psy_dictionary_set(rv, "settings.global.configuration", "Configuration");
	psy_dictionary_set(rv, "settings.global.enable-audio", "Enable Audio");
	psy_dictionary_set(rv, "settings.global.regenerate-plugincache", "Regenerate the plugin cache");		
	psy_dictionary_set(rv, "settings.global.language", "Language");
	psy_dictionary_set(rv, "settings.global.de", "german");
	psy_dictionary_set(rv, "settings.global.en", "english");
	psy_dictionary_set(rv, "settings.global.es", "spanish");
	psy_dictionary_set(rv, "settings.global.importconfig", "Import MFC Ini File");
	/* settings.midicontrollers	*/	
	psy_dictionary_set(rv, "settings.midicontrollers.controllers", "MIDI Controllers");
	psy_dictionary_set(rv, "settings.midicontrollers.add", "Add Controller Map");
	psy_dictionary_set(rv, "settings.midicontrollers.remove", "Remove Controller Map");
	psy_dictionary_set(rv, "settings.midicontrollers.select", "Select");
	psy_dictionary_set(rv, "settings.midicontrollers.mapping", "Midi controllers mapping");
	psy_dictionary_set(rv, "settings.midicontrollers.macselect", "Select Generator/Instrument with");
	psy_dictionary_set(rv, "settings.midicontrollers.auxselect", "Select Instrument/Aux with");
	psy_dictionary_set(rv, "settings.midicontrollers.select-inpsycle", "Currently selected in Psycle");
	psy_dictionary_set(rv, "settings.midicontrollers.select-bybank", "Selected by bank change");
	psy_dictionary_set(rv, "settings.midicontrollers.select-byprogram", "Selected by program change");
	psy_dictionary_set(rv, "settings.midicontrollers.select-bychannel", "Selected by MIDI channel index");
	psy_dictionary_set(rv, "settings.midicontrollers.recordrawmidiasmcm", "Record Raw MIDI as MCM");
	psy_dictionary_set(rv, "settings.midicontrollers.mapping-map-velocity", "Map Velocity");
	psy_dictionary_set(rv, "settings.midicontrollers.mapping-map-pitchwheel", "Map Pitch Wheel");
	psy_dictionary_set(rv, "settings.midicontrollers.mapping-map-controller", "Map Controller");
	psy_dictionary_set(rv, "settings.midicontrollers.mapping-map-active", "active");		
	/* settings.metronome */
	psy_dictionary_set(rv, "settings.metronome.metronome", "Metronome");
	psy_dictionary_set(rv, "settings.metronome.show", "Show Metronome Bar");
	psy_dictionary_set(rv, "settings.metronome.machine", "Machine");
	psy_dictionary_set(rv, "settings.metronome.note", "Note");
	/* settings.seqeditor */	
	psy_dictionary_set(rv, "settings.seqeditor.seqeditor", "Sequence Editor");	
	psy_dictionary_set(rv, "settings.seqeditor.machine", "New Song Default Sampler (For current change song properties)");
	/* settings.dirs */	
	psy_dictionary_set(rv, "settings.dirs.dirs", "Directories");
	psy_dictionary_set(rv, "settings.dirs.song", "Song directory");
	psy_dictionary_set(rv, "settings.dirs.samples", "Samples directory");
	psy_dictionary_set(rv, "settings.dirs.plugins", "Plugins");
	psy_dictionary_set(rv, "settings.dirs.plugin32", "Plug-in32 directory");
	psy_dictionary_set(rv, "settings.dirs.plugin64", "Plug-in64 directory");
	psy_dictionary_set(rv, "settings.dirs.lua", "Lua-scripts directory");
	psy_dictionary_set(rv, "settings.dirs.vst32", "Vst32 directories");
	psy_dictionary_set(rv, "settings.dirs.vst64", "Vst64 directories");
	psy_dictionary_set(rv, "settings.dirs.ladspa", "Ladspa directories");
	psy_dictionary_set(rv, "settings.dirs.lv2", "Lv2 directories");
	psy_dictionary_set(rv, "settings.dirs.skin", "Skin directory");
	psy_dictionary_set(rv, "settings.dirs.doc", "Documentation directory");
	/* settings.theme */	
	psy_dictionary_set(rv, "settings.theme.theme", "Theme");
	psy_dictionary_set(rv, "settings.theme.background", "Background");
	psy_dictionary_set(rv, "settings.theme.font", "Font");
	psy_dictionary_set(rv, "settings.theme.name", "Name");
	/* settings.general */	
	psy_dictionary_set(rv, "settings.general.general", "General");
	psy_dictionary_set(rv, "settings.general.version", "Version");
	psy_dictionary_set(rv, "settings.general.bench", "Workbench");
	psy_dictionary_set(rv, "settings.general.show-about-at-startup", "Show About At Startup");
	psy_dictionary_set(rv, "settings.general.show-song-info-on-load", "Show Song Info On Load");
	psy_dictionary_set(rv, "settings.general.show-maximized-at-startup", "Show maximized at startup");
	psy_dictionary_set(rv, "settings.general.show-playlist", "Show Playlist/Recentsongs");
	psy_dictionary_set(rv, "settings.general.show-pianokbd", "Show Pianokeyboard");
	psy_dictionary_set(rv, "settings.general.show-sequenceedit", "Show Sequenceeditor");
	psy_dictionary_set(rv, "settings.general.show-sequencestepbar", "Show Sequencestepbar");
	psy_dictionary_set(rv, "settings.general.show-plugineditor", "Show Gear");
	psy_dictionary_set(rv, "settings.general.show-paramrack", "Show Parameter Rack");
	psy_dictionary_set(rv, "settings.general.show-gear", "Show Gear");
	psy_dictionary_set(rv, "settings.general.show-midi", "Show Midi Monitor");
	psy_dictionary_set(rv, "settings.general.show-cpu", "Show CPU Monitor");
	psy_dictionary_set(rv, "settings.general.save-recent-songs", "Save recent songs");
	psy_dictionary_set(rv, "settings.general.play-song-after-load", "Play Song After Load");	
	psy_dictionary_set(rv, "settings.general.show-pattern-names", "Show Pattern Names");
	/* settings.visual */	
	psy_dictionary_set(rv, "settings.visual.visual", "Visual");
	psy_dictionary_set(rv, "settings.visual.load-skin", "Load skin");
	psy_dictionary_set(rv, "settings.visual.default-skin", "Default skin");
	psy_dictionary_set(rv, "settings.visual.default-font", "Default Font");
	psy_dictionary_set(rv, "settings.visual.zoom", "Zoom");
	psy_dictionary_set(rv, "settings.visual.default-line", "Default Line");
	psy_dictionary_set(rv, "settings.visual.load-dial-bitmap", "Load dial bitmap");
	psy_dictionary_set(rv, "settings.visual.apptheme", "App Theme");
	psy_dictionary_set(rv, "settings.visual.light", "light");
	psy_dictionary_set(rv, "settings.visual.dark", "dark");
	/* setting.pv */	
	psy_dictionary_set(rv, "settings.pv.patternview", "Pattern View");
	psy_dictionary_set(rv, "settings.pv.font", "Font");
	psy_dictionary_set(rv, "settings.pv.choose-font", "Choose Font");
	psy_dictionary_set(rv, "settings.pv.draw-empty-data", "Draw empty data");
	psy_dictionary_set(rv, "settings.pv.smoothscroll", "Scroll smooth");
	psy_dictionary_set(rv, "settings.pv.default-entries", "Default entries");
	psy_dictionary_set(rv, "settings.pv.displaysinglepattern", "Display Single Pattern");	
	psy_dictionary_set(rv, "settings.pv.line-numbers", "Line numbers");
	psy_dictionary_set(rv, "settings.pv.beat-offset", "Beat offset");
	psy_dictionary_set(rv, "settings.pv.line-numbers-cursor", "Line numbers cursor");
	psy_dictionary_set(rv, "settings.pv.line-numbers-in-hex", "Line numbers in HEX");
	psy_dictionary_set(rv, "settings.pv.wide-instrument-column", "Wide instrument column");
	psy_dictionary_set(rv, "settings.pv.pattern-track-scopes", "Pattern track scopes");
	psy_dictionary_set(rv, "settings.pv.wrap-around", "Wrap Around");
	psy_dictionary_set(rv, "settings.pv.center-cursor-on-screen", "Center cursor on screen");
	psy_dictionary_set(rv, "settings.pv.bar-highlighting", "Bar highlighting: (beats/bar)");
	psy_dictionary_set(rv, "settings.pv.a4-440hz", "A4 is 440Hz (Otherwise it is 220Hz)");
	psy_dictionary_set(rv, "settings.pv.patterndisplay", "Display pattern in");
	psy_dictionary_set(rv, "settings.pv.tracker", "Tracker");
	psy_dictionary_set(rv, "settings.pv.piano", "Pianoroll");
	psy_dictionary_set(rv, "settings.pv.splitvertical", "both side by side");
	psy_dictionary_set(rv, "settings.pv.splithorizontal", "both one below the other");
	psy_dictionary_set(rv, "settings.pv.move-cursor-when-paste", "Move Cursor When Paste");		
	/* settings.pv.theme */
	psy_dictionary_set(rv, "settings.pv.theme.theme", "Theme");
	psy_dictionary_set(rv, "settings.pv.theme.fontface", "Pattern Fontface");
	psy_dictionary_set(rv, "settings.pv.theme.fontpoint", "Pattern Fontpoint");
	psy_dictionary_set(rv, "settings.pv.theme.fontflags", "Pattern Fontflags");
	psy_dictionary_set(rv, "settings.pv.theme.font_x", "Point X");
	psy_dictionary_set(rv, "settings.pv.theme.font_y", "Point Y");
	psy_dictionary_set(rv, "settings.pv.theme.headerskin", "Pattern Header Skin");
	psy_dictionary_set(rv, "settings.pv.theme.separator", "Separator Left");
	psy_dictionary_set(rv, "settings.pv.theme.separator2", "Separator Right");
	psy_dictionary_set(rv, "settings.pv.theme.background", "BackGnd Left");
	psy_dictionary_set(rv, "settings.pv.theme.background2", "BackGnd Right");	
	psy_dictionary_set(rv, "settings.pv.theme.font", "Font Left");
	psy_dictionary_set(rv, "settings.pv.theme.font2", "Font Right");
	psy_dictionary_set(rv, "settings.pv.theme.fontcur", "Font Cur Left");
	psy_dictionary_set(rv, "settings.pv.theme.fontcur2", "Font Cur Right");
	psy_dictionary_set(rv, "settings.pv.theme.fontsel", "Font Sel Left");
	psy_dictionary_set(rv, "settings.pv.theme.fontsel2", "Font Sel Right");
	psy_dictionary_set(rv, "settings.pv.theme.fontplay", "Font Play Left");
	psy_dictionary_set(rv, "settings.pv.theme.fontplay2", "Font Play Right");
	psy_dictionary_set(rv, "settings.pv.theme.row", "Row Left");
	psy_dictionary_set(rv, "settings.pv.theme.row2", "Row Right");
	psy_dictionary_set(rv, "settings.pv.theme.rowbeat", "Beat Left");
	psy_dictionary_set(rv, "settings.pv.theme.rowbeat2", "Beat Right");
	psy_dictionary_set(rv, "settings.pv.theme.row4beat", "Bar Left");
	psy_dictionary_set(rv, "settings.pv.theme.row4beat2", "Bar Right");
	psy_dictionary_set(rv, "settings.pv.theme.selection", "Selection Left");
	psy_dictionary_set(rv, "settings.pv.theme.selection2", "Selection Right");
	psy_dictionary_set(rv, "settings.pv.theme.playbar", "Playbar Left");
	psy_dictionary_set(rv, "settings.pv.theme.playbar2", "Playbar Right");
	psy_dictionary_set(rv, "settings.pv.theme.cursor", "Cursor Left");
	psy_dictionary_set(rv, "settings.pv.theme.cursor2", "Cursor Right");
	psy_dictionary_set(rv, "settings.pv.theme.midline", "Midline Left");
	psy_dictionary_set(rv, "settings.pv.theme.midline2", "Midline Right");
	/* settings.mv */	
	psy_dictionary_set(rv, "settings.mv.machineview", "Machine View");
	psy_dictionary_set(rv, "settings.mv.draw-machine-indexes", "Draw Machine Indexes");
	psy_dictionary_set(rv, "settings.mv.draw-vumeters", "Draw VU Meters");
	psy_dictionary_set(rv, "settings.mv.draw-wirehover", "Draw Wire Hover");
	psy_dictionary_set(rv, "settings.mv.draw-virtualgenerators", "Draw Virtual Generators");
	psy_dictionary_set(rv, "settings.mv.stackview", "Stackview");
	psy_dictionary_set(rv, "settings.mv.stackview-draw-smalleffects", "Draw Small Effects");
	/* settings.mv.stackview */
	/* settings.mv.theme */
	psy_dictionary_set(rv, "settings.mv.theme.theme", "Theme");
	psy_dictionary_set(rv, "settings.mv.theme.vu-background", "Vu BackGround");
	psy_dictionary_set(rv, "settings.mv.theme.vu-bar", "Vu Bar");
	psy_dictionary_set(rv, "settings.mv.theme.onclip", "OnClip");
	psy_dictionary_set(rv, "settings.mv.theme.generators-font-face", "Generators Font Face");
	psy_dictionary_set(rv, "settings.mv.theme.generators-font-point", "Generators Font Point");
	psy_dictionary_set(rv, "settings.mv.theme.effect-fontface", "Effect Fontface");
	psy_dictionary_set(rv, "settings.mv.theme.effect-font-point", "Effect Font Point");
	psy_dictionary_set(rv, "settings.mv.theme.effect-font-flags", "Effect Font Flags");	
	psy_dictionary_set(rv, "settings.mv.theme.background", "Background");
	psy_dictionary_set(rv, "settings.mv.theme.wirecolour", "Wires Left");
	psy_dictionary_set(rv, "settings.mv.theme.wirecolour2", "Wires Right");
	psy_dictionary_set(rv, "settings.mv.theme.polygons", "Polygons");
	psy_dictionary_set(rv, "settings.mv.theme.generators-font", "Generators Font");
	psy_dictionary_set(rv, "settings.mv.theme.effects-font", "Effects Font");
	psy_dictionary_set(rv, "settings.mv.theme.wire-width", "Wire width");
	psy_dictionary_set(rv, "settings.mv.theme.antialias-halo", "AntiAlias halo");
	psy_dictionary_set(rv, "settings.mv.theme.machine-background", "Machine Background");
	psy_dictionary_set(rv, "settings.mv.theme.polygon-size", "Polygon size");
	psy_dictionary_set(rv, "settings.mv.theme.machineskin", "Machineskin");
	/* settings.paramview */	
	psy_dictionary_set(rv, "settings.param.native-machine-parameter-window",
		"Native Machine Parameter Window");
	psy_dictionary_set(rv, "settings.param.font", "Font");
	psy_dictionary_set(rv, "settings.param.load-dial-bitmap", "Load Dial Bitmap");
	psy_dictionary_set(rv, "settings.param.default-skin", "Default skin");
	/* settings.paramview.theme */	
	psy_dictionary_set(rv, "settings.param.theme.theme", "Theme");
	psy_dictionary_set(rv, "settings.param.theme.title-background", "Title Background");
	psy_dictionary_set(rv, "settings.param.theme.title-font", "Title Font");
	psy_dictionary_set(rv, "settings.param.theme.param-background", "Param Background");
	psy_dictionary_set(rv, "settings.param.theme.param-font", "Param Font");
	psy_dictionary_set(rv, "settings.param.theme.value-background", "Value Background");
	psy_dictionary_set(rv, "settings.param.theme.value-font", "Value Font");
	psy_dictionary_set(rv, "settings.param.theme.selparam-background", "SelParam Background");
	psy_dictionary_set(rv, "settings.param.theme.selparam-font", "SelParam Font");
	psy_dictionary_set(rv, "settings.param.theme.selvalue-background", "SelValue Background");
	psy_dictionary_set(rv, "settings.param.theme.selvalue-font", "SelValue Font");
	psy_dictionary_set(rv, "settings.param.theme.machinedialbmp", "Machine Dial Bitmap");
	/* settings.kbd */	
	psy_dictionary_set(rv, "settings.kbd.kbd-misc", "Keyboard and misc");
	psy_dictionary_set(rv, "settings.kbd.record-tws",
		"Record Mouse Tweaks as tws (Smooth tweaks)");
	psy_dictionary_set(rv, "settings.kbd.advance-line-on-record", "Advance Line On Record");
	psy_dictionary_set(rv, "settings.kbd.ctrl-play", "Right CTRL = play; Edit Toggle = stop");
	psy_dictionary_set(rv, "settings.kbd.ft2-home", "FT2 Style Home / End Behaviour");
	psy_dictionary_set(rv, "settings.kbd.ft2-delete", "FT2 Style Delete Behaviour");
	psy_dictionary_set(rv, "settings.kbd.cursoralwayssdown", "Cursor always moves down in Effect Column");
	psy_dictionary_set(rv, "settings.kbd.force-patstep1", "Force pattern step 1 when moving with cursors");
	psy_dictionary_set(rv, "settings.kbd.pgupdowntype", "Page Up / Page Down step by");
	psy_dictionary_set(rv, "settings.kbd.pgupdowntype", "Page Up / Page Down step by");
	psy_dictionary_set(rv, "settings.kbd.pgupdowntype-one-beat", "one beat");
	psy_dictionary_set(rv, "settings.kbd.pgupdowntype-one-bar", "one bar");
	psy_dictionary_set(rv, "settings.kbd.pgupdowntype-lines", "lines");
	psy_dictionary_set(rv, "settings.kbd.pgupdowntype-step-lines", "Page Up / Page Down step lines");
	psy_dictionary_set(rv, "settings.kbd.misc", "Miscellaneous options");
	psy_dictionary_set(rv, "settings.kbd.savereminder", "\"Save file?\" reminders on Load, New or Exit");
	psy_dictionary_set(rv, "settings.kbd.numdefaultlines", "Default lines on new pattern");
	psy_dictionary_set(rv, "settings.kbd.allowmultiinstances", "Allow multiple instances of Psycle");
	psy_dictionary_set(rv, "settings.kbd.ft2-explorer", "Use FT2 Style Fileexplorer");
	psy_dictionary_set(rv, "settings.kbd.followsong", "Follow Song");
	/* settings.io */	
	psy_dictionary_set(rv, "settings.io.input-output", "Input/Output");
	psy_dictionary_set(rv, "settings.io.audio-threads", "Audio Threads");
	psy_dictionary_set(rv, "settings.io.audio-threads-num",
		"Use the value 0 to autodetect your cpu threads");
	/* instrumentsbox */	
	psy_dictionary_set(rv, "instrumentsbox.instrument-groups", "Instrument Groups");
	psy_dictionary_set(rv, "instrumentsbox.group-instruments", "Group Instruments");
	/* instrumentview */
	psy_dictionary_set(rv, "instview.instrument", "Instrument");
	psy_dictionary_set(rv, "instview.empty", "No Instrument");
	psy_dictionary_set(rv, "instview.instrument-name", "Instrument Name");
	psy_dictionary_set(rv, "instview.new-note-action", "New Note Action");
	psy_dictionary_set(rv, "instview.same", "but if the same");
	psy_dictionary_set(rv, "instview.do", "do");
	psy_dictionary_set(rv, "instview.note-cut", "Note Cut");
	psy_dictionary_set(rv, "instview.note-release", "Note Release");
	psy_dictionary_set(rv, "instview.note-fadeout", "Note Fadeout");
	psy_dictionary_set(rv, "instview.none", "None");
	psy_dictionary_set(rv, "instview.notemap", "Notemap");
	psy_dictionary_set(rv, "instview.play-sample-to-fit", "Play sample to fit");
	psy_dictionary_set(rv, "instview.pattern-rows", "Pattern rows");
	psy_dictionary_set(rv, "instview.global-volume", "Global volume");
	psy_dictionary_set(rv, "instview.amplitude-envelope", "Amplitude envelope");
	psy_dictionary_set(rv, "instview.pan-envelope", "Pan envelope");
	psy_dictionary_set(rv, "instview.pitch-envelope", "Pitch envelope");
	psy_dictionary_set(rv, "instview.swing", "Swing (Randomize)");
	psy_dictionary_set(rv, "instview.fadeout", "Fadeout");
	psy_dictionary_set(rv, "instview.attack", "Attack");
	psy_dictionary_set(rv, "instview.decay", "Decay");
	psy_dictionary_set(rv, "instview.sustain-level", "Sustain level");
	psy_dictionary_set(rv, "instview.release", "Release");
	psy_dictionary_set(rv, "instview.filter-type", "Filter type");
	psy_dictionary_set(rv, "instview.filter-envelope", "Filter envelope");
	psy_dictionary_set(rv, "instview.cut-off", "Cut-off");
	psy_dictionary_set(rv, "instview.res", "Res/bandw.");
	psy_dictionary_set(rv, "instview.mod", "Mod. Amount");
	psy_dictionary_set(rv, "instview.random-panning", "Random panning");
	psy_dictionary_set(rv, "instview.general", "General");
	psy_dictionary_set(rv, "instview.volume", "Volume");
	psy_dictionary_set(rv, "instview.pan", "Pan");
	psy_dictionary_set(rv, "instview.filter", "Filter");
	psy_dictionary_set(rv, "instview.pitch", "Pitch");
	psy_dictionary_set(rv, "instview.add", "Add");
	psy_dictionary_set(rv, "instview.remove", "Remove");
	psy_dictionary_set(rv, "instview.smplidx", "Smpl Idx");
	psy_dictionary_set(rv, "instview.key", "Key");
	psy_dictionary_set(rv, "instview.keylo", "Key Lo");
	psy_dictionary_set(rv, "instview.keyhi", "Key Hi");
	psy_dictionary_set(rv, "instview.keyfixed", "Key Fixed");
	/* machineview */
	psy_dictionary_set(rv, "mv.new-machine", "New Machine");
	psy_dictionary_set(rv, "mv.wires", "Wires");
	psy_dictionary_set(rv, "mv.stack", "Stack");
	psy_dictionary_set(rv, "mv.no-machines-loaded", "No Machines Loaded");
	psy_dictionary_set(rv, "mv.no-machine", "No Machine");
	psy_dictionary_set(rv, "mv.connect-to-mixer-send-return-input",
		"Connect to Mixer-send/return-input");
	psy_dictionary_set(rv, "mv.delete", "Delete");
	psy_dictionary_set(rv, "mv.editname", "Editname");
	psy_dictionary_set(rv, "mv.mute", "Mute");
	psy_dictionary_set(rv, "mv.pwr", "Pwr");
	psy_dictionary_set(rv, "mv.master", "MASTER");
	/* machinemenu */
	psy_dictionary_set(rv, "mvmenu.parameters", "Open parameters");		
	psy_dictionary_set(rv, "mvmenu.bank", "Open bank manager");	
	psy_dictionary_set(rv, "mvmenu.connect", "Connect to");	
	psy_dictionary_set(rv, "mvmenu.connections", "Connections");	
	psy_dictionary_set(rv, "mvmenu.replace", "Replace machine");	
	psy_dictionary_set(rv, "mvmenu.clone", "Clone machine");	
	psy_dictionary_set(rv, "mvmenu.insertbefore", "Insert effect before");	
	psy_dictionary_set(rv, "mvmenu.insertafter", "Insert effect after");	
	psy_dictionary_set(rv, "mvmenu.delete", "Delete machine");	
	psy_dictionary_set(rv, "mvmenu.mute", "Mute");	
	psy_dictionary_set(rv, "mvmenu.solo", "Solo");	
	psy_dictionary_set(rv, "mvmenu.bypass", "Bypass");
	psy_dictionary_set(rv, "mvmenu.inputs", "Inputs");
	psy_dictionary_set(rv, "mvmenu.outputs", "Outputs");
	psy_dictionary_set(rv, "mvmenu.sure", "Are you sure?");
	psy_dictionary_set(rv, "mvmenu.yes", "Yes");
	psy_dictionary_set(rv, "mvmenu.no", "No");
	psy_dictionary_set(rv, "mvmenu.noconnections", "No Connections");
	psy_dictionary_set(rv, "mvmenu.nodestinations", "No destinations");
	/* stackview */
	psy_dictionary_set(rv, "stackview.inputs", "Inputs");
	psy_dictionary_set(rv, "stackview.effects", "Effects");
	psy_dictionary_set(rv, "stackview.outputs", "Outputs");
	psy_dictionary_set(rv, "stackview.volumes", "Volumes");
	/* machineframe */
	psy_dictionary_set(rv, "machineframe.about", "About");
	psy_dictionary_set(rv, "machineframe.pwr", "Pwr");
	psy_dictionary_set(rv, "machineframe.parameters", "Parameters");
	psy_dictionary_set(rv, "machineframe.parammap", "Parammap");
	psy_dictionary_set(rv, "machineframe.command", "Command");
	psy_dictionary_set(rv, "machineframe.help", "Help");
	psy_dictionary_set(rv, "machineframe.bus", "Bus");
	psy_dictionary_set(rv, "machineframe.bank", "Bank");
	psy_dictionary_set(rv, "machineframe.program", "Program");
	psy_dictionary_set(rv, "machineframe.import", "Import");
	psy_dictionary_set(rv, "machineframe.export", "Export");
	psy_dictionary_set(rv, "machineframe.saveas", "Save as");
	psy_dictionary_set(rv, "machineframe.delete", "Delete");
	psy_dictionary_set(rv, "machineframe.preview", "Preview");
	psy_dictionary_set(rv, "machineframe.use", "Use");
	psy_dictionary_set(rv, "machineframe.close", "Close");
	/* patternview */
	psy_dictionary_set(rv, "pv.line", "Line");
	psy_dictionary_set(rv, "pv.defaults", "Defaults");
	psy_dictionary_set(rv, "pv.step", "Step");
	psy_dictionary_set(rv, "pv.patname", "Pattern Name");
	psy_dictionary_set(rv, "pv.nopattern", "No Pattern");
	psy_dictionary_set(rv, "pv.length", "Length");
	psy_dictionary_set(rv, "pv.apply", "Apply");
	psy_dictionary_set(rv, "pv.tracker", "Tracker");
	psy_dictionary_set(rv, "pv.roll", "Pianoroll");
	psy_dictionary_set(rv, "pv.horz", "H");
	psy_dictionary_set(rv, "pv.vert", "V");
	psy_dictionary_set(rv, "pv.split", "|");	
	psy_dictionary_set(rv, "pv.beats", "Beats");
	psy_dictionary_set(rv, "pv.keyboard", "Keyboard");
	psy_dictionary_set(rv, "pv.showtracks", "Show Tracks");
	psy_dictionary_set(rv, "pv.all", "All");
	psy_dictionary_set(rv, "pv.current", "Current");
	psy_dictionary_set(rv, "pv.active", "Active");	
	psy_dictionary_set(rv, "pv.tracknames", "Track names");
	psy_dictionary_set(rv, "pv.tracknames_use_same",
		"All patterns use the same");
	psy_dictionary_set(rv, "pv.tracknames_use_individual",
		"Each pattern has its own");	
	psy_dictionary_set(rv, "pv.headerlabel", "Show on pattern headers");
	psy_dictionary_set(rv, "pv.headermodeclassic", "Header");
	psy_dictionary_set(rv, "pv.headermodetext", "Track names");
	psy_dictionary_set(rv, "pv.headerhint", "This settings can also be changed by clicking on the line text in the top left of the pattern editor");	
	/* transformpattern */
	psy_dictionary_set(rv, "transformpattern.searchpattern", "Search pattern");
	psy_dictionary_set(rv, "transformpattern.note", "Note");
	psy_dictionary_set(rv, "transformpattern.instr", "Instrum/Aux");
	psy_dictionary_set(rv, "transformpattern.mac", "Machine");
	psy_dictionary_set(rv, "transformpattern.replacepattern", "Replace pattern");
	psy_dictionary_set(rv, "transformpattern.search", "Search");	
	psy_dictionary_set(rv, "transformpattern.replaceall", "Replace all");
	psy_dictionary_set(rv, "transformpattern.searchon", "Search on");
	psy_dictionary_set(rv, "transformpattern.entiresong", "Entire song");
	psy_dictionary_set(rv, "transformpattern.currentpattern", "Current pattern");
	psy_dictionary_set(rv, "transformpattern.currentselection", "Current selection");
	/* swingfill */
	psy_dictionary_set(rv, "swingfill.tempo", "Tempo(BPM)");
	psy_dictionary_set(rv, "swingfill.bpm", "BPM");
	psy_dictionary_set(rv, "swingfill.center", "Center");
	psy_dictionary_set(rv, "swingfill.actual", "Actual");
	psy_dictionary_set(rv, "swingfill.cycle", "Cycle Length(lines)");
	psy_dictionary_set(rv, "swingfill.variance", "Variance(%)");
	psy_dictionary_set(rv, "swingfill.phase", "Phase (degrees)");
	psy_dictionary_set(rv, "swingfill.apply", "Apply");
	/* seqview */
	psy_dictionary_set(rv, "seqview.follow-song", "Follow Song");
	psy_dictionary_set(rv, "seqview.show-playlist", "Show Playlist");
	psy_dictionary_set(rv, "seqview.show-pattern-names", "Show Pattern Names");
	psy_dictionary_set(rv, "seqview.record-noteoff", "Record note-off");
	psy_dictionary_set(rv, "seqview.record-tweak", "Record tweak");
	psy_dictionary_set(rv, "seqview.allow-notes-to_effect", "Allow notes to effect");
	psy_dictionary_set(rv, "seqview.multichannel-audition", "Multitrack playback on keypress");
	psy_dictionary_set(rv, "seqview.duration", "Duration");
	psy_dictionary_set(rv, "seqview.more", "Show More");
	psy_dictionary_set(rv, "seqview.less", "Show Less");
	psy_dictionary_set(rv, "seqview.ins", "Ins");
	psy_dictionary_set(rv, "seqview.new", "New");
	psy_dictionary_set(rv, "seqview.clone", "Clone");
	psy_dictionary_set(rv, "seqview.del", "Delete");
	psy_dictionary_set(rv, "seqview.new-trk", "+ New Track");
	psy_dictionary_set(rv, "seqview.del-trk", "Del Track");
	psy_dictionary_set(rv, "seqview.clear", "Clear");
	psy_dictionary_set(rv, "seqview.rename", "Rename");
	psy_dictionary_set(rv, "seqview.copy", "Copy");
	psy_dictionary_set(rv, "seqview.paste", "Paste");
	psy_dictionary_set(rv, "seqview.singlesel", "SingleSel");
	psy_dictionary_set(rv, "seqview.multisel", "MultiSel");
	psy_dictionary_set(rv, "seqview.showpianokbd", "Pianokeyboard");
	psy_dictionary_set(rv, "seqview.hidepianokbd", "Pianokeyboard");
	psy_dictionary_set(rv, "seqview.showseqeditor", "SequenceEditor");
	psy_dictionary_set(rv, "seqview.hideseqeditor", "SequenceEditor");
	psy_dictionary_set(rv, "seqview.showstepsequencer", "Stepsequencer");
	psy_dictionary_set(rv, "seqview.hidestepsequencer", "Stepsequencer");
	/* seqedit */
	psy_dictionary_set(rv, "seqedit.editor", "Sequence Editor");
	psy_dictionary_set(rv, "seqedit.track", "Track");
	psy_dictionary_set(rv, "seqedit.move", "Move");
	psy_dictionary_set(rv, "seqedit.reorder", "Reorder");
	psy_dictionary_set(rv, "seqedit.insert", "Insert");
	psy_dictionary_set(rv, "seqedit.timesigs", "Timesigs");
	psy_dictionary_set(rv, "seqedit.repetitions", "Repetitions");
	psy_dictionary_set(rv, "seqedit.noitem", "No Item Selected");
	psy_dictionary_set(rv, "seqedit.patternitem", "Pattern Item");
	psy_dictionary_set(rv, "seqedit.sampleitem", "Sample Item");
	psy_dictionary_set(rv, "seqedit.markeritem", "Marker Item");
	psy_dictionary_set(rv, "seqedit.timesigitem", "TimeSig Item");
	psy_dictionary_set(rv, "seqedit.loopitem", "Loop Item");
	psy_dictionary_set(rv, "seqedit.position", "Position");
	psy_dictionary_set(rv, "seqedit.length", "Length");
	psy_dictionary_set(rv, "seqedit.end", "End");
	psy_dictionary_set(rv, "seqedit.name", "Name");
	psy_dictionary_set(rv, "seqedit.numerator", "Numerator");
	psy_dictionary_set(rv, "seqedit.denominator", "Denominator");
	/* newmachine */
	psy_dictionary_set(rv, "newmachine.all", "All");
	psy_dictionary_set(rv, "newmachine.favorites", "Favorites");
	psy_dictionary_set(rv, "newmachine.jme-version-unknown",
		"Load new gamefx and blitz if version is unknown");
	psy_dictionary_set(rv, "newmachine.song-loading-compatibility", "Song loading compatibility");
	psy_dictionary_set(rv, "newmachine.rescan", "Rescan");
	psy_dictionary_set(rv, "newmachine.in", "in");
	psy_dictionary_set(rv, "newmachine.plugin-directories", "Plugin directories");
	psy_dictionary_set(rv, "newmachine.filters", "FILTERS");
	psy_dictionary_set(rv, "newmachine.categories", "Categories");
	psy_dictionary_set(rv, "newmachine.sort", "Sort by");
	psy_dictionary_set(rv, "newmachine.favorite", "Favorite");
	psy_dictionary_set(rv, "newmachine.favorites", "Favorites");
	psy_dictionary_set(rv, "newmachine.name", "Name");
	psy_dictionary_set(rv, "newmachine.type", "Type");
	psy_dictionary_set(rv, "newmachine.mode", "Role");
	psy_dictionary_set(rv, "newmachine.sections", "Sections");
	psy_dictionary_set(rv, "newmachine.section", "Section");
	psy_dictionary_set(rv, "newmachine.mksection", "Mksection");
	psy_dictionary_set(rv, "newmachine.remove", "Remove");
	psy_dictionary_set(rv, "newmachine.clear", "Clear");
	psy_dictionary_set(rv, "newmachine.copy", "Copy");
	psy_dictionary_set(rv, "newmachine.delete", "Delete");
	psy_dictionary_set(rv, "newmachine.select-plugin-to-view-description",
		"Select a plugin to view its description");
	psy_dictionary_set(rv, "newmachine.select-first-section", "Select/Create first a section");
	psy_dictionary_set(rv, "newmachine.select-first-plugin", "Select first a plugin");
	psy_dictionary_set(rv, "newmachine.search-plugin", "Search Plugin");
	psy_dictionary_set(rv, "newmachine.category", "Category");
	psy_dictionary_set(rv, "newmachine.apiversion", "API Version");
	psy_dictionary_set(rv, "newmachine.version", "Version");
	psy_dictionary_set(rv, "newmachine.module", "Module");
	psy_dictionary_set(rv, "newmachine.anycategory", "Any Category");
	psy_dictionary_set(rv, "newmachine.scanning", "Scanning");
	psy_dictionary_set(rv, "newmachine.stop", "Stop");
	psy_dictionary_set(rv, "newmachine.stopping", "Stopping");
	/* samplesview */	
	psy_dictionary_set(rv, "samplesview.samplename", "Samplename");
	psy_dictionary_set(rv, "samplesview.samplerate", "Samplerate");
	psy_dictionary_set(rv, "samplesview.samples", "Samples");
	psy_dictionary_set(rv, "samplesview.groupsamples", "Group Samples");
	psy_dictionary_set(rv, "samplesview.groupsfirstsample", "Groups first sample");
	psy_dictionary_set(rv, "samplesview.default-volume", "Default volume");
	psy_dictionary_set(rv, "samplesview.global-volume", "Global volume");
	psy_dictionary_set(rv, "samplesview.pan-position", "Pan Position");
	psy_dictionary_set(rv, "samplesview.sampled-note", "Sampled note");
	psy_dictionary_set(rv, "samplesview.pitch-finetune", "Pitch finetune");
	psy_dictionary_set(rv, "samplesview.process", "Process");
	psy_dictionary_set(rv, "samplesview.no-wave-loaded", "No wave loaded");
	psy_dictionary_set(rv, "samplesview.cont-loop", "Continuous Loop");
	psy_dictionary_set(rv, "samplesview.disabled", "Disabled");
	psy_dictionary_set(rv, "samplesview.forward", "Forward");
	psy_dictionary_set(rv, "samplesview.bidirection", "Bidirection");
	psy_dictionary_set(rv, "samplesview.start", "Start");
	psy_dictionary_set(rv, "samplesview.end", "End");
	psy_dictionary_set(rv, "samplesview.sustain-loop", "Sustain Loop");
	psy_dictionary_set(rv, "samplesview.play", "Play");
	psy_dictionary_set(rv, "samplesview.stop", "Stop");
	psy_dictionary_set(rv, "samplesview.select-together", "Select Channels Together");
	psy_dictionary_set(rv, "samplesview.selstart", "Selection Start");
	psy_dictionary_set(rv, "samplesview.selend", "Selection End");
	psy_dictionary_set(rv, "samplesview.visual", "Visual");
	psy_dictionary_set(rv, "samplesview.doublecont", "Double Cont Loop");
	psy_dictionary_set(rv, "samplesview.doublesus", "Double Sus Loop");
	psy_dictionary_set(rv, "samplesview.lines", "Lines");
	psy_dictionary_set(rv, "samplesview.source", "Source");
	psy_dictionary_set(rv, "samplesview.nosongloaded", "No Song Loaded");
	psy_dictionary_set(rv, "samplesview.loadsong", "Load Song");
	psy_dictionary_set(rv, "samplesview.songselect", "Select a song");
	psy_dictionary_set(rv, "samplesview.copy", "Copy");
	/* cmds */	
	psy_dictionary_set(rv, "cmds.keymap", "Keymap");	
	psy_dictionary_set(rv, "cmds.help", "Help");
	psy_dictionary_set(rv, "cmds.helpshortcut", "Kbd Help");
	psy_dictionary_set(rv, "cmds.editmachine", "Machines");
	psy_dictionary_set(rv, "cmds.editpattern", "Patterns");
	psy_dictionary_set(rv, "cmds.addmachine", "Add Machine");
	psy_dictionary_set(rv, "cmds.playsong", "Play Song");
	psy_dictionary_set(rv, "cmds.playstart", "Play Start");
	psy_dictionary_set(rv, "cmds.playfrompos", "Play Pos");
	psy_dictionary_set(rv, "cmds.playstop", "stop");
	psy_dictionary_set(rv, "cmds.songposdec", "Seqpos dec");
	psy_dictionary_set(rv, "cmds.songposinc", "Seqpos Inc");
	psy_dictionary_set(rv, "cmds.maxpattern", "Max Pattern");
	psy_dictionary_set(rv, "cmds.infomachine", "gear");
	psy_dictionary_set(rv, "cmds.editinstr", "Instruments");
	psy_dictionary_set(rv, "cmds.editsample", "Samples");
	psy_dictionary_set(rv, "cmds.editwave", "Wave Edit");
	psy_dictionary_set(rv, "cmds.terminal", "Terminal");
	psy_dictionary_set(rv, "cmds.instrdec", "Current Instrument -1");
	psy_dictionary_set(rv, "cmds.instrinc", "Current Instrument +1");

	psy_dictionary_set(rv, "cmds.navup", "Nav Up");
	psy_dictionary_set(rv, "cmds.navdown", "Nav Down");
	psy_dictionary_set(rv, "cmds.navleft", "Nav Left");
	psy_dictionary_set(rv, "cmds.navright", "Nav Right");
	psy_dictionary_set(rv, "cmds.navpageup", "Nav Up 16");
	psy_dictionary_set(rv, "cmds.navpagedown", "Nav Down 16");
	psy_dictionary_set(rv, "cmds.navtop", "Nav Top");
	psy_dictionary_set(rv, "cmds.navbottom", "Nav Bottom");
	psy_dictionary_set(rv, "cmds.columnprev", "Prev column");
	psy_dictionary_set(rv, "cmds.columnnext", "Next column");

	psy_dictionary_set(rv, "cmds.rowinsert", "Insert row");
	psy_dictionary_set(rv, "cmds.rowdelete", "Delete row");
	psy_dictionary_set(rv, "cmds.rowclear", "Clear row");

	psy_dictionary_set(rv, "cmds.blockstart", "Block Start");
	psy_dictionary_set(rv, "cmds.blockend", "Block End");
	psy_dictionary_set(rv, "cmds.blockunmark", "Block Unmark");
	psy_dictionary_set(rv, "cmds.blockcut", "Block cut");
	psy_dictionary_set(rv, "cmds.blockcopy", "Block copy");
	psy_dictionary_set(rv, "cmds.blockpaste", "Block paste");
	psy_dictionary_set(rv, "cmds.blockmix", "Block Mix");

	psy_dictionary_set(rv, "cmds.transposeblockinc", "Transpose Block +1");
	psy_dictionary_set(rv, "cmds.transposeblockdec", "Transpose Block -1");
	psy_dictionary_set(rv, "cmds.transposeblockinc12", "Transpose Block +12");
	psy_dictionary_set(rv, "cmds.transposeblockdec12", "Transpose Block -12");

	psy_dictionary_set(rv, "cmds.selectall", "Block Select All");
	psy_dictionary_set(rv, "cmds.selectcol", "Block Select Column");
	psy_dictionary_set(rv, "cmds.selectbar", "Block Select Bar");

	psy_dictionary_set(rv, "cmds.selectmachine",
		"Select Mac/Ins in Cursor Pos");
	psy_dictionary_set(rv, "undo", "Edit Undo");
	psy_dictionary_set(rv, "redo", "Edit Redo");
	psy_dictionary_set(rv, "follow_song", "Follow Song");
	/* msg */
	psy_dictionary_set(rv, "msg.psyreq", "Psycle Request");
	psy_dictionary_set(rv, "msg.seqclear", "Sequence Clear Request! Do you really want clear the sequenceand pattern data?");
	psy_dictionary_set(rv, "msg.loadsample", "Load sample request, but a sample already exists in this slot. If you continue, it will be ovewritten!");
	psy_dictionary_set(rv, "msg.yes", "Yes");
	psy_dictionary_set(rv, "msg.no", "No");
	psy_dictionary_set(rv, "msg.cont", "Continue");
	psy_dictionary_set(rv, "msg.cancel", "cancel");
	psy_dictionary_set(rv, "msg.psyexit", "Exit Psycle Request, but your Song is not saved!");
	psy_dictionary_set(rv, "msg.saveexit", "Save and Exit");
	psy_dictionary_set(rv, "msg.nosaveexit", "Exit (no save)");
	psy_dictionary_set(rv, "msg.newsong", "New Song Request, but your Song is not saved!");
	psy_dictionary_set(rv, "msg.savenew", "Save and Create New Song");
	psy_dictionary_set(rv, "msg.nosavenew", "Create New Song (no save)");
	psy_dictionary_set(rv, "msg.loadsong", "New Load Request, but your Song is not saved!");
	psy_dictionary_set(rv, "msg.saveload", "Save and Load Song");
	psy_dictionary_set(rv, "msg.nosaveload", "Load Song (no save)");
	psy_dictionary_set(rv, "msg.audiostarting", "Audio starting");
	psy_dictionary_set(rv, "msg.audiostarted", "Audio started");
}

#ifdef PSYCLE_DEFAULT_LANG
static int on_enum_properties(void* context, psy_Dictionary* property, uintptr_t level);

void save_translator_default(void)
{
	psy_Dictionary* lang;
	char path[4096];

	strcpy(path, psy_dir_config());	
	strcat(path, "\\en.ini");

	lang = psy_property_allocinit_key(NULL);
	psy_property_set_comment(lang,
		"Psycle English Dictionary created by\r\n; " PSYCLE__BUILD__IDENTIFIER("\r\n; "));
	make_translator_default(lang);
	propertiesio_save(lang, path);
	psy_property_deallocate(lang);
}

void save_translator_template(void)
{
	psy_Dictionary* lang;
	char path[4096];

	strcpy(path, psy_dir_config());
	strcat(path, "\\lang.ini");

	lang = psy_property_allocinit_key(NULL);
	psy_property_set_comment(lang,
		"Psycle Template Dictionary created by\r\n; " PSYCLE__BUILD__IDENTIFIER("\r\n; "));
	make_translator_default(lang);
	psy_property_enumerate((psy_Dictionary*)lang, NULL,
		(psy_PropertyCallback)on_enum_properties);
	propertiesio_save(lang, path);
	psy_property_deallocate(lang);
}

int on_enum_properties(void* context, psy_Dictionary* property, uintptr_t level)
{
	if (psy_property_type(property) == PSY_PROPERTY_TYPE_STRING) {
		psy_property_set_item_str(property, "");
	}
	return 1;
}

#endif
