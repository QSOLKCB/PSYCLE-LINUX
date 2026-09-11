/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "patternview.h"

#ifdef PSYCLE_USE_PATTERN_VIEW

/* host */
#include "resources/resource.h"
#include "skinio.h"
#include "styles.h"
/* ui */
#include <uiapp.h>
#include <uistyle.h>
/* platform */
#include "../../detail/os.h"
#include "../../detail/portable.h"

#if defined DIVERSALIS__OS__UNIX
#define _MAX_PATH 4096
#endif


/* prototypes */
static void patternview_on_destroyed(PatternView*);
static void patternview_init_component(PatternView*);
static void patternview_init_layout(PatternView*);
static void patternview_init_pv_state(PatternView*);
static void patternview_init_tracker_view(PatternView*);
static void patternview_init_bar(PatternView*);
static void patternview_init_pianoroll(PatternView*, psy_DiskOp*);
static void patternview_init_block_menu(PatternView*);
static void patternview_init_transform_pattern_view(PatternView*);
static void patternview_init_swing_fill_view(PatternView*);
static void patternview_init_edit_tracknames(PatternView*);
static void patternview_init_interpolate_view(PatternView*);
static void patternview_init_tabbar(PatternView*,
	psy_ui_Component* tabbarparent);
static void patternview_connect_app_zoom(PatternView*);
static void patternview_connect_input_handler(PatternView*);
static void patternview_rebuild(PatternView*);
static void patternview_on_song_changed(PatternView*, psy_audio_Player* sender);
static void patternview_connect_song(PatternView*);
static void patternview_connect_config(PatternView*,
	psy_Configuration* pat_config);
static void patternview_configure(PatternView*);
static void patternview_on_zoom(PatternView*, psy_Property*);
static void patternview_on_font(PatternView*, psy_Property*);
static void patternview_on_select_display(PatternView*, psy_Property*);
static void patternview_on_follow_song(PatternView*, psy_Property* sender);
static void patternview_on_wide_inst(PatternView*, psy_Property* sender);
static void patternview_on_focus(PatternView*);
static void patternview_on_mouse_down(PatternView*, psy_ui_MouseEvent*);
static void patternview_on_mouse_up(PatternView*, psy_ui_MouseEvent*);
static void patternview_on_key_down(PatternView*, psy_ui_KeyboardEvent*);
static void patternview_on_context_menu(PatternView*, psy_ui_Component*);
static void patternview_on_cursor_changed(PatternView*, psy_audio_Sequence*);
static void patternview_update_cursor(PatternView*);
static void patternview_on_grid_scroll(PatternView*, psy_ui_Component*,
	double dx, double dy);
static void patternview_on_app_zoom(PatternView*, psy_ui_AppZoom*);
static void patternview_num_tracks_changed(PatternView*, psy_audio_Pattern*,
	uintptr_t numsongtracks);
static void patternview_on_column_resize(PatternView*, TrackerGrid*);
static void patternview_update_scroll_step(PatternView*);
static void patternview_select_display(PatternView*, PatternDisplayMode);
static void patternview_update_font(PatternView*);
static psy_ui_FontInfo patternview_zoom_font_info(PatternView*, double zoom);
static void patternview_select_section(PatternView*, psy_ui_Component* sender,
	uintptr_t section, uintptr_t options);
static void patternview_set_display_config(PatternView*, PatternDisplayMode);
static uintptr_t patternview_section(const PatternView*);
static bool patternview_on_automate(PatternView*, InputHandler* sender);
static bool patternview_on_edt_cmd(PatternView*, InputHandler* sender);
static void patternview_toggle_pattern_menu(PatternView*);

/* vtable */
static psy_ui_ComponentVtable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(PatternView* self)
{
	if (!vtable_initialized) {
		vtable = *(self->component.vtable);
		vtable.on_destroyed =
			(psy_ui_fp_component)
			patternview_on_destroyed;
		vtable.on_mouse_down =
			(psy_ui_fp_component_on_mouse_event)
			patternview_on_mouse_down;
		vtable.on_mouse_up =
			(psy_ui_fp_component_on_mouse_event)
			patternview_on_mouse_up;
		vtable.on_key_down =
			(psy_ui_fp_component_on_key_event)
			patternview_on_key_down;		
		vtable.on_focus =
			(psy_ui_fp_component)
			patternview_on_focus;
		vtable.section =
			(psy_ui_fp_component_section)
			patternview_section;
		vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(patternview_base(self), &vtable);
}

/* implementation */
void patternview_init(PatternView* self, psy_ui_Component* parent,
	psy_ui_Component* tabbarparent,	psy_Configuration* pat_config,
	Workspace* workspace)
{
	assert(self);
	assert(workspace);
	
	psy_ui_component_init(&self->component, parent, NULL);
	vtable_init(self);
	self->pat_cfg = pat_config;
	self->workspace = workspace;	
	self->display = PATTERN_DISPLAYMODE_TRACKER;
	self->prevent_grid_scroll = FALSE;
	psy_signal_connect(&workspace->player_.signal_song_changed, self,
		patternview_on_song_changed);
	patternview_init_component(self);
	patternview_init_layout(self);	
	patternview_init_pv_state(self);
	patternview_connect_song(self);	
	patternproperties_init(&self->properties, &self->component, &self->pvstate);
	patternview_init_bar(self);	
	patternview_init_tracker_view(self);
	patternview_init_pianoroll(self, filechooser_base(
		&self->workspace->file_chooser));
	patternview_init_block_menu(self);	
	patternview_init_transform_pattern_view(self);
	patternview_init_swing_fill_view(self);
	patternview_init_edit_tracknames(self);
	patternview_init_interpolate_view(self);
	patternview_init_tabbar(self, tabbarparent);	
	patternview_connect_app_zoom(self);
	patternview_connect_input_handler(self);	
	patternviewstyles_init(&self->pattern_styles, psycleconfig_directories(
		workspace_cfg(self->workspace)));
	patternstyleconfigurator_init(&self->pattern_style_configurator,
		&self->pattern_styles, pat_config);
	patternview_connect_config(self, pat_config);	
	patternview_configure(self);
	patternview_rebuild(self);	
}

void patternview_on_destroyed(PatternView* self)
{	
	assert(self);

#ifdef PSYCLE_USE_TRACKERVIEW
	trackerstate_dispose(&self->state);
	trackconfig_dispose(&self->track_config);
#endif	
	patternviewstate_dispose(&self->pvstate);
}

void patternview_init_component(PatternView* self)
{
	assert(self);
		
	psy_ui_component_set_tab_index(patternview_base(self), 0);
	psy_ui_component_set_title(patternview_base(self), "main.patterns");
	psy_ui_component_set_id(patternview_base(self), VIEW_ID_PATTERNS);	
	psy_signal_connect(&self->component.signal_select_section, self,
		patternview_select_section);
}

void patternview_init_layout(PatternView* self)
{
	assert(self);
		
	psy_ui_notebook_init(&self->notebook, &self->component);
	psy_ui_component_set_style_type(psy_ui_notebook_base(&self->notebook),
		STYLE_PATTERNVIEW);
	psy_ui_component_set_align(psy_ui_notebook_base(&self->notebook),
		psy_ui_ALIGN_CLIENT);	
}

void patternview_init_pv_state(PatternView* self)
{
	assert(self);
		
	patternviewstate_init(&self->pvstate,
		psycleconfig_patview(workspace_cfg(self->workspace)),
		psycleconfig_misc(workspace_cfg(self->workspace)),
		workspace_player(self->workspace),
		NULL,		
		&self->workspace->undo_redo_);	
}

void patternview_init_tracker_view(PatternView* self)
{
	assert(self);
	
#ifdef PSYCLE_USE_TRACKERVIEW
	trackconfig_init(&self->track_config, psy_configuration_value_bool(
		self->pat_cfg, "wideinstcolumn", FALSE));
	trackerstate_init(&self->state, &self->track_config, &self->pvstate,
		self->workspace);
	/* header */
	trackerheaderview_init(&self->header, &self->component, 
		&self->track_config, &self->state, self->workspace);	
	psy_ui_component_set_align(&self->header.component, psy_ui_ALIGN_TOP);
	/* defaultline */
	patterndefaultline_init(&self->defaultline, &self->component,
		&self->pvstate, &self->track_config,
		psycleconfig_patview(workspace_cfg(self->workspace)),
		self->workspace);	
	psy_signal_connect(&self->defaultline.grid.signal_col_resize, self,
		patternview_on_column_resize);
	/* tracker */
	trackerview_init(&self->trackerview, &self->notebook.component,		
		&self->state, psycleconfig_patview(workspace_cfg(self->workspace)),
		self->workspace);
	psy_signal_connect(&self->trackerview.grid.signal_col_resize, self,
		patternview_on_column_resize);	
	psy_signal_connect(&self->trackerview.grid.component.signal_scrolled, self,
		patternview_on_grid_scroll);	
	psy_signal_connect(&self->defaultline.grid.component.signal_scrolled, self,
		patternview_on_grid_scroll);
#endif			
}

void patternview_init_bar(PatternView* self)
{
	assert(self);
	
	patternviewbar_init(&self->patternviewbar, &self->component,
		psycleconfig_patview(workspace_cfg(self->workspace)),
		self->workspace);
	psy_ui_component_set_margin(&self->patternviewbar.component,
		psy_ui_margin_make_em(0.5, 0.0, 0.0, 0.0));
	psy_ui_component_set_align(&self->patternviewbar.component,
		psy_ui_ALIGN_BOTTOM);
	self->component.sizer_ = psy_ui_sizer_base(&self->patternviewbar.sizer_);
}

void patternview_init_pianoroll(PatternView* self, psy_DiskOp* disk_op)
{
	assert(self);
	
#ifdef PSYCLE_USE_PIANOROLL		
	pianoroll_init(&self->pianoroll, &self->notebook.component,
		&self->pvstate,
		disk_op, self->workspace);		
#endif		
}

void patternview_init_block_menu(PatternView* self)
{
	assert(self);
		
	patternmenu_init(&self->blockmenu, patternview_base(self),
		trackergrid_base(&self->trackerview.grid),
		workspace_player(self->workspace));
}

void patternview_init_transform_pattern_view(PatternView* self) 
{
	assert(self);

	transformpatternview_init(&self->transformpattern, &self->component,
		&self->pvstate, self->workspace);
}

void patternview_init_swing_fill_view(PatternView* self)
{
	assert(self);
	
	swingfillview_init(&self->swing_fill_, &self->component, &self->pvstate);
}

void patternview_init_edit_tracknames(PatternView* self)
{
	assert(self);
	
	tracknameedit_init(&self->tracknames, &self->component, &self->pvstate,
		self->workspace);
}


void patternview_init_interpolate_view(PatternView* self)
{
	assert(self);
		
	interpolatecurveview_init(&self->interpolatecurveview, &self->component,
		&self->pvstate, self->workspace);
	psy_ui_component_set_align(&self->interpolatecurveview.component,
		psy_ui_ALIGN_BOTTOM);
	psy_ui_component_hide(&self->interpolatecurveview.component);
	/* splitter */
	psy_ui_splitter_init(&self->splitter, &self->component);
	psy_ui_component_set_align(psy_ui_splitter_base(&self->splitter),
		psy_ui_ALIGN_BOTTOM);
	psy_ui_component_hide(psy_ui_splitter_base(&self->splitter));	
}

void patternview_init_tabbar(PatternView* self, psy_ui_Component* tabbarparent)
{
	assert(self);
		
	patternviewtabbar_init(&self->tabbar, tabbarparent,
		psycleconfig_patview(workspace_cfg(self->workspace)),
		self->workspace);
	self->display = PATTERN_DISPLAYMODE_INVALID;
	patternview_select_display(self, PATTERN_DISPLAYMODE_TRACKER);
	psy_signal_connect(&self->tabbar.context_button.signal_clicked, self,
		patternview_on_context_menu);
}

void patternview_connect_app_zoom(PatternView* self)
{
	assert(self);
	
	psy_signal_connect(&psy_ui_app_zoom(psy_ui_app())->signal_zoom, self,
		patternview_on_app_zoom);
}

void patternview_connect_input_handler(PatternView* self)
{
	assert(self);
	
	inputhandler_connect(psy_ui_app_input_handler(psy_ui_app()),
		INPUTHANDLER_IMM, psy_EVENTDRIVER_CMD, "edit", psy_INDEX_INVALID,
		self, NULL, (fp_inputhandler_input)patternview_on_edt_cmd);
	inputhandler_connect(psy_ui_app_input_handler(psy_ui_app()),
		INPUTHANDLER_IMM, psy_EVENTDRIVER_AUTOMATE, "tracker", psy_INDEX_INVALID,
		self, NULL, (fp_inputhandler_input)patternview_on_automate);
}

void patternview_on_song_changed(PatternView* self, psy_audio_Player* sender)
{	
	assert(self);

	patternview_connect_song(self);	
	psy_audio_sequencecursor_set_order_index(&self->pvstate.cursor,	
		psy_audio_orderindex_make_invalid());
	patternview_rebuild(self);
	patternview_update_cursor(self);
	psy_ui_component_align(&self->component);
	psy_ui_component_invalidate(&self->component);
}

void patternview_connect_song(PatternView* self)
{	
	psy_audio_Song* song;

	assert(self);

	song = workspace_song(self->workspace);
	if (song) {
		patternviewstate_set_sequence(&self->pvstate, &song->sequence_);		
		psy_signal_connect(&song->sequence_.signal_cursor_changed, self,
			patternview_on_cursor_changed);
		psy_signal_connect(&song->patterns_.signal_numsongtrackschanged, self,
			patternview_num_tracks_changed);
	} else {
		patternviewstate_set_sequence(&self->pvstate, NULL);		
	}
}

void patternview_connect_config(PatternView* self,
	psy_Configuration* pat_config)
{	
	assert(self);

	psy_configuration_connect(pat_config, "patterndisplay",
		self, patternview_on_select_display);
	psy_configuration_connect(pat_config, "zoom",
		self, patternview_on_zoom);
	psy_configuration_connect(pat_config, "font",
		self, patternview_on_font);
	psy_configuration_connect(pat_config, "wideinstcolumn",
		self, patternview_on_wide_inst);
	psy_configuration_connect(self->pvstate.misc_cfg_, "followsong",
		self, patternview_on_follow_song);
	/* theme */
	patternstyleconfigurator_connect(&self->pattern_style_configurator);
}

void patternview_configure(PatternView* self)
{			
	assert(self);	
	
	psy_configuration_configure(self->pat_cfg, "wideinst");
	psy_configuration_configure(self->pat_cfg, "patterndisplay");
	psy_configuration_configure(self->pat_cfg, "zoom");
	psy_configuration_configure(self->pat_cfg, "font");
	psy_configuration_configure(self->pat_cfg, "griddefaults");
	patternstyleconfigurator_configure(&self->pattern_style_configurator);
}

void patternview_on_focus(PatternView* self)
{
	assert(self);

	if (psy_ui_tabbar_selected(&self->tabbar.tabbar) == 1) { /* Pianoroll */
#ifdef PSYCLE_USE_PIANOROLL		
		psy_ui_component_set_focus(&self->pianoroll.grid.component);		
#endif		
	} else {
#ifdef PSYCLE_USE_TRACKERVIEW		
		psy_ui_component_set_focus(&self->trackerview.grid.component);
#endif		
	}
}

void patternview_on_context_menu(PatternView* self, psy_ui_Component* sender)
{
	assert(self);

	psy_ui_component_toggle_visibility(patternmenu_base(&self->blockmenu));	
	psy_ui_component_invalidate(&self->component);
}

void patternview_select_display(PatternView* self, PatternDisplayMode display)
{
	assert(self);

	if (self->display != display) {
		self->display = display;
		switch (display) {			
		case PATTERN_DISPLAYMODE_TRACKER:
		case PATTERN_DISPLAYMODE_PIANOROLL:
			psy_ui_tabbar_mark(&self->tabbar.tabbar, display);
			psy_ui_notebook_full(&self->notebook);
			psy_ui_notebook_select(&self->notebook, display);
			if (display == PATTERN_DISPLAYMODE_PIANOROLL) {
				pianoroll_align(&self->pianoroll, TRUE, TRUE);				
			}
			break;
		case PATTERN_DISPLAYMODE_TRACKER_PIANOROLL_VERTICAL:
			psy_ui_tabbar_mark(&self->tabbar.tabbar, 0);
			psy_ui_notebook_split(&self->notebook, psy_ui_VERTICAL);
			break;
		case PATTERN_DISPLAYMODE_TRACKER_PIANOROLL_HORIZONTAL:
			psy_ui_tabbar_mark(&self->tabbar.tabbar, 0);
			psy_ui_notebook_split(&self->notebook, psy_ui_HORIZONTAL);
			break;
		default:
			break;
		}
		psy_ui_component_invalidate(psy_ui_notebook_base(&self->notebook));
	}	
}

void patternview_on_font(PatternView* self, psy_Property* sender)
{
	patternview_update_font(self);
}

void patternview_on_zoom(PatternView* self, psy_Property* sender)
{
	assert(self);

	patternview_update_font(self);
}

void patternview_on_select_display(PatternView* self, psy_Property* sender)
{
	assert(self);
	
	patternview_select_display(self, (PatternDisplayMode)
		psy_property_item_int(sender));	
}

void patternview_on_follow_song(PatternView* self, psy_Property* sender)
{
	assert(self);
	
#ifdef PSYCLE_USE_TRACKERVIEW	
	psy_ui_component_invalidate(&self->trackerview.component);
#endif	
}

void patternview_on_wide_inst(PatternView* self, psy_Property* sender)
{
	assert(self);
	
#ifdef PSYCLE_USE_TRACKERVIEW	
	trackconfig_init_columns(&self->track_config, psy_property_item_bool(
		sender));		
	psy_ui_component_invalidate(&self->trackerview.component);	
#endif	
}

void patternview_on_grid_scroll(PatternView* self, psy_ui_Component* sender,
	double dx, double dy)
{
	assert(self);
	
#ifdef PSYCLE_USE_TRACKERVIEW	
	if ((dx == 0.0) || (self->prevent_grid_scroll)) {
		return;
	}	
	self->prevent_grid_scroll = TRUE;
	if (sender == trackergrid_base(&self->trackerview.grid)) {			
		psy_ui_component_set_scroll_left(&self->header.header.component,
			psy_ui_component_scroll_left(&self->trackerview.grid.component));
		psy_ui_component_set_scroll_left(&self->defaultline.grid.component,
			psy_ui_component_scroll_left(&self->trackerview.grid.component));			
	} else {
		psy_ui_component_set_scroll_left(&self->header.header.component,
			psy_ui_component_scroll_left(&self->defaultline.grid.component));
		psy_ui_component_set_scroll_left(&self->trackerview.grid.component,
			psy_ui_component_scroll_left(&self->defaultline.grid.component));
	}
	self->prevent_grid_scroll = FALSE;	
#endif	
}

void patternview_on_cursor_changed(PatternView* self,
	psy_audio_Sequence* sender)
{	
	assert(self);
	
	patternview_update_cursor(self);
	if ((sender->lastcursor.channel_ != sender->cursor.channel_)) {
		trackerheader_update_states(&self->header.header);
	}
}

void patternview_update_cursor(PatternView* self)
{
	assert(self);
	
	patternviewstate_sync_cursor_to_sequence(&self->pvstate);	
}

void patternview_num_tracks_changed(PatternView* self,
	psy_audio_Pattern* sender, uintptr_t numsongtracks)
{
	assert(self);
		
	patternview_rebuild(self);
}

void patternview_rebuild(PatternView* self)
{
	assert(self);

#ifdef PSYCLE_USE_TRACKERVIEW
	patterndefaultline_update_song_tracks(&self->defaultline);
	trackerheader_build(&self->header.header);	
	trackergrid_build(&self->trackerview.grid);	
	trackergrid_build(&self->defaultline.grid);
	trackerheader_build(&self->header.header);			
	psy_ui_component_align_full(&self->trackerview.scroller.component);
	psy_ui_component_invalidate(&self->component);	
#endif	
}

void patternview_on_mouse_down(PatternView* self, psy_ui_MouseEvent* ev)
{
	assert(self);
	
	psy_ui_mouseevent_stop_propagation(ev);
}

void  patternview_on_mouse_up(PatternView* self, psy_ui_MouseEvent* ev)
{
	assert(self);

	if (psy_ui_mouseevent_button(ev) == 2) {
		 psy_ui_component_toggle_visibility(patternmenu_base(&self->blockmenu));		 
	}
	psy_ui_mouseevent_stop_propagation(ev);
}

void patternview_on_key_down(PatternView* self, psy_ui_KeyboardEvent* ev)
{
	assert(self);

	if (psy_ui_keyboardevent_keycode(ev) != psy_ui_KEY_ESCAPE) {
		return;
	}
	if (psy_ui_component_visible(patternmenu_base(&self->blockmenu))) {
		psy_ui_component_hide_align(patternmenu_base(&self->blockmenu));
		psy_ui_component_set_focus(&self->component);
	} else {
		psy_ui_component_toggle_visibility(patternmenu_base(&self->blockmenu));
		psy_ui_component_invalidate(&self->component);
		patternmenu_focus_first(&self->blockmenu);
	}
	if (psy_ui_component_visible(interpolatecurveview_base(
			&self->interpolatecurveview))) {
		psy_ui_component_hide_align(interpolatecurveview_base(
			&self->interpolatecurveview));
		psy_ui_component_hide_align(psy_ui_splitter_base(
			&self->splitter));
	}	
	psy_ui_component_invalidate(&self->component);
	psy_ui_keyboardevent_stop_propagation(ev);	
}

void patternview_toggle_pattern_menu(PatternView* self)
{
	assert(self);

	if (psy_ui_component_visible(patternmenu_base(&self->blockmenu))) {
		psy_ui_component_hide_align(patternmenu_base(&self->blockmenu));
		psy_ui_component_set_focus(&self->component);
	} else {
		psy_ui_component_toggle_visibility(patternmenu_base(&self->blockmenu));
		psy_ui_component_invalidate(&self->component);
		patternmenu_focus_first(&self->blockmenu);
	}
}

void patternview_on_column_resize(PatternView* self, TrackerGrid* sender)
{
	assert(self);

#ifdef PSYCLE_USE_TRACKERVIEW
	psy_ui_component_align(&self->defaultline.pane);		
	psy_ui_component_align(&self->trackerview.scroller.pane);	
	psy_ui_component_align(&self->header.pane);
	psy_ui_component_invalidate(&self->component);
#endif	
}

void patternview_on_app_zoom(PatternView* self, psy_ui_AppZoom* sender)
{
	assert(self);

	patternview_update_font(self);
}

void patternview_update_font(PatternView* self)
{	
	psy_ui_FontInfo fontinfo;	
	psy_ui_Font font;	
	
	assert(self);
	
	fontinfo = patternview_zoom_font_info(self, patternviewstate_zoom(
		&self->pvstate));
	psy_ui_font_init(&font, &fontinfo);
	psy_ui_component_set_font(&self->header.component, &font);
#ifdef PSYCLE_USE_TRACKERVIEW		
	psy_ui_component_set_font(&self->trackerview.component, &font);
	psy_ui_component_set_font(&self->trackerview.grid.component, &font);
	psy_ui_component_set_font(&self->defaultline.component, &font);
	psy_ui_component_set_font(&self->defaultline.grid.component, &font);
	trackerstate_update_textmetric(&self->state, &font);		
	trackerstate_update_textmetric(&self->defaultline.state, &font);		
#endif		
#ifdef PSYCLE_USE_PIANOROLL
	psy_ui_component_set_font(&self->pianoroll.component, &font);
	keyboardstate_update_metrics(&self->pianoroll.keyboardstate,
		psy_ui_component_textmetric(&self->pianoroll.component));
#endif		
	psy_ui_font_dispose(&font);
	patternview_update_scroll_step(self);
	if (psy_ui_component_draw_visible(&self->component)) {
		psy_ui_component_align_full(&self->component);
		psy_ui_component_invalidate(&self->component);
	}
}

psy_ui_FontInfo patternview_zoom_font_info(PatternView* self, double zoom)
{
	psy_ui_FontInfo fontinfo;

	assert(self);

	psy_ui_fontinfo_init_string(&fontinfo,
		psy_configuration_value_str(self->pat_cfg, "font", "tahoma; 16"));
	fontinfo.lfHeight = (int32_t)((double)fontinfo.lfHeight * zoom);
	return fontinfo;
}

void patternview_update_scroll_step(PatternView* self)
{
	psy_ui_Value step;

	assert(self);

#ifdef PSYCLE_USE_TRACKERVIEW
	step = psy_ui_value_make_px(trackerstate_trackwidth(&self->state,
		psy_INDEX_INVALID, psy_ui_component_textmetric(
			&self->trackerview.grid.component)));	
	psy_ui_component_set_scroll_step_width(trackergrid_base(
		&self->trackerview.grid), step);
	psy_ui_component_set_scroll_step_width(trackergrid_base(
		&self->defaultline.grid), step);
	psy_ui_component_set_scroll_step_width(trackerheader_base(
		&self->header.header), step);
	psy_ui_component_set_scroll_step_height(trackergrid_base(
		&self->trackerview.grid), self->state.line_height);	
#endif		
}

uintptr_t patternview_section(const PatternView* self)
{
	assert(self);
	
	switch (self->display) {
	case PATTERN_DISPLAYMODE_TRACKER:
		return SECTION_ID_PATTERNVIEW_TRACKER;		
	case PATTERN_DISPLAYMODE_PIANOROLL:
		return SECTION_ID_PATTERNVIEW_PIANO;
	case PATTERN_DISPLAYMODE_TRACKER_PIANOROLL_VERTICAL:
		return SECTION_ID_PATTERNVIEW_VSPLIT;
	case PATTERN_DISPLAYMODE_TRACKER_PIANOROLL_HORIZONTAL:
		return SECTION_ID_PATTERNVIEW_HSPLIT;
	default:
		return SECTION_ID_PATTERNVIEW_TRACKER;
	}	
}

void patternview_select_section(PatternView* self, psy_ui_Component* sender,
	uintptr_t section, uintptr_t options)
{	
	assert(self);
	
	if (section == psy_INDEX_INVALID) {
		return;
	}	
	switch (section) {
	case SECTION_ID_PATTERNVIEW_TRACKER:		
		patternview_set_display_config(self,
			PATTERN_DISPLAYMODE_TRACKER);
		break;
	case SECTION_ID_PATTERNVIEW_PIANO:		
		patternview_set_display_config(self,
			PATTERN_DISPLAYMODE_PIANOROLL);
		break;
	case SECTION_ID_PATTERNVIEW_VSPLIT:
		patternview_set_display_config(self,
			PATTERN_DISPLAYMODE_TRACKER_PIANOROLL_VERTICAL);
		break;
	case SECTION_ID_PATTERNVIEW_HSPLIT:
		patternview_set_display_config(self,
			PATTERN_DISPLAYMODE_TRACKER_PIANOROLL_HORIZONTAL);
		break;
	default:
		break;
	}	
}

void patternview_set_display_config(PatternView* self, PatternDisplayMode display)
{
	psy_Property* p;

	assert(self);	

	p = psy_configuration_at(self->pat_cfg, "patterndisplay");
	if (p) {
		psy_property_set_item_int(p, display);
	}
}

bool patternview_on_edt_cmd(PatternView* self, InputHandler* sender)
{
	psy_EventDriverCmd cmd;
	
	assert(self);

	cmd = inputhandler_cmd(sender);	
	switch (cmd.id) {
	case CMD_BLOCKINTERPOLATECURVE:
		psy_ui_component_toggle_visibility(interpolatecurveview_base(
			&self->interpolatecurveview));
		psy_ui_component_toggle_visibility(psy_ui_splitter_base(
			&self->splitter));
		psy_ui_component_align(&self->component);
		psy_ui_component_invalidate(&self->component);
		return 1;
	case CMD_BLOCKTRANSFORM:		
		psy_ui_component_toggle_visibility(transformpatternview_base(
			&self->transformpattern));
		return 1;
	case CMD_BLOCKSWINGFILL:
		swingfillview_use_block(&self->swing_fill_);
		psy_ui_component_toggle_visibility(swingfillview_base(
			&self->swing_fill_));
		return 1;
	case CMD_BLOCKTRKSWINGFILL:
		swingfillview_use_track(&self->swing_fill_);
		psy_ui_component_toggle_visibility(swingfillview_base(
			&self->swing_fill_));
		return 1;
	case CMD_PATTERN_MENU:
		patternview_toggle_pattern_menu(self);
		return 1;
	default:		
		return 0;
	}
}

bool patternview_on_automate(PatternView* self, InputHandler* sender)
{	
	psy_EventDriverCmd cmd;
	psy_audio_Machine* machine;	
	ViewIndex view;
	
	assert(self);

	if (!workspace_song(self->workspace)) {
		return INPUTHANDLER_STOP;
	}	
	cmd = inputhandler_cmd(sender);
	machine = psy_audio_machines_at(
		psy_audio_song_machines(workspace_song(self->workspace)),
		cmd.id);
	view = workspace_current_view(self->workspace);
	if (view.id != VIEW_ID_PATTERNS) {
		return INPUTHANDLER_STOP;
	}
	if (!psy_audio_player_edit_mode(workspace_player(self->workspace))) {
		return INPUTHANDLER_STOP;
	}
	if (!psy_configuration_value_bool(psycleconfig_misc(
		workspace_cfg(self->workspace)), "record-tweak", TRUE)) {
		return INPUTHANDLER_STOP;
	}
	if (machine) {
		psy_audio_MachineParam* param;
		
		param = psy_audio_machine_parameter(machine, cmd.midi.byte0);
		if (param) {			
			psy_audio_Pattern* pattern;
			
			pattern = patternviewstate_pattern(&self->pvstate);
			if (pattern) {				
				intptr_t val;
				
				val = psy_audio_machine_parameter_pattern_value(machine, param);				
				psy_undoredo_execute(self->pvstate.undo_redo,
					&insertcommand_allocinit(pattern,
						self->pvstate.cursor,
						psy_audio_patternevent_make_tweak(cmd.id,
							cmd.midi.byte0, val, FALSE), psy_dsp_beatpos_zero(),						
						self->pvstate.sequence)->command);
			}			
		}
	}
	return INPUTHANDLER_STOP;
}

#endif /* PSYCLE_USE_PATTERN_VIEW */
