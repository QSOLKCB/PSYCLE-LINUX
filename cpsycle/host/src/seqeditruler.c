/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
**  copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "seqeditruler.h"

#ifdef PSYCLE_USE_SEQEDITOR

/* host */
#include "resources/resource.h"
#include "sequencetrackbox.h"
#include "styles.h"
/* audio */
#include <exclusivelock.h>
#include <sequencecmds.h>
/* ui */
#include <uiapp.h>
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static void seqeditruler_on_draw(SeqEditRuler*, psy_ui_Graphics*);
static void seqeditruler_update_baseline(SeqEditRuler*);
static void seqeditruler_draw_ruler(SeqEditRuler*, psy_ui_Graphics*);
static void seqeditruler_draw_cursor(const SeqEditRuler*, psy_ui_Graphics*,
	double currbeat, double step);
static void seqeditruler_draw_edit_position(SeqEditRuler*, psy_ui_Graphics*);
static void seqeditruler_on_preferred_size(SeqEditRuler*,
	const psy_ui_Size* limit, psy_ui_Size* rv);
static double seqeditruler_step(const SeqEditRuler*);
static psy_ui_Colour seqeditruler_step_colour(const SeqEditRuler*,
	intptr_t line);
static void seqeditruler_update_colours(SeqEditRuler*);

/* vtable */
static psy_ui_ComponentVtable seqeditruler_vtable;
static bool seqeditruler_vtable_initialized = FALSE;

static void seqeditruler_vtable_init(SeqEditRuler* self)
{
	if (!seqeditruler_vtable_initialized) {
		seqeditruler_vtable = *(self->component.vtable);		
		seqeditruler_vtable.ondraw =
			(psy_ui_fp_component_ondraw)
			seqeditruler_on_draw;
		seqeditruler_vtable.onpreferredsize =
			(psy_ui_fp_component_on_preferred_size)
			seqeditruler_on_preferred_size;		
		seqeditruler_vtable_initialized = TRUE;
	}
	self->component.vtable = &seqeditruler_vtable;
}

/* implementation */
void seqeditruler_init(SeqEditRuler* self, psy_ui_Component* parent,
	SeqEditState* state)
{
	assert(self);
	assert(state);	

	psy_ui_component_init(&self->component, parent, NULL);
	seqeditruler_vtable_init(self);		
	self->state = state;
	self->baseline = 12;
}

void seqeditruler_on_draw(SeqEditRuler* self, psy_ui_Graphics* g)
{
	assert(self);

	seqeditruler_update_colours(self);
	seqeditruler_update_baseline(self);
	seqeditruler_draw_ruler(self, g);	
	seqeditruler_draw_edit_position(self, g);
}

void seqeditruler_update_baseline(SeqEditRuler* self)
{
	const psy_ui_TextMetric* tm;
	psy_ui_RealSize size;	

	size = psy_ui_component_size_px(&self->component);
	tm = psy_ui_component_textmetric(&self->component);
	self->baseline = size.height - tm->tmHeight + 1;
}

void seqeditruler_draw_ruler(SeqEditRuler* self, psy_ui_Graphics* g)
{
	const psy_ui_TextMetric* tm;
	psy_ui_RealSize size;	
	psy_ui_RealRectangle clip_g;
	double textline;
	double linewidth;
	double duration;
	double clipstart;
	double clipend;
	double currbeat;
	double step;
	
	size = psy_ui_component_size_px(&self->component);
	tm = psy_ui_component_textmetric(&self->component);	
	textline = self->baseline - tm->tmHeight - 2;
	duration = (size.width + psy_ui_component_scroll_left_px(&self->component)) /
		(double)self->state->pxperbeat;
	/* psy_audio_sequence_duration(&workspace_song(
	 * self->workspace)->sequence); */
	linewidth = duration * self->state->pxperbeat;	
	psy_ui_graphics_draw_solid_rectangle(g, psy_ui_realrectangle_make(
		psy_ui_realpoint_zero(), psy_ui_realsize_make(size.width, self->baseline)),
		psy_ui_style_background_colour(psy_ui_style(STYLE_SEQEDT_RULER_TOP)));
	psy_ui_graphics_draw_solid_rectangle(g, psy_ui_realrectangle_make(
		psy_ui_realpoint_make(0.0, self->baseline),
		psy_ui_realsize_make(size.width, size.height - self->baseline)),
		psy_ui_style_background_colour(psy_ui_style(STYLE_SEQEDT_RULER_BOTTOM)));
	// psy_ui_graphics_set_colour(g, linecolour);
	// psy_ui_drawline(g, psy_ui_realpoint_make(0, baseline),
	//	psy_ui_realpoint_make(linewidth, baseline));
	clip_g = psy_ui_graphics_cliprect(g);
	clipstart = psy_dsp_beatpos_real(seqeditstate_pxtobeat(self->state, clip_g.left));
	clipend = psy_min(psy_dsp_beatpos_real(seqeditstate_pxtobeat(
		self->state, clip_g.right)), duration);
	step = seqeditruler_step(self);	
	clipstart = floor(clipstart / step) * step;
	for (currbeat = clipstart; currbeat <= clipend; currbeat += step) {
		double cpx;
		char txt[64];
		intptr_t line;
		psy_ui_Colour colour;
		double center;

		line = (intptr_t)(currbeat / step);
		cpx = seqeditstate_beat_to_px(self->state, psy_dsp_beatpos_make_real(
			currbeat, psy_dsp_DEFAULT_PPQ));
		colour = seqeditruler_step_colour(self, (intptr_t)(currbeat / step));		
		psy_ui_graphics_set_text_colour(g, colour);
		psy_ui_graphics_set_colour(g, colour);				
		psy_ui_drawline(g,
			psy_ui_realpoint_make(cpx, self->baseline ),
			psy_ui_realpoint_make(cpx, self->baseline + tm->tmHeight / 2));		
		seqeditruler_draw_cursor(self, g, currbeat, step);
		psy_snprintf(txt, 64, "%d", (int)(currbeat));
		if (currbeat == 0.0) {
			center = 0.0;
		} else {
			center = -((double)psy_strlen(txt) * tm->tmAveCharWidth / 2.0);
		}
		psy_ui_graphics_textout(g, psy_ui_realpoint_make(cpx + center,
			textline), txt, psy_strlen(txt));
	}
	if (self->state->drawcursor && self->state->cursoractive) {
		psy_ui_RealPoint cp;
		
		cp = psy_ui_realpoint_make(seqeditstate_beat_to_px(self->state,
			self->state->cursorposition), self->baseline);
		psy_ui_graphics_set_colour(g, psy_ui_style_colour(psy_ui_style(
			STYLE_SEQEDT_RULER_CURSOR)));
		psy_ui_drawline(g, cp, psy_ui_realpoint_make(cp.x, self->baseline +
			size.height - 1));
	}
}

void seqeditruler_draw_cursor(const SeqEditRuler* self, psy_ui_Graphics* g,
	double currbeat, double step)
{
	if (self->state->drawcursor && self->state->cursoractive) {
		double cursor;

		cursor = floor(psy_dsp_beatpos_real(self->state->cursorposition) /
			step) * step;
		if (currbeat >= cursor && currbeat <= cursor + step) {			
			if (cursor == currbeat &&
				psy_dsp_beatpos_real(self->state->cursorposition) -
					cursor < step / 2) {
				psy_ui_graphics_set_text_colour(g, self->linebeatcolour);
			}
			else if (cursor + step == currbeat &&
				psy_dsp_beatpos_real(self->state->cursorposition) -
				cursor > step / 2) {
				psy_ui_graphics_set_text_colour(g, self->linebeatcolour);
			}
			psy_ui_graphics_set_colour(g, self->linecolour);			
		}
	}
}

psy_ui_Colour seqeditruler_step_colour(const SeqEditRuler* self, intptr_t line)
{		
	
	if ((line % 8) == 0) {
		return self->linebeat4colour;
	} else if ((line % 4) == 0) {
		return self->linebeatcolour;
	}
	return self->linecolour;
}

void seqeditruler_draw_edit_position(SeqEditRuler* self, psy_ui_Graphics* g)
{
	psy_audio_SequenceEntry* seqentry;

	seqentry = psy_audio_sequence_entry(seqeditstate_sequence(self->state),
		psy_audio_sequenceselection_first(
			&self->state->workspace->song->sequence_.selection));
	if (seqentry) {		
		/*const psy_ui_Glyph* glyph;

		glyph = psy_ui_icons_at(&psy_ui_app()->icons, psy_ui_ICON_MORE);
		if (glyph) {			
			psy_ui_graphics_draw_glyph(g, glyph, 
				psy_ui_realpoint_make(
				seqeditstate_beat_to_px(self->state,
					psy_audio_sequenceentry_offset(seqentry)),
					self->baseline + 2),
				psy_ui_style_colour(psy_ui_style(STYLE_SEQEDT_RULER_CURSOR)));
		} */		
	}
}

double seqeditruler_step(const SeqEditRuler* self)
{
	double rv;
	
	rv = 80.0 / self->state->pxperbeat;
	if (rv > 16.0) {
		return floor((rv / 16) * 16.0);
	} else if (rv > 4.0) {
		return floor((rv / 4.0) * 4.0);
	} else if (rv > 2.0) {
		return floor((rv / 2.0) * 2.0);
	}
	return 1.0;
}

void seqeditruler_on_preferred_size(SeqEditRuler* self,
	const psy_ui_Size* limit, psy_ui_Size* rv)
{	
	if (seqeditstate_sequence(self->state)) {				
		rv->width = psy_ui_value_make_px(
			seqeditstate_beat_to_px(self->state, psy_dsp_beatpos_add(
				psy_audio_sequence_duration(seqeditstate_sequence(
					self->state)),
				psy_dsp_beatpos_make_real(400.0, psy_dsp_DEFAULT_PPQ))));
	} else {
		rv->width = psy_ui_value_make_px(0.0);
	}	
	rv->height = psy_ui_value_make_eh(2.0);	
}

void seqeditruler_update_edit_position(SeqEditRuler* self)
{
	psy_ui_component_invalidate(&self->component);
}

void seqeditruler_update_play_position(SeqEditRuler* self)
{
	psy_ui_component_invalidate(&self->component);
}

void seqeditruler_update_cursor_position(SeqEditRuler* self)
{
	psy_ui_component_invalidate(&self->component);
}

void seqeditruler_update_colours(SeqEditRuler* self)
{
	if (psy_ui_app_has_dark_theme(psy_ui_app())) {
		self->overlaycolour = psy_ui_colour_white();
	} else {
		self->overlaycolour = psy_ui_colour_black();
	}
	self->linecolour = psy_ui_style_colour(psy_ui_style(
		STYLE_SEQEDT_RULER_BOTTOM));
	if (psy_ui_app_has_dark_theme(psy_ui_app())) {
		self->overlaycolour = psy_ui_colour_white();
	} else {
		self->overlaycolour = psy_ui_colour_black();
	}
	self->linebeatcolour = psy_ui_colour_overlayed(&self->linecolour,
		&self->overlaycolour, 0.10);
	self->linebeat4colour = psy_ui_colour_overlayed(&self->linecolour,
		&self->overlaycolour, 0.20);
}

#endif /* PSYCLE_USE_SEQEDITOR */
