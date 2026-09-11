/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(SEQUENCERBAR_H)
#define SEQUENCERBAR_H

/* ui */
#include <uibutton.h>
/* container */
#include <configuration.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct SequencerBar
** @brief Settings of the sequencer/pattern/player
**
** @detail
** Changes settings of the sequencer/view/player
** - Follow song – Make the cursor follow the song progress during playback. 
** - Record note-off – Record Noteoffs (when the key is lifted) during
**   real-time recording.
** - Record tweak – Record any mouse tweaks (altering of parameters)
**   during real-time recording
** - Multitrack playback on keypress – Allows multiple notes to be played at
**   the same time while playing with a PC keyboard.
** - Allow notes to effect machines – Allows sending notes to effects machines
**   while using the PC keyboard. This can be used with certain VSTs.
**   Otherwise, notes will be sent to the selected generator.
** - Show/hides the seq editor and step sequencer
*/
typedef struct SequencerBar {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Component main_settings_;
	psy_List* setting_items_;
	psy_ui_Button follow_song_;
	psy_ui_Button show_names_;
	psy_ui_Button record_note_off_;
	psy_ui_Button record_tweak_;
	psy_ui_Button multi_channel_audition_;
	psy_ui_Button allow_notes_to_effect_;
	psy_ui_Button more_;
	uintptr_t num_more_;
	bool doresize;
	
	psy_ui_Component more_settings_;
	psy_ui_Component view_buttons_;
	psy_ui_Button toggle_seq_edit_;
	psy_ui_Button toggle_step_seq_;
	psy_ui_Button toggle_kbd_;	
} SequencerBar;

void sequencerbar_init(SequencerBar*, psy_ui_Component* parent,
	psy_Configuration* misc, psy_Configuration* general);

INLINE psy_ui_Component* sequencerbar_base(SequencerBar* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* SEQUENCERBAR_H */
