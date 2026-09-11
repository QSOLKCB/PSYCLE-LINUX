/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "trackerview.h"

#ifdef PSYCLE_USE_TRACKERVIEW

/* local */
#include "cmddef.h"
#include "cmdsnotes.h"
#include "patternnavigator.h"
/* ui */
#include <trackercmds.h>
/* audio */
#include <sequencecmds.h>
/* platform */
#include "../../detail/portable.h"

#define ISDIGIT TRUE


/* prototypes */
static void trackerview_init_nav_bar(TrackerView*);
static void trackerview_on_song_changed(TrackerView*, psy_audio_Player* sender);
static void trackerview_connect_sequencer(TrackerView*, psy_audio_Sequencer*);
static void trackerview_connect_song(TrackerView*, psy_audio_Song*);
static void trackerview_connect_config(TrackerView*, psy_Configuration*);
static void trackerview_configure(TrackerView*, psy_Configuration*);
static void trackerview_on_cursor_changed(TrackerView*, psy_audio_Sequence*
	sender);
static void trackerview_on_draw_empty_data(TrackerView*, psy_Property* sender);
static void trackerview_on_center_cursor(TrackerView*, psy_Property* sender);
static void trackerview_on_line_numbers(TrackerView*, psy_Property* sender);
static void trackerview_on_single_display(TrackerView*, psy_Property* sender);
static void trackerview_on_play_line_changed(TrackerView*,
	psy_audio_Sequencer* sender);
static void trackerview_on_play_status_changed(TrackerView*,
	psy_audio_Sequencer* sender);
static void trackerview_on_grid_scrolled(TrackerView*, psy_ui_Component*,
	double dx, double dy);
static void trackerview_on_mouse_down(TrackerView*, psy_ui_MouseEvent*);
static void trackerview_on_draw(TrackerView*, psy_ui_Graphics*);
static void trackerview_on_sequence_tweak(TrackerView*,
	psy_audio_Sequence* sender);
static void trackerview_on_track_reposition(TrackerView*,
	psy_audio_Sequence* sender, uintptr_t track_index);
static void trackerview_on_pattern_length_changed(TrackerView*,
	psy_audio_Patterns* sender, uintptr_t pattern_index);
static bool trackerview_on_input(TrackerView*, InputHandler* sender);
static bool trackerview_handle_command(TrackerView*, psy_EventDriverCmd);
static void trackerview_connect_input_handler(TrackerView*, InputHandler*);
static void trackerview_on_navhandler_selected(TrackerView*, psy_ui_NavHandler*);
	
/* vtable */
static psy_ui_ComponentVtable trackerview_vtable;
static bool trackerview_vtable_initialized = FALSE;

static void trackerview_vtable_init(TrackerView* self)
{
	if (!trackerview_vtable_initialized) {
		trackerview_vtable = *(self->component.vtable);
		trackerview_vtable.ondraw =
			(psy_ui_fp_component_ondraw)
			trackerview_on_draw;
		trackerview_vtable.on_mouse_down =
			(psy_ui_fp_component_on_mouse_event)
			trackerview_on_mouse_down;
		trackerview_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &trackerview_vtable);
}

/* implementation */
void trackerview_init(TrackerView* self, psy_ui_Component* parent,
	TrackerState* state, psy_Configuration* config,
	Workspace* workspace)
{
	assert(self);
	assert(workspace);
	
	psy_ui_component_init(&self->component, parent, NULL);	
	trackerview_vtable_init(self);
	self->workspace = workspace;	
	/* hscroll */
	psy_ui_scrollbar_init(&self->hscroll, &self->component);
	psy_ui_component_set_align(psy_ui_scrollbar_base(&self->hscroll),
		psy_ui_ALIGN_BOTTOM);
	/* vscroll */
	psy_ui_scrollbar_init(&self->vscroll, &self->component);
	psy_ui_scrollbar_set_orientation(&self->vscroll, psy_ui_VERTICAL);
	psy_ui_component_set_align(psy_ui_scrollbar_base(&self->vscroll),
		psy_ui_ALIGN_RIGHT);
	/* lines */
	trackerlinenumberview_init(&self->lines, &self->component, state);
	psy_ui_component_set_align(&self->lines.component, psy_ui_ALIGN_LEFT);
	/* grid */
	trackergrid_init(&self->grid, &self->component, state,
		filechooser_base(&workspace->file_chooser));		
	psy_ui_component_set_wheel_scroll(trackergrid_base(&self->grid), 4);
	psy_ui_component_set_overflow(trackergrid_base(&self->grid),
		psy_ui_OVERFLOW_SCROLL);
	/* scroll */
	psy_ui_scroller_init(&self->scroller, &self->component, &self->hscroll,
		&self->vscroll);
	psy_ui_scroller_set_client(&self->scroller, trackergrid_base(&self->grid));	
	psy_ui_component_set_align(&self->scroller.component, psy_ui_ALIGN_CLIENT);	
	psy_signal_connect(&workspace->player_.signal_song_changed, self,
		trackerview_on_song_changed);
	psy_signal_connect(&self->grid.component.signal_scrolled, self,
		trackerview_on_grid_scrolled);
	trackerview_connect_sequencer(self, &workspace->player_.sequencer);
	trackerview_connect_song(self, workspace_song(workspace));	
	trackerview_connect_config(self, config);
	trackerview_configure(self, config);
	/* navbar */
	trackerview_init_nav_bar(self);
}

void trackerview_init_nav_bar(TrackerView* self)
{
	assert(self);

	// navbar_init(&self->nav_bar, trackerview_base(self), input_handler);
	// psy_ui_component_hide(navbar_base(&self->nav_bar));
	// psy_ui_component_set_align(navbar_base(&self->nav_bar),
	//	psy_ui_ALIGN_BOTTOM);
	//navbar_add(&self->nav_bar, "<", psy_INDEX_INVALID);
	//noteselect_init(&self->note_select, navbar_base(&self->nav_bar),
	//	workspace_player(self->workspace), input_handler);
	//++self->nav_bar.nav_handler.num_items;
	//navbar_add(&self->nav_bar, "Edit", psy_INDEX_INVALID);
	//psy_signal_connect(&self->nav_bar.nav_handler.signal_selected, self,
	//	trackerview_on_navhandler_selected);
	//navbar_select(&self->nav_bar, 1);
	trackerview_connect_input_handler(self, psy_ui_app_input_handler(
		psy_ui_app()));
}

void trackerview_connect_input_handler(TrackerView* self, InputHandler* input_handler)
{
	assert(self);

	if (!input_handler) {
		return;
	}
	inputhandler_connect(input_handler, INPUTHANDLER_FOCUS,
		psy_EVENTDRIVER_CMD, "tracker", psy_INDEX_INVALID,
		self, &self->grid, (fp_inputhandler_input)trackerview_on_input);
}

void trackerview_on_song_changed(TrackerView* self, psy_audio_Player* sender)
{
	assert(self);
	
	psy_audio_sequencecursor_init(&self->grid.old_cursor);
	trackerview_connect_song(self, psy_audio_player_song(sender));	
}

void trackerview_connect_sequencer(TrackerView* self, psy_audio_Sequencer*
	sequencer)
{	
	assert(self);
		
	psy_signal_connect(&sequencer->signal_play_line_changed,
		self, trackerview_on_play_line_changed);
	psy_signal_connect(&sequencer->signal_play_status_changed,
		self, trackerview_on_play_status_changed);
}

void trackerview_connect_song(TrackerView* self, psy_audio_Song* song)
{
	psy_audio_Sequence* sequence;
	
	assert(self);
	
	if (!song) {
		return;
	}	
	sequence = psy_audio_song_sequence(song);
	if (sequence) {		
		psy_signal_connect(&sequence->signal_cursor_changed,
			self, trackerview_on_cursor_changed);
		psy_signal_connect(&sequence->signal_tweak,
			self, trackerview_on_sequence_tweak);
		psy_signal_connect(&sequence->signal_track_reposition,
			self, trackerview_on_track_reposition);
		psy_signal_connect(&song->patterns_.signal_length_changed,
			self, trackerview_on_pattern_length_changed);
	}
}

void trackerview_connect_config(TrackerView* self, psy_Configuration* config)
{
	assert(self);

	if (!config) {
		return;
	}
	psy_configuration_connect(config, "drawemptydata",
		self, trackerview_on_draw_empty_data);
	psy_configuration_connect(config, "linenumbers",
		self, trackerview_on_line_numbers);
	psy_configuration_connect(config, "centercursoronscreen",
		self, trackerview_on_center_cursor);
	psy_configuration_connect(config, "displaysinglepattern",
		self, trackerview_on_single_display);
}

void trackerview_configure(TrackerView* self, psy_Configuration* config)
{
	assert(self);

	if (!config) {
		return;
	}
	psy_configuration_configure(config, "drawemptydata");		
	psy_configuration_configure(config, "linenumbers");		
	psy_configuration_configure(config, "centercursoronscreen");		
	psy_configuration_configure(config, "displaysinglepattern");		
}

void trackerview_on_cursor_changed(TrackerView* self,
	psy_audio_Sequence* sender)
{		
	bool invalidate_cursor;
	bool invalidate_align;
	
	assert(self);
				
	if (psy_audio_sequence_lpb_changed(sender)) {		
		psy_ui_component_align(&self->component);
		psy_ui_component_invalidate(&self->component);
	}
	invalidate_cursor = TRUE;
	invalidate_align = FALSE;	
	if (patternviewstate_single_mode(self->grid.state->pv)) {		
		if (trackerstate_playing_following_song(self->grid.state) &&
			(psy_dsp_beatpos_less(
				self->grid.state->pv->sequence->cursor.offset,
				self->grid.state->pv->sequence->lastcursor.offset))) {
			psy_ui_component_set_scroll_top_px(&self->grid.component, 0.0);			
			invalidate_align = TRUE;
			invalidate_cursor = FALSE;
		} else if (!psy_audio_orderindex_equal(
				&self->grid.state->pv->sequence->cursor.order_index,
				self->grid.state->pv->sequence->lastcursor.order_index)) {
			if (trackerstate_playing_following_song(self->grid.state)) {				
				psy_ui_component_set_scroll_top_px(&self->grid.component, 0.0);
			}
			invalidate_align = TRUE;
			invalidate_cursor = FALSE;
		}			
	} else if (trackerstate_playing_following_song(self->grid.state)) {		
		if (psy_dsp_beatpos_less(self->grid.state->pv->sequence->cursor.offset,
				self->grid.state->pv->sequence->lastcursor.offset)) {
			psy_ui_component_set_scroll_top_px(&self->grid.component, 0.0);
			return;		
		}
	}
	if (!psy_ui_component_draw_visible(&self->component)) {
		return;
	}
	if (invalidate_cursor && !(trackerstate_playing_following_song(
			self->grid.state))) {
		trackerlinenumbers_invalidate_cursor(&self->lines.linenumbers);
		trackergrid_invalidate_cursor(&self->grid);
	}
	if (invalidate_align) {	
		trackerview_align(self);		
	}
}

void trackerview_on_play_line_changed(TrackerView* self,
	psy_audio_Sequencer* sender)
{
	assert(self);
	
	if (!psy_ui_component_draw_visible(trackerview_base(self))) {
		return;
	}
	if (trackerstate_playing_following_song(self->grid.state)) {		
		trackergrid_scroll_down(&self->grid, *patternviewstate_cursor(
			self->grid.state->pv), FALSE);		
	}
	//psy_ui_component_invalidate(&self->grid.component);	
	trackergrid_invalidate_playbar(&self->grid);
	trackerlinenumbers_invalidate_playbar(&self->lines.linenumbers);	
}

void trackerview_on_play_status_changed(TrackerView* self,
	psy_audio_Sequencer* sender)
{	
	assert(self);
	
	trackerlinenumbers_invalidate_playbar(&self->lines.linenumbers);
	trackergrid_invalidate_playbar(&self->grid);
	trackerlinenumbers_invalidate_cursor(&self->lines.linenumbers);
	trackergrid_invalidate_cursor(&self->grid);
}

void trackerview_on_grid_scrolled(TrackerView* self, psy_ui_Component* sender,
	double dx, double dy)
{	
	assert(self);
	
	trackerlinenumberview_set_scroll_top(&self->lines,
		psy_ui_component_scroll_top(sender));				
	trackerstate_scroll_to(self->grid.state, sender, dx, dy);	
}

void trackerview_on_draw_empty_data(TrackerView* self, psy_Property* sender)
{
	assert(self);
	
	trackergrid_show_empty_data(&self->grid, psy_property_item_bool(
		sender));
}

void trackerview_on_center_cursor(TrackerView* self, psy_Property* sender)
{
	assert(self);
		
	trackergrid_set_center_mode(&self->grid, psy_property_item_int(sender));
}

void trackerview_on_line_numbers(TrackerView* self, psy_Property* sender)
{
	assert(self);
	
	if (psy_property_item_bool(sender)) {
		psy_ui_component_show(trackerlinenumberview_base(&self->lines));		
	} else {
		psy_ui_component_hide(trackerlinenumberview_base(&self->lines));
	}	
	psy_ui_component_align(&self->component);
	psy_ui_component_invalidate(&self->component);
}

void trackerview_on_single_display(TrackerView* self, psy_Property* sender)
{
	assert(self);
	
	psy_ui_component_align(&self->component);
	psy_ui_component_invalidate(&self->component);
}

void trackerview_on_mouse_down(TrackerView* self, psy_ui_MouseEvent* ev)
{
	assert(self);

	if (psy_ui_mouseevent_button(ev) == 1) {
		psy_ui_component_set_focus(&self->grid.component);
		psy_ui_mouseevent_stop_propagation(ev);
	}
}

void trackerview_on_draw(TrackerView* self, psy_ui_Graphics* g)
{
	assert(self);
				
	psy_audio_sequencecursor_update_abs(&self->grid.state->pv->cursor,
		self->grid.state->pv->sequence);	
}

void trackerview_on_sequence_tweak(TrackerView* self,
	psy_audio_Sequence* sender)
{
	assert(self);
	
	trackergrid_invalidate_internal_cursor(&self->grid,
		self->grid.state->pv->cursor);
}

void trackerview_on_pattern_length_changed(TrackerView* self,
	psy_audio_Patterns* sender, uintptr_t pattern_index)
{
	assert(self);
	
	if (patternviewstate_single_mode(self->grid.state->pv)) {
		trackerview_align(self);
	} /* else handled in patternview_on_track_reposition */
}

void trackerview_on_track_reposition(TrackerView* self,
	psy_audio_Sequence* sender, uintptr_t track_index)
{
	assert(self);
	
	if (!patternviewstate_single_mode(self->grid.state->pv)) {
		trackerview_align(self);		
	}
}

void trackerview_align(TrackerView* self)
{
	assert(self);
		
	psy_ui_component_align(&self->scroller.pane);
	psy_ui_component_align(&self->lines.component);
	psy_ui_component_invalidate(&self->component);
	psy_ui_component_invalidate(&self->lines.component);
}

void trackerview_make_cmds(psy_Property* parent)
{
	psy_Property* cmds;

	assert(parent);

	cmds = psy_property_set_text(psy_property_append_section(parent,
		"tracker"), "Trackerview");
	set_cmd_all(cmds, CMD_NAVUP,
		psy_ui_KEY_UP, psy_SHIFT_OFF, psy_CTRL_OFF,
		"navup", "up");	
	set_cmd_all(cmds, CMD_NAVDOWN,
		psy_ui_KEY_DOWN, psy_SHIFT_OFF, psy_CTRL_OFF,
		"navdown", "down");
	set_cmd_all(cmds, CMD_NAVLEFT,
		psy_ui_KEY_LEFT, psy_SHIFT_OFF, psy_CTRL_OFF,
		"navleft", "left");
	set_cmd_all(cmds, CMD_NAVRIGHT,
		psy_ui_KEY_RIGHT, psy_SHIFT_OFF, psy_CTRL_OFF,
		"navright", "right");
	set_cmd_all(cmds, CMD_NAVPAGEUP,
		psy_ui_KEY_PRIOR, psy_SHIFT_OFF, psy_CTRL_OFF,
		"navpageup", "pageup");
	set_cmd_all(cmds, CMD_NAVPAGEDOWN,
		psy_ui_KEY_NEXT, psy_SHIFT_OFF, psy_CTRL_OFF,
		"navpagedown", "pagedown");
	set_cmd_all(cmds, CMD_NAVPAGEUPKEYBOARD,
		psy_ui_KEY_PRIOR, psy_SHIFT_ON, psy_CTRL_OFF,
		"navpageupkbd", "pageup kbd");
	set_cmd_all(cmds, CMD_NAVPAGEDOWNKEYBOARD,
		psy_ui_KEY_NEXT, psy_SHIFT_ON, psy_CTRL_OFF,
		"navpagedownkbd", "pagedown kbd");
	set_cmd(cmds, CMD_NAVTOP, psy_ui_KEY_HOME, "navtop", "track top");
	set_cmd(cmds, CMD_NAVBOTTOM, psy_ui_KEY_END, "navbottom", "track bottom");
	set_cmd_all(cmds, CMD_COLUMNPREV, psy_ui_KEY_TAB, psy_SHIFT_ON, psy_CTRL_OFF,
		"columnprev", "prev col");
	set_cmd(cmds, CMD_COLUMNNEXT, psy_ui_KEY_TAB, "columnnext", "next col");
	set_cmd(cmds, CMD_DIGIT0, psy_ui_KEY_DIGIT0, "digit0", "0");
	set_cmd(cmds, CMD_DIGIT1, psy_ui_KEY_DIGIT1, "digit1", "1");
	set_cmd(cmds, CMD_DIGIT2, psy_ui_KEY_DIGIT2, "digit2", "2");
	set_cmd(cmds, CMD_DIGIT3, psy_ui_KEY_DIGIT3, "digit3", "3");
	set_cmd(cmds, CMD_DIGIT4, psy_ui_KEY_DIGIT4, "digit4", "4");
	set_cmd(cmds, CMD_DIGIT5, psy_ui_KEY_DIGIT5, "digit5", "5");
	set_cmd(cmds, CMD_DIGIT6, psy_ui_KEY_DIGIT6, "digit6", "6");
	set_cmd(cmds, CMD_DIGIT7, psy_ui_KEY_DIGIT7, "digit7", "7");
	set_cmd(cmds, CMD_DIGIT8, psy_ui_KEY_DIGIT8, "digit8", "8");
	set_cmd(cmds, CMD_DIGIT9, psy_ui_KEY_DIGIT9, "digit9", "9");
	set_cmd(cmds, CMD_DIGITA, psy_ui_KEY_A, "digitA", "A");
	set_cmd(cmds, CMD_DIGITB, psy_ui_KEY_B, "digitB", "B");
	set_cmd(cmds, CMD_DIGITC, psy_ui_KEY_C, "digitC", "C");
	set_cmd(cmds, CMD_DIGITD, psy_ui_KEY_D, "digitD", "D");
	set_cmd(cmds, CMD_DIGITE, psy_ui_KEY_E, "digitE", "E");
	set_cmd(cmds, CMD_DIGITF, psy_ui_KEY_F, "digitF", "F");
	set_cmd_all(cmds, CMD_NAVSELECT, psy_ui_KEY_RETURN, psy_SHIFT_ON, psy_CTRL_ON,
		"navselect", "nav");
}

bool trackerview_on_input(TrackerView* self, InputHandler* sender)
{
	assert(self);

	return trackerview_handle_command(self, inputhandler_cmd(sender));
}

bool trackerview_handle_command(TrackerView* self, psy_EventDriverCmd cmd)
{
	assert(self);

	switch (psy_eventdrivercmd_id(&cmd)) {
	case CMD_NAVSELECT:
		// psy_ui_component_show(navbar_base(&self->nav_bar));
		inputhandler_send(psy_ui_app_input_handler(psy_ui_app()), "edit",
			psy_eventdrivercmd_make_cmd(CMD_PATTERN_MENU));
		return TRUE;
	default:
		return FALSE;
	}
}

void trackerview_on_navhandler_selected(TrackerView* self, psy_ui_NavHandler* nav_handler)
{
	assert(self);

	// switch (nav_handler->selected)
	//{
	//case 0:
	//	if (self->nav_back) {
	//		psy_ui_component_show(navbar_base(self->nav_back));
	//		psy_ui_component_hide(navbar_base(&self->nav_bar));
	//		psy_ui_component_align_invalidate(psy_ui_app_main(psy_ui_app()));
	//		psy_ui_component_set_focus(navbar_base(self->nav_back));
	//	}
	//	break;
	//case 1: /* enter note */		
	//	psy_ui_component_set_focus(trackergrid_base(&self->grid));
	//	noteselect_enter(&self->note_select);
	//	psy_ui_component_set_focus(navbar_base(&self->nav_bar));
	//	break;
	//case 2: /* edit trackgrid */
	//	psy_ui_component_hide(navbar_base(&self->nav_bar));
	//	psy_ui_component_align_invalidate(trackerview_base(self));
	//	psy_ui_component_set_focus(trackergrid_base(&self->grid));
	//default:
	//	break;
	//}
}

#endif /* PSYCLE_USE_TRACKERVIEW */
