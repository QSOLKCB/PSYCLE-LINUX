/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "pianoruler.h"

#ifdef PSYCLE_USE_PIANOROLL

/* platform */
#include "../../detail/portable.h"
#include "../../detail/trace.h"

/* prototypes */
static void pianoruler_on_draw(PianoRuler*, psy_ui_Graphics*);
static psy_audio_BlockSelection pianoruler_clipselection(PianoRuler*,
	psy_ui_RealRectangle clip);
static void pianoruler_draw_ruler(PianoRuler*, psy_ui_Graphics*,
	psy_audio_BlockSelection);
static void pianoruler_draw_beat(PianoRuler*, psy_ui_Graphics*, intptr_t beat,
	double x, double baseline, const psy_ui_TextMetric*);
static void pianoruler_on_preferred_size(PianoRuler*, const psy_ui_Size* limit,
	psy_ui_Size* rv);
static psy_ui_Colour pianoruler_step_colour(const PianoRuler*, intptr_t line);
static void pianoruler_update_colours(PianoRuler*);

/* vtable */
static psy_ui_ComponentVtable pianoruler_vtable;
static bool pianoruler_vtable_initialized = FALSE;

static void pianoruler_vtable_init(PianoRuler* self)
{
	assert(self);

	if (!pianoruler_vtable_initialized) {
		pianoruler_vtable = *(self->component.vtable);
		pianoruler_vtable.ondraw =
			(psy_ui_fp_component_ondraw)
			pianoruler_on_draw;
		pianoruler_vtable.onpreferredsize =
			(psy_ui_fp_component_on_preferred_size)
			pianoruler_on_preferred_size;
		pianoruler_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(pianoruler_base(self), &pianoruler_vtable);
}

void pianoruler_init(PianoRuler* self, psy_ui_Component* parent,
	PianoGridState* gridstate)
{
	assert(self);
	assert(gridstate);

	psy_ui_component_init(&self->component, parent, NULL);	
	pianoruler_vtable_init(self);
	self->gridstate = gridstate;
}

void pianoruler_on_preferred_size(PianoRuler* self, const psy_ui_Size* limit,
	psy_ui_Size* rv)
{
	assert(self);

	rv->width = psy_ui_value_make_px(pianogridstate_beat_to_px(self->gridstate,
		patternviewstate_length(self->gridstate->pv)));		
	rv->height = psy_ui_value_make_eh(2.0);
}

void pianoruler_on_draw(PianoRuler* self, psy_ui_Graphics* g)
{	
	assert(self);

	pianoruler_update_colours(self);
	pianoruler_draw_ruler(self, g, pianoruler_clipselection(self,
		psy_ui_graphics_cliprect(g)));
}

psy_audio_BlockSelection pianoruler_clipselection(PianoRuler* self,
	psy_ui_RealRectangle clip)
{
	psy_audio_BlockSelection rv;

	assert(self);

	pianogridstate_clip(self->gridstate, clip.left, clip.right,
		&rv.topleft.offset, &rv.bottomright.offset);
	return rv;
}

void pianoruler_draw_ruler(PianoRuler* self, psy_ui_Graphics* g,
	psy_audio_BlockSelection clip)
{	
	const psy_ui_TextMetric* tm;
	psy_ui_RealSize size;
	double baselinetop;
	double baseline;
	intptr_t line;
	psy_dsp_beatpos_t c;
	double scrollleft;	
	psy_ui_Style* style;
	psy_ui_Colour overlaycolour;
	psy_ui_Colour linecolour;
	psy_ui_Colour linebeatcolour;
	psy_ui_Colour linebeat4colour;
	psy_ui_Colour top;
	psy_ui_Colour bottom;
	
	assert(self);

	if (psy_dsp_beatpos_is_zero(pianogridstate_step(self->gridstate))) {
		return;
	}
	tm = psy_ui_component_textmetric(&self->component);
	size = psy_ui_component_scroll_size_px(&self->component);	
	baseline = size.height - tm->tmHeight + 2;	
	baselinetop = baseline + tm->tmHeight / 3;
	scrollleft = psy_ui_component_scroll_left_px(&self->component);
	style = psy_ui_style(STYLE_PV_ROWBEAT);
	psy_ui_graphics_set_colour(g, psy_ui_style_background_colour(style));
	linecolour = psy_ui_style_colour(psy_ui_style(STYLE_PV_ROWBEAT));
	overlaycolour = psy_ui_colour_white();	
	linebeatcolour = psy_ui_colour_overlayed(&linecolour, &overlaycolour, 0.10);
	linebeat4colour = psy_ui_colour_overlayed(&linecolour, &overlaycolour, 0.20);
	
	top = bottom = psy_ui_style_background_colour(psy_ui_style(STYLE_PATTERNVIEW));
	// psy_ui_colour_add_rgb(&top, -5, -5, -5);
	// psy_ui_colour_add_rgb(&bottom, -10, -10, -10);
	psy_ui_graphics_draw_solid_rectangle(g, psy_ui_realrectangle_make(
		psy_ui_realpoint_zero(), psy_ui_realsize_make(size.width, baseline)),
		top);	
	psy_ui_graphics_draw_solid_rectangle(g, psy_ui_realrectangle_make(
		psy_ui_realpoint_make(0, baseline),
		psy_ui_realsize_make(size.width, size.height - baseline)),
		bottom);
	psy_ui_graphics_set_colour(g, linecolour);	
	// psy_ui_drawline(g, psy_ui_realpoint_make(scrollleft, baseline),
	//	psy_ui_realpoint_make(size.width + scrollleft, baseline));
	c = patternviewstate_draw_offset(self->gridstate->pv,
		clip.topleft.offset);
	line = pianogridstate_beat_to_line(self->gridstate, c);		
	psy_ui_graphics_set_background_mode(g, psy_ui_TRANSPARENT);	
	for (; psy_dsp_beatpos_less(c, clip.bottomright.offset);
			c = psy_dsp_beatpos_add(c, pianogridstate_step(self->gridstate)),
			++line) {
		double cpx;
		psy_ui_Colour colour;
				
		if (psy_dsp_beatpos_greater(c, clip.bottomright.offset)) {
			break;
		}
		cpx = pianogridstate_beat_to_px(self->gridstate, c);
		colour = pianoruler_step_colour(self, line);
		psy_ui_graphics_set_text_colour(g, colour);
		psy_ui_graphics_set_colour(g, colour);
		psy_ui_drawline(g,
			psy_ui_realpoint_make(cpx, baseline),
			psy_ui_realpoint_make(cpx, baseline + tm->tmHeight / 2));
		// psy_ui_drawline(g, psy_ui_realpoint_make(cpx, baseline),
		//	psy_ui_realpoint_make(cpx, baselinetop));
		if ((line % self->gridstate->pv->cursor.lpb) == 0) {
			pianoruler_draw_beat(self, g, (intptr_t)psy_dsp_beatpos_real(c), cpx,
				baseline, tm);
		}
	}
}

psy_ui_Colour pianoruler_step_colour(const PianoRuler* self, intptr_t line)
{

	if ((line % 16) == 0) {
		return self->linebeat4colour;
	}	
	return self->linecolour;
}

void pianoruler_draw_beat(PianoRuler* self, psy_ui_Graphics* g, intptr_t beat,
	double x, double baseline, const psy_ui_TextMetric* tm)
{	
	char txt[40];	
	double center;

	assert(self);
		
	psy_snprintf(txt, 40, "%d", beat);
	if (beat == 0.0) {
		center = 0.0;
	} else {
		center = -((double)psy_strlen(txt) * tm->tmAveCharWidth / 2.0);
	}
	psy_ui_graphics_textout(g, psy_ui_realpoint_make(x + center,
		baseline - tm->tmHeight), txt, psy_strlen(txt));	
}

void pianoruler_update_colours(PianoRuler* self)
{
	if (psy_ui_app_has_dark_theme(psy_ui_app())) {
		self->overlaycolour = psy_ui_colour_white();
	}
	else {
		self->overlaycolour = psy_ui_colour_black();
	}
	self->linecolour = psy_ui_style_colour(psy_ui_style(
		STYLE_SEQEDT_RULER_BOTTOM));
	if (psy_ui_app_has_dark_theme(psy_ui_app())) {
		self->overlaycolour = psy_ui_colour_white();
	}
	else {
		self->overlaycolour = psy_ui_colour_black();
	}
	self->linebeatcolour = psy_ui_colour_overlayed(&self->linecolour,
		&self->overlaycolour, 0.10);
	self->linebeat4colour = psy_ui_colour_overlayed(&self->linecolour,
		&self->overlaycolour, 0.20);
}

#endif /* PSYCLE_USE_PIANOROLL */
