/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(MAINFRAME_H)
#define MAINFRAME_H

/* host */
#include "closebar.h"
#include "cpuview.h"
#include "filebar.h"
#include "fileview.h"
#include "gear.h"
#include "interpreter.h"
#include "kbdhelp.h"
#include "keyboardview.h"
#include "machinebar.h"
#include "mainviews.h"
#include "mainstatusbar.h"
#include "messageview.h"
#include "metronomebar.h"
#include "midimonitor.h"
#include "paramgear.h"
#include "playbar.h"
#include "playposbar.h"
#include "plugineditor.h"
#include "psycmdline.h"
#include "sequencerbar.h"
#ifdef PSYCLE_USE_SEQVIEW
#include "sequenceview.h"
#endif
#ifdef PSYCLE_USE_SEQEDITOR
#include "seqeditor.h"
#endif
#include "startscript.h"
#include "stepbox.h"
#ifdef PSYCLE_USE_STEPSEQUENCER
#include "stepsequencerview.h"
#endif
#include "songbar.h"
#include "trackscopeview.h"
#include "undoredobar.h"
#include "vubar.h"
#include "workspace.h"
/* ui */
#include <uiframe.h>
#include <uitabbar.h>
#include <uinotebook.h>
#include <uisplitbar.h>
/* container */
#include <properties.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct MainFrame
** @brief The root component of all other components of psycle
**
** @details
** Initializes the workspace_. First component created after program start by
** psycle.c and last destroyed at program termination.
*/

typedef struct MainFrame {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */	
	psy_ui_Component pane_;
	CloseBar close_;
	psy_ui_Component top_;	
	psy_ui_Component top_rows_;
	psy_ui_Component top_row_0;
	psy_ui_Component top_row_0_client_;	
	psy_ui_Component top_row_1_;	
	psy_ui_Component client_;
	psy_ui_Component left_;
#ifdef PSYCLE_USE_FRAME_DRAG
	FrameDrag frame_drag_;
#endif
	psy_ui_Component help_setting_bar;
	psy_ui_Component help_setting_bar_client;
	psy_ui_Button settings_btn;
	psy_ui_Button help_btn_;
	MainViews main_views_;		
	MessageView message_view_;
	psy_ui_Splitter splitter_;
	psy_ui_Splitter splitter_message_view_;
	StartScript start_script_;		
	FileBar file_bar_;
	UndoRedoBar undo_redo_bar_;
	MachineBar machine_bar_;
	SongBar song_bar_;
	PlayBar play_bar_;
	MetronomeBar metronome_bar_;
	PlayPosBar play_pos_bar_;
	TrackScopeView track_scopes_;	
#ifdef PSYCLE_USE_STEPSEQUENCER
	StepsequencerView step_sequencer_;
#endif	
	KeyboardView piano_kbd_;	
#ifdef PSYCLE_USE_SEQVIEW	
	SeqView sequence_view_;
#endif		
	SequencerBar sequencer_bar_;
	psy_ui_Splitter param_gear_splitter_;
	ParamRack param_gear_;
#ifdef PSYCLE_USE_SEQEDITOR	
	psy_ui_Splitter seq_edit_splitter_;
	SeqEditor seq_edit_;
#endif	
	KbdHelp kbd_help_;
	Gear gear_;
	psy_ui_Splitter gear_splitter;
#ifdef PSYCLE_USE_PLUGIN_EDITOR
	PluginEditor plugin_editor_;
	psy_ui_Splitter plugin_editor_splitter_;
#endif	
	VuBar vu_;
	CPUView cpu_;
	psy_ui_Splitter cpu_splitter_;
	MidiMonitor midi_;
	psy_ui_Splitter midi_splitter_;	
	MainStatusBar status_;	
	Workspace workspace_;	
	Interpreter interpreter_;	
	bool title_modified_;
	ParamViews param_views_;
	psy_ui_RealPoint frame_drag_offset_;
	bool allow_frame_move_;	
	bool starting_;		
	/* references */
	psy_Configuration* patview_cfg_;
	psy_Configuration* metronome_cfg_;
} MainFrame;

void mainframe_init(MainFrame*, const PsycleCmdLine*);

MainFrame* mainframe_alloc(void);
MainFrame* mainframe_alloc_init(const PsycleCmdLine*);

bool mainframe_show_maximized_at_start(const MainFrame*);

INLINE psy_ui_Component* mainframe_base(MainFrame* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* MAINFRAME_H */
