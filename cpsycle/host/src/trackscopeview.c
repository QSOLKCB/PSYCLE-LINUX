/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "trackscopeview.h"
/* host */
#include "styles.h"
/* ui */
#include <uiapp.h>
/* std */
#include <math.h>
/* platform */
#include "../../detail/portable.h"

/* prototypes */
static void trackscopes_on_draw(TrackScopes*, psy_ui_Graphics*);
static void trackscopes_draw_track(TrackScopes*, psy_ui_Graphics*,
	psy_ui_RealPoint, uintptr_t track);
static void trackscopes_draw_track_index(TrackScopes*, psy_ui_Graphics*,
	psy_ui_RealPoint, uintptr_t track);
static void trackscopes_on_mouse_down(TrackScopes*, psy_ui_MouseEvent*);
static void trackscopes_on_align(TrackScopes*);
static void trackscopes_on_preferred_size(TrackScopes*, psy_ui_Size* limit,
	psy_ui_Size* rv);
static uintptr_t trackscopes_num_rows(const TrackScopes*);
uintptr_t trackscopes_num_columns(const TrackScopes*);

/* vtable */
static psy_ui_ComponentVtable vtable;
static bool vtable_initialized = FALSE;

static psy_ui_ComponentVtable* vtable_init(TrackScopes* self)
{
	if (!vtable_initialized) {
		vtable = *(self->component.vtable);		
		vtable.onalign =
			(psy_ui_fp_component)
			trackscopes_on_align;
		vtable.onpreferredsize =
			(psy_ui_fp_component_on_preferred_size)
			trackscopes_on_preferred_size;
		vtable.ondraw =
			(psy_ui_fp_component_ondraw)
			trackscopes_on_draw;
		vtable.on_mouse_down =
			(psy_ui_fp_component_on_mouse_event)
			trackscopes_on_mouse_down;
		vtable_initialized = TRUE;
	}
	return &vtable;
}

/* implementation */
void trackscopes_init(TrackScopes* self, psy_ui_Component* parent,
	Workspace* workspace)
{	
	psy_ui_component_init(&self->component, parent, NULL);	
	psy_ui_component_set_vtable(&self->component, vtable_init(self));
	psy_ui_component_set_style_type(&self->component, STYLE_TRACKSCOPE);
	self->workspace = workspace;
	self->trackwidth = 90;
	self->trackheight = 30;
	self->textheight = 12;
	self->maxcolumns = 16;
	self->running = TRUE;	
}

void trackscopes_on_draw(TrackScopes* self, psy_ui_Graphics* g)
{
	if (workspace_song(self->workspace)) {
		uintptr_t numtracks;
		uintptr_t c;
		intptr_t rows = 1;
		uintptr_t currtrack;
		psy_ui_RealPoint cp;		
								
		currtrack = 0;
		psy_ui_realpoint_init(&cp);
		numtracks = psy_audio_song_num_song_tracks(workspace_song(
			self->workspace));
		for (c = 0; c < numtracks; ++c) {
			trackscopes_draw_track_index(self, g, cp, c);
			if (!psy_audio_trackstate_istrackmuted(
					&workspace_song(self->workspace)->patterns_.trackstate, c)) {
				trackscopes_draw_track(self, g, cp, c);
			}
			if (currtrack < self->maxcolumns - 1) {
				++currtrack;
				cp.x += self->trackwidth;
			} else {
				currtrack = 0;
				cp.x = 0;
				cp.y += self->trackheight;			
			}
		}
	}
}

void trackscopes_draw_track_index(TrackScopes* self, psy_ui_Graphics* g,
	psy_ui_RealPoint pt, uintptr_t track)
{
	char text[64];
		
	psy_snprintf(text, 64, "%X", (int)track);	
	psy_ui_graphics_textout(g, psy_ui_realpoint_make(pt.x + 3, pt.y + 2), text,
		strlen(text));
}

void trackscopes_draw_track(TrackScopes* self, psy_ui_Graphics* g,
	psy_ui_RealPoint pt, uintptr_t track)
{
	uintptr_t lastmachine;
	double width;
	double height;

	width = self->trackwidth;
	height = self->trackheight;
	if (psy_table_exists(&workspace_player(self->workspace)->sequencer.lastmachine,
			track)) {
		char text[40];

		lastmachine = (uintptr_t)
			psy_table_at(&workspace_player(self->workspace)->sequencer.lastmachine,
				track);
		psy_snprintf(text, 40, "%X", lastmachine);
		psy_ui_graphics_textout(g, psy_ui_realpoint_make(pt.x + width - 10,
			pt.y + height - self->textheight), text, strlen(text));
	} else {
		lastmachine = psy_INDEX_INVALID;
	}	
	if (workspace_song(self->workspace)) {
		psy_audio_Machine* machine;
		double centery;
		bool active = FALSE;
		
		centery = height / 2 + pt.y;
		machine = psy_audio_machines_at(
			&workspace_song(self->workspace)->machines_, lastmachine);
		if (machine) {
			psy_audio_Buffer* memory;
			
			memory = psy_audio_machine_buffermemory(machine);
			if (memory) {
				uintptr_t numsamples;
				uintptr_t frame;
				double px;
				double py;
				double cpx = 0.0;
				int x1, y1, x2, y2;
				static double epsilon = 0.01;
				
				numsamples = psy_audio_machine_buffermemory_size(machine);
				numsamples = psy_min(numsamples, psy_audio_MAX_STREAM_SIZE);
				if (numsamples > 0) {
					bool zero;
					uintptr_t writepos;
					double rms;

					rms = psy_audio_buffer_rmsdisplay(memory);
					zero = rms < epsilon;
					if (!zero) {
						uintptr_t i;
						uintptr_t step;
						
						active = TRUE;
						step = 1;
						px = width / numsamples;
						py = height * psy_audio_buffer_range_factor(
							memory, PSY_DSP_AMP_RANGE_VST) / 3.0;						
						writepos = memory->writepos;
						if (writepos >= numsamples) {
							frame = numsamples - writepos;
						} else {
							frame = writepos - numsamples;
						}
						frame = psy_min(frame, numsamples - 1);						
						x1 = x2 = 0;
						y1 = y2 = (int) (memory->samples[0][frame] * py);
						for (i = 1; i < numsamples; i += step) {
							x1 = x2;
							x2 = (int) (i * px);
							if (x1 != x2) {
								y1 = y2;							
								y2 = (int) (memory->samples[0][frame] * py);
								if (y2 > height / 2 || y2 < -height / 2) {
									continue;
								}
								psy_ui_drawline(g,
									psy_ui_realpoint_make(
										pt.x + x1, centery + y1),
									psy_ui_realpoint_make(
										pt.x + x2, centery + y2));
							}
							++frame;
							if (frame >= numsamples) {
								frame = 0;
							}
						}
					}
				}
			}			
		}
		if (!active) {
			psy_ui_drawline(g, psy_ui_realpoint_make(pt.x, centery),
				psy_ui_realpoint_make(pt.x + width, centery));
		}
	}	
}

void trackscopes_on_align(TrackScopes* self)
{
	const psy_ui_TextMetric* tm;
	psy_ui_Size size;	
	
	tm = psy_ui_component_textmetric(&self->component);
	size = psy_ui_component_scroll_size(&self->component);	
	self->trackheight = (int)(tm->tmHeight * 2.75);
	self->textheight = tm->tmHeight;
	self->maxcolumns = trackscopes_num_columns(self);	
	self->trackwidth = psy_ui_value_px(&size.width, tm, NULL) /
		self->maxcolumns;
}

void trackscopes_on_preferred_size(TrackScopes* self, psy_ui_Size* limit,
	psy_ui_Size* rv)
{			
	
	rv->width = psy_ui_value_make_ew(2 * 30);
	rv->height = psy_ui_value_make_eh(trackscopes_num_rows(self) * 2.75);
}

uintptr_t trackscopes_num_rows(const TrackScopes* self)
{		 
	if (workspace_song_const(self->workspace)) {		
		return (uintptr_t)ceil(
			(double)psy_audio_song_num_song_tracks(workspace_song_const(
				self->workspace)) /
			(double)trackscopes_num_columns(self));
	}
	return 1;
}

uintptr_t trackscopes_num_columns(const TrackScopes* self)
{	
	if (workspace_song(self->workspace)) {
		uintptr_t numtracks;

		numtracks = psy_audio_song_num_song_tracks(workspace_song(
			self->workspace));
		if (numtracks > 32) {
			return 32;
		}		
	}
	return 16;
}

void trackscopes_on_mouse_down(TrackScopes* self, psy_ui_MouseEvent* ev)
{
	uintptr_t track;				
	uintptr_t row;
	psy_audio_Patterns* patterns;
		
	if (!workspace_song(self->workspace)) {
		return;
	}								
	patterns = &workspace_song(self->workspace)->patterns_;
	row = (uintptr_t)(psy_ui_mouseevent_pt(ev).y / self->trackheight);
	track = (row * self->maxcolumns) +
		((uintptr_t)(psy_ui_mouseevent_pt(ev).x / self->trackwidth));
	if (psy_ui_mouseevent_button(ev) == 1) {
		if (!psy_audio_trackstate_istrackmuted(&patterns->trackstate, track)) {
			psy_audio_trackstate_mutetrack(&patterns->trackstate, track);
		} else {
			psy_audio_trackstate_unmutetrack(&patterns->trackstate, track);
		}
	} else if (psy_ui_mouseevent_button(ev) == 2) {
		if (psy_audio_patterns_is_track_soloed(patterns, track)) {
			psy_audio_patterns_deactivate_solo_track(patterns);
		} else {
			psy_audio_patterns_activate_solo_track(patterns, track);
		}
		psy_ui_component_invalidate(&self->component);
	}
}

void trackscopes_start(TrackScopes* self)
{
	self->running = TRUE;
}

void trackscopes_stop(TrackScopes* self)
{
	self->running = FALSE;	
}

void trackscopes_idle(TrackScopes* self)
{
	if (self->running) {
#ifndef PSYCLE_DEBUG_PREVENT_TIMER_DRAW
		psy_ui_component_invalidate(&self->component);		
#endif
	}
}

/* TrackScopeView */
void trackscopeview_init(TrackScopeView* self, psy_ui_Component* parent,
	Workspace* workspace)
{
	psy_ui_component_init(&self->component, parent, NULL);	
	psy_ui_component_set_tab_index(&self->component, 0);
	psy_ui_component_set_style_type_focus(&self->component, psy_ui_STYLE_FOCUS);
	psy_ui_component_set_style_type(&self->component, STYLE_TRACKSCOPES);
	trackscopes_init(&self->scopes, &self->component, workspace);
	psy_ui_component_set_align(&self->scopes.component, psy_ui_ALIGN_CLIENT);
}

void trackscopeview_idle(TrackScopeView* self)
{
	trackscopes_idle(&self->scopes);
}
