/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "pianokeyboard.h"
/* platform */
#include "../../detail/portable.h"
#include "../../detail/trace.h"


void pianokeycolours_init(PianoKeyColours* self)
{
	assert(self);
	
	self->keyblack = psy_ui_colour_make(0x00595959);
	self->keywhite = psy_ui_colour_make(0x00C0C0C0);
	self->keyseparator = psy_ui_colour_make(0x999999);
	self->keyactive = psy_ui_colour_make(0x00808080);
	self->keywhiteselect = psy_ui_colour_make(0x00FF2288);
	self->keyblackselect = psy_ui_colour_weighted(self->keywhiteselect, 800);
}

/* PianoKeyboard */

/* prototypes */
static void pianokeyboard_on_draw(PianoKeyboard*, psy_ui_Graphics*);
static void pianokeyboard_on_mouse_down(PianoKeyboard*, psy_ui_MouseEvent*);
static void pianokeyboard_on_mouse_move(PianoKeyboard*, psy_ui_MouseEvent*);
static void pianokeyboard_on_mouse_up(PianoKeyboard*, psy_ui_MouseEvent*);
static void pianokeyboard_on_preferred_size(PianoKeyboard*,
	const psy_ui_Size* limit, psy_ui_Size* rv);
static void pianokeyboard_play(PianoKeyboard*, uint8_t key);
static uint8_t pianokeyboard_top_key(const PianoKeyboard*);
static void pianokeyboard_draw_text_vertical(PianoKeyboard*, psy_ui_Graphics*);
static void pianokeyboard_draw_white_keys_vertical(PianoKeyboard*,
	psy_ui_Graphics*);
static void pianokeyboard_draw_black_keys_vertical(PianoKeyboard*,
	psy_ui_Graphics*);
static void pianokeyboard_draw_white_keys_horizontal(PianoKeyboard*,
	psy_ui_Graphics*);
static void pianokeyboard_draw_black_keys_horizontal(PianoKeyboard*,
	psy_ui_Graphics*);
static psy_ui_Colour pianokeyboard_key_colour(PianoKeyboard*, uint8_t key);
static void pianokeyboard_on_align(PianoKeyboard*);
static uint8_t pianokeyboard_screen_to_key(PianoKeyboard*, psy_ui_RealPoint);

/* vtable */
static psy_ui_ComponentVtable pianokeyboard_vtable;
static bool pianokeyboard_vtable_initialized = FALSE;

static void pianokeyboard_vtable_init(PianoKeyboard* self)
{
	assert(self);

	if (!pianokeyboard_vtable_initialized) {
		pianokeyboard_vtable = *(self->component.vtable);
		pianokeyboard_vtable.ondraw =
			(psy_ui_fp_component_ondraw)
			pianokeyboard_on_draw;
		pianokeyboard_vtable.on_mouse_down =
			(psy_ui_fp_component_on_mouse_event)
			pianokeyboard_on_mouse_down;
		pianokeyboard_vtable.on_mouse_move =
			(psy_ui_fp_component_on_mouse_event)
			pianokeyboard_on_mouse_move;
		pianokeyboard_vtable.on_mouse_up =
			(psy_ui_fp_component_on_mouse_event)
			pianokeyboard_on_mouse_up;
		pianokeyboard_vtable.onalign =
			(psy_ui_fp_component)
			pianokeyboard_on_align;
		pianokeyboard_vtable.onpreferredsize =
			(psy_ui_fp_component_on_preferred_size)
			pianokeyboard_on_preferred_size;		
		pianokeyboard_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(pianokeyboard_base(self),
		&pianokeyboard_vtable);	
}

/* implementation */
void pianokeyboard_init(PianoKeyboard* self, psy_ui_Component* parent,
	KeyboardState* keyboardstate, psy_audio_Player* player)
{
	assert(self);
	assert(keyboardstate);	

	psy_ui_component_init(pianokeyboard_base(self), parent, NULL);	
	pianokeyboard_vtable_init(self);	
	self->keyboardstate = keyboardstate;
	self->player = player;	
	psy_ui_component_prevent_app_focus_out(pianokeyboard_base(self));
	pianokeycolours_init(&self->colours);
	if (self->keyboardstate->orientation == psy_ui_VERTICAL) {
		psy_ui_component_set_preferred_width(pianokeyboard_base(self),
			psy_ui_value_make_ew(10.0));
	} else {
		psy_ui_component_set_preferred_height(pianokeyboard_base(self),
			psy_ui_value_make_ew(8.0));
	}
}


void pianokeyboard_on_mouse_down(PianoKeyboard* self, psy_ui_MouseEvent* ev)
{
	assert(self);

	psy_ui_component_capture(&self->component);	
	pianokeyboard_play(self, pianokeyboard_screen_to_key(self,
		psy_ui_mouseevent_offset(ev)));			
	psy_ui_mouseevent_stop_propagation(ev);
}

uint8_t pianokeyboard_screen_to_key(PianoKeyboard* self, psy_ui_RealPoint pt)
{
	psy_ui_RealSize size;	

	assert(self);

	size = psy_ui_component_scroll_size_px(pianokeyboard_base(self));
	return keyboardstate_screen_to_key(self->keyboardstate, pt,
		(self->keyboardstate->orientation == psy_ui_VERTICAL)
		? size.width
		: size.height);
}

void pianokeyboard_on_mouse_move(PianoKeyboard* self, psy_ui_MouseEvent* ev)
{
	assert(self);

	psy_ui_mouseevent_stop_propagation(ev);	
}

uint8_t pianokeyboard_top_key(const PianoKeyboard* self)
{
	assert(self);

	return keyboardstate_screen_to_key(self->keyboardstate,
		self->keyboardstate->orientation == psy_ui_VERTICAL
		? psy_ui_realpoint_make(0.0, psy_ui_component_scroll_top_px(&self->component))
		: psy_ui_realpoint_make(psy_ui_component_scroll_left_px(&self->component), 0.0),
		0.0);
}

void pianokeyboard_on_mouse_up(PianoKeyboard* self, psy_ui_MouseEvent* ev)
{
	assert(self);

	psy_ui_component_release_capture(&self->component);	
	pianokeyboard_play(self, psy_audio_NOTECOMMANDS_RELEASE);
	memset(&self->keyboardstate->active_keys, 0, 255);
	psy_ui_component_stop_timer(&self->component, 0);	
	psy_ui_mouseevent_stop_propagation(ev);
}

void pianokeyboard_play(PianoKeyboard* self, uint8_t key)
{
	if (!self->player) {
		return;
	}
	if (!psy_audio_player_playing_key(self->player, key)) {
		psy_audio_PatternEvent ev;		
				
		ev = psy_audio_player_pattern_event(self->player, key);
		ev.note = key;
		self->keyboardstate->active_keys[key] = 1;
		psy_audio_player_play_event(self->player, &ev, 0);
		psy_ui_component_invalidate(&self->component);
	}	
}

void pianokeyboard_set_keyboard_type(PianoKeyboard* self, KeyboardType
	keyboardtype)
{
	double width_em;

	assert(self);

	width_em = 10.0;
	switch (keyboardtype) {
		case KEYBOARDTYPE_KEYS:
			self->keyboardstate->drawpianokeys = TRUE;
			self->keyboardstate->notemode = psy_dsp_NOTESTAB_A440;
			break;
		case KEYBOARDTYPE_NOTES:
			self->keyboardstate->drawpianokeys = FALSE;
			self->keyboardstate->notemode = psy_dsp_NOTESTAB_A440;
			break;
		case KEYBOARDTYPE_DRUMS:
			self->keyboardstate->drawpianokeys = FALSE;
			self->keyboardstate->notemode = psy_dsp_NOTESTAB_GMPERCUSSION;
			width_em = 21.0;			
			break;
		default:
			break;
	}
	psy_ui_component_set_preferred_width(pianokeyboard_base(self),
		psy_ui_value_make_ew(width_em));	
	psy_ui_component_invalidate(pianokeyboard_base(self));	
}

void pianokeyboard_on_preferred_size(PianoKeyboard* self, const psy_ui_Size* limit,
	psy_ui_Size* rv)
{
	*rv = psy_ui_size_make(psy_ui_value_make_ew(10.0),
		psy_ui_value_make_px(self->keyboardstate->keyboard_extent_px));
}

void pianokeyboard_on_draw(PianoKeyboard* self, psy_ui_Graphics* g)
{
	assert(self);

	if (self->keyboardstate->orientation == psy_ui_VERTICAL) {
		self->keyboardstate->key_extent_px = psy_ui_value_px(&self->keyboardstate->key_extent,
			psy_ui_component_textmetric(pianokeyboard_base(self)), NULL);
		self->keyboardstate->keyboard_extent_px = keyboardstate_extent(self->keyboardstate, 
			psy_ui_component_textmetric(pianokeyboard_base(self)));		
		if (self->keyboardstate->drawpianokeys) {
			pianokeyboard_draw_white_keys_vertical(self, g);
			pianokeyboard_draw_black_keys_vertical(self, g);
		}
		else {
			pianokeyboard_draw_text_vertical(self, g);
		}
	}
	else {
		pianokeyboard_draw_white_keys_horizontal(self, g);
		pianokeyboard_draw_black_keys_horizontal(self, g);
	}
}

void pianokeyboard_draw_text_vertical(PianoKeyboard* self, psy_ui_Graphics* g)
{
	uint8_t key;
	psy_ui_RealSize size;
	const psy_ui_TextMetric* tm;

	assert(self);

	size = psy_ui_component_scroll_size_px(pianokeyboard_base(self));
	tm = psy_ui_component_textmetric(pianokeyboard_base(self));
	self->keyboardstate->keyboard_extent_px = keyboardstate_extent(self->keyboardstate, tm);
	self->keyboardstate->key_extent_px = psy_ui_value_px(&self->keyboardstate->key_extent, tm, NULL);
	psy_ui_graphics_set_colour(g, self->colours.keyseparator);	
	psy_ui_graphics_set_text_colour(g, self->colours.keyseparator);	
	psy_ui_graphics_set_background_mode(g, psy_ui_TRANSPARENT);
	for (key = self->keyboardstate->keymin; key < self->keyboardstate->keymax; ++key) {
		double cp;

		cp = keyboardstate_key_to_px(self->keyboardstate, key);
		psy_ui_drawline(g, psy_ui_realpoint_make(0, cp),
			psy_ui_realpoint_make(size.width, cp));
		
		psy_ui_RealRectangle r;
		psy_ui_Colour restorebg;
		psy_ui_Colour bg;
		psy_ui_Component* curr;

		restorebg = psy_ui_component_background_colour(&self->component);
		bg = restorebg;
		curr = &self->component;
		while (curr && bg.mode.transparent) {
			curr = psy_ui_component_parent(curr);
			if (curr) {
				bg = psy_ui_component_background_colour(curr);
			}
		}
		r = psy_ui_realrectangle_make(psy_ui_realpoint_make(0.0, cp),
			psy_ui_realsize_make(size.width,
				self->keyboardstate->key_extent_px));
		if (psy_dsp_isblack(key) && psy_strlen(psy_dsp_notetostr(key,
			self->keyboardstate->notemode)) > 0) {
			psy_ui_colour_add_rgb(&bg, 5, 5, 5);
			psy_ui_set_background_colour(g, bg);
		}
		else {
			psy_ui_set_background_colour(g, bg);
		}
		psy_ui_graphics_textout_rectangle(g,
			psy_ui_realrectangle_topleft(&r),
			psy_ui_ETO_CLIPPED | psy_ui_ETO_OPAQUE, r,
			psy_dsp_notetostr(key, self->keyboardstate->notemode),
			psy_strlen(psy_dsp_notetostr(key, self->keyboardstate->notemode)));
		psy_ui_set_background_colour(g, restorebg);		
	}
}

void pianokeyboard_draw_white_keys_horizontal(PianoKeyboard* self,
	psy_ui_Graphics* g)
{
	psy_ui_RealSize size;	
	uint8_t key;
	double cp = 0;	

	assert(self);
	
	size = psy_ui_component_size_px(&self->component);
	psy_ui_graphics_set_colour(g, self->colours.keyseparator);
	psy_ui_graphics_set_background_mode(g, psy_ui_TRANSPARENT);
	psy_ui_graphics_set_text_colour(g, self->colours.keyseparator);
	for (key = self->keyboardstate->keymin; key < self->keyboardstate->keymax; ++key) {
		if (!psy_dsp_isblack(key)) {
			psy_ui_RealRectangle r;			

			r = psy_ui_realrectangle_make(
					psy_ui_realpoint_make(cp, 0),
					psy_ui_realsize_make(
						(self->keyboardstate->key_extent_px + 1),
						size.height));			
			psy_ui_graphics_draw_solid_rectangle(g, r,
				pianokeyboard_key_colour(self, key));
			psy_ui_drawline(g, psy_ui_realpoint_make(cp, 0),
				psy_ui_realpoint_make(cp, size.height));
			cp += self->keyboardstate->key_extent_px;			
		}
	}
}

void pianokeyboard_draw_white_keys_vertical(PianoKeyboard* self,
	psy_ui_Graphics* g)
{
	uint8_t key;
	psy_ui_RealSize size;	
	psy_ui_RealRectangle clip;
	
	assert(self);

	size = psy_ui_component_scroll_size_px(pianokeyboard_base(self));
	clip = psy_ui_graphics_cliprect(g);
	psy_ui_graphics_set_colour(g, self->colours.keyseparator);	
	psy_ui_graphics_set_text_colour(g, self->colours.keyseparator);	
	psy_ui_graphics_set_background_mode(g, psy_ui_TRANSPARENT);
	for (key = self->keyboardstate->keymin; key < self->keyboardstate->keymax; ++key) {
		double cp;

		cp = keyboardstate_key_to_px(self->keyboardstate, key);		
		if (cp + self->keyboardstate->keyboard_extent_px < clip.top) {
			break;
		}		
		if (cp > clip.bottom) {
			continue;
		}
		psy_ui_drawline(g, psy_ui_realpoint_make(0, cp),
			psy_ui_realpoint_make(size.width, cp));		
		if (psy_dsp_isblack(key)) {
			 psy_ui_graphics_draw_solid_rectangle(g, psy_ui_realrectangle_make(
				psy_ui_realpoint_make(size.width * 0.60, cp),
				psy_ui_realsize_make(size.width * 0.40,
					self->keyboardstate->key_extent_px)),
					self->colours.keywhite);
			psy_ui_drawline(g,
					psy_ui_realpoint_make(size.width * 0.60, cp +
						self->keyboardstate->key_extent_px / 2),
					psy_ui_realpoint_make(size.width, cp +
						self->keyboardstate->key_extent_px / 2));
		} else {
			psy_ui_RealRectangle r;
			const psy_ui_TextMetric* tm;

			tm = psy_ui_component_textmetric(pianokeyboard_base(self));
			r = psy_ui_realrectangle_make(psy_ui_realpoint_make(0.0, cp),
				psy_ui_realsize_make(size.width,
					self->keyboardstate->key_extent_px));								
			psy_ui_graphics_draw_solid_rectangle(g, r, pianokeyboard_key_colour(self, key));
			if (psy_dsp_iskey_c(key) || psy_dsp_iskey_e(key)) {
				psy_ui_drawline(g,
					psy_ui_realpoint_make(0, cp + self->keyboardstate->key_extent_px),
					psy_ui_realpoint_make(size.width, cp + self->keyboardstate->key_extent_px));
				if (psy_dsp_iskey_c(key)) {
					psy_ui_graphics_set_text_colour(g, self->colours.keyblack);
					psy_ui_graphics_textout_rectangle(g,
						psy_ui_realpoint_make(
							size.width - tm->tmAveCharWidth * 4,
							cp),
						psy_ui_ETO_CLIPPED, r,
						psy_dsp_notetostr(key, self->keyboardstate->notemode),
						psy_strlen(psy_dsp_notetostr(key, self->keyboardstate->notemode)));
					psy_ui_graphics_set_text_colour(g, self->colours.keyseparator);
				}
			}
		}
	}			
}


void pianokeyboard_draw_black_keys_horizontal(PianoKeyboard* self,
	psy_ui_Graphics* g)
{
	psy_ui_RealSize size;	
	int key;
	double cp;
	double top = 0.60;
	double bottom = 1 - top;	

	assert(self);
	
	size = psy_ui_component_size_px(&self->component);	
	cp = 0;
	
	for (key = self->keyboardstate->keymin; key < self->keyboardstate->keymax; ++key) {
		if (!psy_dsp_isblack(key)) {
			cp += self->keyboardstate->key_extent_px;
		} else {
			psy_ui_RealRectangle r;
			int x;
			int width;			

			x = (int)cp - (int)(self->keyboardstate->key_extent_px * 0.68 / 2);
			width = (int)(self->keyboardstate->key_extent_px * 0.68);
			if (self->keyboardstate->key_align == psy_ui_ALIGN_BOTTOM) {
				r = psy_ui_realrectangle_make(psy_ui_realpoint_make(x, 0),
					psy_ui_realsize_make(width, (int)(size.height * top)));
			} else {
				r = psy_ui_realrectangle_make(psy_ui_realpoint_make(x, 
						size.height * (1 - top)),
					psy_ui_realsize_make(width, (int)(size.height * top)));
			}
			psy_ui_graphics_draw_solid_rectangle(g, r,
				pianokeyboard_key_colour(self, key));
		}	
	}
}

void pianokeyboard_draw_black_keys_vertical(PianoKeyboard* self,
	psy_ui_Graphics* g)
{
	uint8_t key;
	psy_ui_RealSize key_size;
	double cp;
	psy_ui_RealRectangle clip;

	assert(self);

	key_size = psy_ui_component_scroll_size_px(pianokeyboard_base(self));
	key_size.width *= 0.6;
	key_size.height = self->keyboardstate->key_extent_px;	
	clip = psy_ui_graphics_cliprect(g);
	cp = keyboardstate_key_to_px(self->keyboardstate, self->keyboardstate->keymin);
	for (key = self->keyboardstate->keymin; key < self->keyboardstate->keymax; ++key) {
		if (cp < clip.bottom) {
			if (psy_dsp_isblack(key)) {
				psy_ui_graphics_draw_solid_rectangle(g,
					psy_ui_realrectangle_make(psy_ui_realpoint_make(0, cp), key_size),
					pianokeyboard_key_colour(self, key));
			}
			if (cp < clip.top) {
				break;
			}
		}
		cp -= self->keyboardstate->key_extent_px;
	}
}


psy_ui_Colour pianokeyboard_key_colour(PianoKeyboard* self, uint8_t key)
{
	assert(self);

	if (psy_audio_parameterrange_intersect(
			&self->keyboardstate->entry.keyrange, key)) {
		if (psy_dsp_isblack(key)) {
			return self->colours.keyblackselect;
		} else {
			return self->colours.keywhiteselect;
		}	
	} 
	if (self->player && self->keyboardstate->active_keys[key] != 0) {
		if (psy_dsp_isblack(key)) {
			return self->colours.keyactive;
		} else {
			return self->colours.keyactive;
		}
	}
	if (psy_dsp_isblack(key)) {
		return self->colours.keyblack;
	}	
	return self->colours.keywhite;
}

static int numwhitekey(int key)
{
	int octave = key / 12;
	int offset = key % 12;
	int c = 0;
	int i;

	for (i = 1; i <= offset; ++i) {
		if (!psy_dsp_isblack(i)) ++c;
	}
	return octave * 7 + c;
}

void pianokeyboard_on_align(PianoKeyboard* self)
{
	if (self->keyboardstate->align_keys) {		
		uint8_t key;
		uint8_t numwhitekeys;		
		psy_ui_IntSize size;

		numwhitekeys = 0;
		for (key = self->keyboardstate->keymin;
				key < self->keyboardstate->keymax; ++key) {
			if (!psy_dsp_isblack(key)) {
				++numwhitekeys;
			}
		}		
		size = psy_ui_intsize_init_size(
			psy_ui_component_scroll_size(&self->component), 
				psy_ui_component_textmetric(&self->component), NULL);
		self->keyboardstate->key_extent_px = size.width / (double)numwhitekeys;		
	}
}
