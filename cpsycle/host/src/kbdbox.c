/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "kbdbox.h"
/* host */
#include "styles.h"

static double keyheight = 3.0;
static double keybasewidth = 10.0;
static double keywidths[] = { 1.0, 1.5, 1.75, 2.27, 3.8, 0.25 };

void kbdboxstate_init(KbdBoxState* self, psy_audio_Player* player)
{
	assert(self);

	self->player = player;
	self->input = psy_audio_encodeinput(0, 0, 0, 0, 0);
	self->pressedkey = 0;
	kbdboxstate_clear_modifier(self);	
}

void kbdboxstate_clear_modifier(KbdBoxState* self)
{	
	assert(self);

	self->shift = FALSE;
	self->ctrl = FALSE;
	self->alt = FALSE;
}

/* KbdBoxKey */

/* prototypes */
static void kbdboxkey_init_label(KbdBoxKey*, psy_ui_Label*, const char* text);
static void kbdboxkey_init_style(KbdBoxKey*);
static void kbdboxkey_on_mouse_down(KbdBoxKey*, psy_ui_MouseEvent*);
static void kbdboxkey_on_mouse_up(KbdBoxKey*, psy_ui_MouseEvent*);
static void kbdboxkey_on_key_down(KbdBoxKey*, psy_ui_KeyboardEvent*);

/* vtable */
static psy_ui_ComponentVtable kbdboxkey_vtable;
static bool kbdboxkey_vtable_initialized = FALSE;

static void kbdboxkey_vtable_init(KbdBoxKey* self)
{
	if (!kbdboxkey_vtable_initialized) {
		kbdboxkey_vtable = *(self->component.vtable);		
		kbdboxkey_vtable.on_mouse_down =
			(psy_ui_fp_component_on_mouse_event)
			kbdboxkey_on_mouse_down;
		kbdboxkey_vtable.on_key_down =
			(psy_ui_fp_component_on_key_event)
			kbdboxkey_on_key_down;
		kbdboxkey_vtable.on_mouse_up =
			(psy_ui_fp_component_on_mouse_event)
			kbdboxkey_on_mouse_up;
		kbdboxkey_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(kbdboxkey_base(self),
		&kbdboxkey_vtable);
}

/* implementation */
void kbdboxkey_init_all(KbdBoxKey* self, psy_ui_Component* parent, 
	uintptr_t size, uint32_t keycode, const char* label, KbdBoxState* state)
{
	psy_ui_component_init(&self->component, parent, NULL);
	kbdboxkey_vtable_init(self);
	psy_ui_component_set_padding(&self->component,
		psy_ui_margin_make_px(2.0, 0.0, 0.0, 2.0));
	psy_ui_component_set_tab_index(&self->component, 0);	
	psy_ui_component_set_default_align(&self->component, psy_ui_ALIGN_TOP,
		psy_ui_margin_zero());	
	// psy_ui_component_prevent_app_focus_out(&self->component);
	self->keycode = keycode;
	self->state = state;	
	kbdboxkey_init_label(self, &self->label, label);
	kbdboxkey_init_label(self, &self->desc0, "");
	kbdboxkey_init_label(self, &self->desc1, "");
	kbdboxkey_init_label(self, &self->desc2, "");
	kbdboxkey_init_label(self, &self->desc3, "");
	if (size != 5) { /* not empty */
		kbdboxkey_init_style(self);
	}
	size = psy_max(0, psy_min(5, size));
	psy_ui_component_set_preferred_size(&self->component,
		psy_ui_size_make_em(keybasewidth * keywidths[size], keyheight));
}

KbdBoxKey* kbdboxkey_alloc_init_all(psy_ui_Component* parent,
	uintptr_t size, uint32_t keycode, const char* label,
	KbdBoxState* state)
{
	KbdBoxKey* rv;

	rv = (KbdBoxKey*)malloc(sizeof(KbdBoxKey));
	if (rv) {
		kbdboxkey_init_all(rv, parent, size, keycode, label, state);
		psy_ui_component_deallocate_after_destroyed(&rv->component);		
	}
	return rv;
}

void kbdboxkey_init_label(KbdBoxKey* self, psy_ui_Label* label,
	const char* text)
{
	psy_ui_label_init(label, &self->component);
	psy_ui_label_prevent_translation(label);
	psy_ui_label_set_char_number(label, 8.0);
	psy_ui_label_set_text_alignment(label, psy_ui_ALIGNMENT_LEFT);
	psy_ui_label_set_text(label, text);
	psy_ui_component_prevent_app_focus_out(psy_ui_label_base(label));	
}

void kbdboxkey_init_style(KbdBoxKey* self)
{	
	uintptr_t select_style_id;

	switch (self->keycode) {
	case  psy_ui_KEY_SHIFT:
		select_style_id = STYLE_KEY_SHIFT_SELECT;
		break;
	case  psy_ui_KEY_MENU:
		select_style_id = STYLE_KEY_ALT_SELECT;
		break;
	case  psy_ui_KEY_CONTROL:
		select_style_id = STYLE_KEY_CTRL_SELECT;
		break;
	default:
		select_style_id = STYLE_KEY_SELECT;
		break;
	}
	psy_ui_component_set_style_types(&self->component,
		STYLE_KEY, STYLE_KEY, select_style_id, psy_INDEX_INVALID);	
	psy_ui_component_set_style_type_focus(&self->component,
		psy_ui_STYLE_FOCUS);
}

void kbdboxkey_clear_descriptions(KbdBoxKey* self)
{
	psy_ui_label_set_text(&self->desc0, "");
	psy_ui_label_set_text(&self->desc1, "");
	psy_ui_label_set_text(&self->desc2, "");
	psy_ui_label_set_text(&self->desc3, "");	
}

void kbdboxkey_set_description(KbdBoxKey* self, uint32_t keycode,
	const char* text)
{		
	bool shift;
	bool ctrl;
	bool alt;
	bool up;
	
	psy_audio_decode_input((uint32_t)keycode, &keycode, &shift, &ctrl, &alt, &up);
	if (shift && ctrl) {
		psy_ui_label_set_text(&self->desc3, text);
	} else if (shift) {
		psy_ui_label_set_text(&self->desc1, text);
	} else  if (ctrl) {
		psy_ui_label_set_text(&self->desc2, text);
	} else {
		psy_ui_label_set_text(&self->desc0, text);
	}	
}

void kbdboxkey_on_mouse_down(KbdBoxKey* self, psy_ui_MouseEvent* ev)
{
	self->state->pressedkey = self->keycode;
	if (self->keycode == psy_ui_KEY_SHIFT) {
		self->state->shift = !self->state->shift;		
		if (self->state->shift) {
			psy_ui_component_add_style_state(&self->component,
				psy_ui_STYLESTATE_SELECT);			
		} else {
			psy_ui_component_remove_style_state(&self->component,
				psy_ui_STYLESTATE_SELECT);
		}		
		psy_ui_mouseevent_stop_propagation(ev);
	} else if (self->keycode == psy_ui_KEY_CONTROL) {
		self->state->ctrl = !self->state->ctrl;
		if (self->state->ctrl) {
			psy_ui_component_add_style_state(&self->component,
				psy_ui_STYLESTATE_SELECT);
		} else {
			psy_ui_component_remove_style_state(&self->component,
				psy_ui_STYLESTATE_SELECT);
		}
		psy_ui_mouseevent_stop_propagation(ev);
	} else if (self->keycode == psy_ui_KEY_MENU) {
		self->state->alt = !self->state->alt;
		if (self->state->alt) {
			psy_ui_component_add_style_state(&self->component,
				psy_ui_STYLESTATE_SELECT);
		} else {
			psy_ui_component_remove_style_state(&self->component,
				psy_ui_STYLESTATE_SELECT);
		}
		psy_ui_mouseevent_stop_propagation(ev);
	} else {
		psy_ui_component_add_style_state(&self->component,
			psy_ui_STYLESTATE_SELECT);		
	}
}

void kbdboxkey_on_mouse_up(KbdBoxKey* self, psy_ui_MouseEvent* ev)
{
	bool ismod;

	ismod = (self->state->pressedkey == psy_ui_KEY_SHIFT) ||
		(self->state->pressedkey == psy_ui_KEY_CONTROL) ||
		(self->state->pressedkey == psy_ui_KEY_MENU);
	if (self->state->pressedkey != 0 && !ismod) {
		psy_ui_component_remove_style_state(&self->component,
			psy_ui_STYLESTATE_SELECT);
	} else {
		psy_ui_mouseevent_stop_propagation(ev);
	}
}

void kbdboxkey_on_key_down(KbdBoxKey* self, psy_ui_KeyboardEvent* ev)
{
	assert(self);

	if (psy_ui_keyboardevent_keycode(ev) == psy_ui_KEY_RETURN) {
		self->state->pressedkey = self->keycode;
		if (self->keycode == psy_ui_KEY_SHIFT) {
			self->state->shift = !self->state->shift;
			if (self->state->shift) {
				psy_ui_component_add_style_state(&self->component,
					psy_ui_STYLESTATE_SELECT);
			}
			else {
				psy_ui_component_remove_style_state(&self->component,
					psy_ui_STYLESTATE_SELECT);
			}			
		}
		else if (self->keycode == psy_ui_KEY_CONTROL) {
			self->state->ctrl = !self->state->ctrl;
			if (self->state->ctrl) {
				psy_ui_component_add_style_state(&self->component,
					psy_ui_STYLESTATE_SELECT);
			}
			else {
				psy_ui_component_remove_style_state(&self->component,
					psy_ui_STYLESTATE_SELECT);
			}			
		}
		else if (self->keycode == psy_ui_KEY_MENU) {
			self->state->alt = !self->state->alt;
			if (self->state->alt) {
				psy_ui_component_add_style_state(&self->component,
					psy_ui_STYLESTATE_SELECT);
			}
			else {
				psy_ui_component_remove_style_state(&self->component,
					psy_ui_STYLESTATE_SELECT);
			}			
		}
		else {
			psy_ui_component_add_style_state(&self->component,
				psy_ui_STYLESTATE_SELECT);
		}
		{
			psy_EventDriverInput input;

			input.message = psy_EVENTDRIVER_PRESS;
			input.param1 = psy_audio_encodeinput(self->state->pressedkey,
				self->state->shift, self->state->ctrl, self->state->alt, 0);
			input.param2 = psy_audio_player_octave(self->state->player) * 12;			
			psy_eventdriver_write(psy_audio_player_kbd_driver(self->state->player),
				input);
			// kbdbox_reset_mod_states(self);
		}
		psy_ui_keyboardevent_stop_propagation(ev);
	}
}

/* KbdBox */

/* implementation */
static void kbdbox_on_destroyed(KbdBox*);
static void kbdbox_on_mouse_down(KbdBox*, psy_ui_MouseEvent*);
static void kbdbox_on_mouse_up(KbdBox*, psy_ui_MouseEvent*);
static void kbdbox_init_font(KbdBox*);
static void kbdbox_make_keys(KbdBox*);
static psy_Property* kbdbox_define_keys(KbdBox*);
static void kbdbox_define_key(KbdBox*, psy_Property* section,
	uintptr_t keycode, const char* label, int size, int cr);
static void kbdbox_add_row(KbdBox*);
static void kbdbox_add_key(KbdBox*, uint32_t keycode, uintptr_t size,
	const char* label);
static void kbdbox_reset_mod_states(KbdBox*);
static void kbdbox_on_input(KbdBox*, psy_EventDriver* sender);

/* vtable */
static psy_ui_ComponentVtable kbdbox_vtable;
static bool kbdbox_vtable_initialized = FALSE;

static void kbdbox_vtable_init(KbdBox* self)
{
	if (!kbdbox_vtable_initialized) {
		kbdbox_vtable = *(self->component.vtable);
		kbdbox_vtable.on_destroyed =
			(psy_ui_fp_component)
			kbdbox_on_destroyed;
		kbdbox_vtable.on_mouse_down =
			(psy_ui_fp_component_on_mouse_event)
			kbdbox_on_mouse_down;
		kbdbox_vtable.on_mouse_up =
			(psy_ui_fp_component_on_mouse_event)
			kbdbox_on_mouse_up;
		kbdbox_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &kbdbox_vtable);
}

/* implementation */
void kbdbox_init(KbdBox* self, psy_ui_Component* parent, Workspace* workspace)
{	
	assert(self);
	assert(workspace);

	psy_ui_component_init(kbdbox_base(self), parent, NULL);
	kbdbox_vtable_init(self);
	psy_ui_component_set_default_align(kbdbox_base(self), psy_ui_ALIGN_TOP,
		psy_ui_margin_make_em(0.0, 0.0, 0.3, 0.0));
	// psy_ui_component_prevent_app_focus_out(&self->component);
	self->workspace = workspace;
	kbdboxstate_init(&self->state, workspace_player(workspace));	
	psy_table_init(&self->keys);	
	kbdbox_init_font(self);
	kbdbox_make_keys(self);	
	psy_signal_connect(&self->workspace->player_.eventdrivers.signal_input,
		self, kbdbox_on_input);
}

void kbdbox_on_destroyed(KbdBox* self)
{
	psy_table_dispose(&self->keys);
}

void kbdbox_init_font(KbdBox* self)
{
	const psy_ui_Font* font;
	
	font = psy_ui_component_font(&self->component);
	if (font) {
		psy_ui_FontInfo fontinfo;		

		fontinfo = psy_ui_font_font_info(font);
		fontinfo.lfHeight = floor(fontinfo.lfHeight * 0.6);
		psy_ui_component_set_font_info(&self->component, fontinfo);		
	}
}

void kbdbox_make_keys(KbdBox* self)
{
	psy_Property* keyset;

	psy_ui_component_clear(&self->component);
	keyset = kbdbox_define_keys(self);	
	if (keyset) {		
		psy_List* p;
				
		kbdbox_add_row(self);
		for (p = psy_property_begin(keyset); p != NULL; p = p->next) {				
			psy_Property* property;
			intptr_t keycode;
			
			property = (psy_Property*)p->entry;				
			keycode = psy_property_at_int(property, "keycode", -1);
			if (keycode != -1) {				
				if (psy_property_at_int(property, "cr", 0)) {
					kbdbox_add_row(self);
				}				
				kbdbox_add_key(self, (uint32_t)keycode,
					(uintptr_t)psy_property_at_int(property, "size", 0),
					psy_property_at_str(property, "label", ""));
			}				
		}	
		psy_property_deallocate(keyset);
		keyset = NULL;
	}
}
	
psy_Property* kbdbox_define_keys(KbdBox* self)
{
	psy_Property* rv;	
	uintptr_t kc;	

	rv = psy_property_allocinit_key(NULL);	
	kc = 0x1000;
	kbdbox_define_key(self, rv, psy_ui_KEY_ESCAPE, "ESC", 1, 0);
	kbdbox_define_key(self, rv, kc++, "", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_F1, "F1", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_F2, "F2", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_F3, "F3", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_F4, "F4", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_F5, "F5", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_F6, "F6", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_F7, "F7", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_F8, "F8", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_F9, "F9", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_F10, "F10", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_F11, "F11", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_F12, "F12", 0, 0);
	kbdbox_define_key(self, rv, kc++, "", 5, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_INSERT, "INS", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_HOME, "HOME", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_PRIOR, "PGUP", 0, 0);
	
	kbdbox_define_key(self, rv, psy_ui_KEY_OEM_3, "`", 0, 1);
	kbdbox_define_key(self, rv, psy_ui_KEY_DIGIT1, "1", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_DIGIT2, "2", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_DIGIT3, "3", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_DIGIT4, "4", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_DIGIT5, "5", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_DIGIT6, "6", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_DIGIT7, "7", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_DIGIT8, "8", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_DIGIT9, "9", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_DIGIT0, "0", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_OEM_MINUS, "-", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_OEM_PLUS, "=", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_BACK, "BACK", 1, 0);
	kbdbox_define_key(self, rv, kc++, "", 5, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_DELETE, "DEL", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_END, "END", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_NEXT, "PGDWN", 0, 0);

	kbdbox_define_key(self, rv, psy_ui_KEY_TAB, "TAB", 1, 1);
	kbdbox_define_key(self, rv, psy_ui_KEY_Q, "Q", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_W, "W", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_E, "E", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_R, "R", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_T, "T", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_Y, "Y", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_U, "U", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_I, "I", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_O, "O", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_P, "P", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_OEM_4, "[", 0, 0);	
	kbdbox_define_key(self, rv, psy_ui_KEY_OEM_6, "]", 0, 0);
	
	kbdbox_define_key(self, rv, kc++, "CAPS", 2, 1);
	kbdbox_define_key(self, rv, psy_ui_KEY_A, "A", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_S, "S", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_D, "D", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_F, "F", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_G, "G", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_H, "H", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_J, "J", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_K, "K", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_L, "L", 0, 0); 
	kbdbox_define_key(self, rv, psy_ui_KEY_OEM_1, ".", 0, 0);	
	kbdbox_define_key(self, rv, psy_ui_KEY_OEM_7, "'", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_OEM_5, "'", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_RETURN, "ENTER", 2, 0);
	
	kbdbox_define_key(self, rv, psy_ui_KEY_SHIFT, "SHIFT", 3, 1);
	kbdbox_define_key(self, rv, psy_ui_KEY_Z, "Z", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_X, "X", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_C, "C", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_V, "V", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_B, "B", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_N, "N", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_M, "M", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_OEM_COMMA, ",", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_OEM_PERIOD, ".", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_OEM_2, "/", 0, 0);
	kbdbox_define_key(self, rv, kc++, "SHIFT", 3, 0);
	
	kbdbox_define_key(self, rv, psy_ui_KEY_CONTROL, "CTRL", 2, 1);
	kbdbox_define_key(self, rv, kc++, "", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_MENU, "ALT", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_SPACE, "Psycle", 4, 0);
	kbdbox_define_key(self, rv, kc++, "ALT", 1, 0);	
	kbdbox_define_key(self, rv, kc++, "CTRL", 1, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_LEFT, "Left", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_RIGHT, "Right", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_DOWN, "Down", 0, 0);
	kbdbox_define_key(self, rv, psy_ui_KEY_UP, "Up", 0, 0);
	return rv;
}

void kbdbox_define_key(KbdBox* self, psy_Property* section,
	uintptr_t keycode, const char* label, int size, int cr)
{
	psy_Property* key;

	key = psy_property_append_section(section, "key");
	psy_property_append_int(key, "keycode", keycode, 0, 0);	
	psy_property_append_str(key, "label", label);
	psy_property_append_int(key, "size", size, 0, 0);
	psy_property_append_int(key, "cr", cr, 0, 0);
}

void kbdbox_add_row(KbdBox* self)
{
	self->currrow = psy_ui_component_allocinit(&self->component, NULL);
	if (self->currrow) {
		psy_ui_component_prevent_app_focus_out(self->currrow);
		psy_ui_component_set_default_align(self->currrow, psy_ui_ALIGN_LEFT,
			psy_ui_margin_make_em(0.0, 0.3, 0.0, 0.0));
		psy_ui_component_set_align_expand(self->currrow, psy_ui_HEXPAND);
	}
}

void kbdbox_add_key(KbdBox* self, uint32_t keycode, uintptr_t size,
	const char* label)
{	
	if (self->currrow && !psy_table_exists(&self->keys, keycode)) {
		KbdBoxKey* key;	

		key = kbdboxkey_alloc_init_all(self->currrow, size, keycode, label,
			&self->state);
		if (key) {
			psy_table_insert(&self->keys, keycode, key);																		
		}
	}
}

void kbdbox_clear_descriptions(KbdBox* self)
{
	psy_TableIterator it;

	for (it = psy_table_begin(&self->keys);
			!psy_tableiterator_equal(&it, psy_table_end());
			psy_tableiterator_inc(&it)) {		
		kbdboxkey_clear_descriptions((KbdBoxKey*)psy_tableiterator_value(&it));
	}	
}

void kbdbox_set_description(KbdBox* self, uint32_t keycode, const char* text)
{
	KbdBoxKey* key;
	uint32_t keycode_decoded;
	bool shift;
	bool ctrl;
	bool alt;
	bool up;

	psy_audio_decode_input((uint32_t)keycode, &keycode_decoded, &shift, &ctrl, &alt, &up);
	key = (KbdBoxKey*)psy_table_at(&self->keys, keycode_decoded);
	if (key) {		
		kbdboxkey_set_description(key, keycode, text);		
	}
}

void kbdbox_on_mouse_down(KbdBox* self, psy_ui_MouseEvent* ev)
{
	if (self->state.pressedkey != 0) {
		psy_EventDriverInput input;

		input.message = psy_EVENTDRIVER_PRESS;
		input.param1 = psy_audio_encodeinput(self->state.pressedkey,
			self->state.shift, self->state.ctrl, self->state.alt, 0);
		input.param2 = psy_audio_player_octave(&self->workspace->player_) * 12;
		psy_eventdriver_write(workspace_kbd_driver(self->workspace), input);		
		kbdbox_reset_mod_states(self);
	}
}

void kbdbox_on_mouse_up(KbdBox* self, psy_ui_MouseEvent* ev)
{
	bool ismod;

	ismod = (self->state.pressedkey == psy_ui_KEY_SHIFT) ||
		(self->state.pressedkey == psy_ui_KEY_CONTROL) ||
		(self->state.pressedkey == psy_ui_KEY_MENU);
	if (self->state.pressedkey != 0 && !ismod) {
		psy_EventDriverInput input;

		input.message = psy_EVENTDRIVER_RELEASE;
		input.param1 = psy_audio_encodeinput(self->state.pressedkey,
			self->state.shift, self->state.ctrl, self->state.alt, TRUE);
		input.param2 = psy_audio_player_octave(&self->workspace->player_) * 12;
		psy_eventdriver_write(workspace_kbd_driver(self->workspace), input);		
		self->state.pressedkey = 0;
	}	
}

void kbdbox_reset_mod_states(KbdBox* self)
{	
	psy_TableIterator it;

	for (it = psy_table_begin(&self->keys);
			!psy_tableiterator_equal(&it, psy_table_end());
			psy_tableiterator_inc(&it)) {
		KbdBoxKey* key;

		key = (KbdBoxKey*)psy_tableiterator_value(&it);
		if (key->keycode == psy_ui_KEY_SHIFT || key->keycode == psy_ui_KEY_CONTROL
				|| key->keycode == psy_ui_KEY_MENU) {
			psy_ui_component_remove_style_state(kbdboxkey_base(key),
				psy_ui_STYLESTATE_SELECT);
		}
	}	
}

void kbdbox_on_input(KbdBox* self, psy_EventDriver* sender)
{
	const psy_EventDriverInfo* driver_info;

	assert(self);

	if (!psy_ui_component_draw_visible(&self->component)) {
		return;
	}
	driver_info = psy_eventdriver_info(sender);	
	if (driver_info && (driver_info->guid == PSY_EVENTDRIVER_KBD_GUID)) {
		psy_EventDriverInput input;

		input = psy_eventdriver_input(sender);
		if (input.message == psy_EVENTDRIVER_PRESS) {
			kbdbox_press_key(self, (uint32_t)input.param1);
		} else if (input.message == psy_EVENTDRIVER_RELEASE) {
			kbdbox_release_key(self, (uint32_t)input.param1);			
		}
	}
}

void kbdbox_press_key(KbdBox* self, uint32_t keycode)
{
	KbdBoxKey* key;
	uint32_t keycode_decoded;	
	bool shift;
	bool ctrl;
	bool alt;
	bool up;

	psy_audio_decode_input(keycode, &keycode_decoded, &shift, &ctrl, &alt, &up);
	key = (KbdBoxKey*)psy_table_at(&self->keys, keycode_decoded);
	if (key) {		
		psy_ui_component_add_style_state(&key->component,
			psy_ui_STYLESTATE_SELECT);
		if (shift) {
			self->state.shift = shift;
		}
		if (ctrl) {
			self->state.shift = ctrl;
		}
		if (alt) {
			self->state.alt = alt;
		}		
	}
}

void kbdbox_release_key(KbdBox* self, uint32_t keycode)
{
	KbdBoxKey* key;
	uint32_t keycode_decoded;
	bool shift;
	bool ctrl;
	bool alt;
	bool up;

	psy_audio_decode_input(keycode, &keycode_decoded, &shift, &ctrl, &alt, &up);
	key = (KbdBoxKey*)psy_table_at(&self->keys, keycode_decoded);
	if (key) {		
		psy_ui_component_remove_style_state(&key->component,
			psy_ui_STYLESTATE_SELECT);
		kbdboxstate_clear_modifier(&self->state);		
	}
}
