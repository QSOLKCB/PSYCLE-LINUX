/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_ui_STYLE_H
#define psy_ui_STYLE_H

/* local */
#include "uibitmap.h"
#include "uicolour.h"
#include "uidef.h"
#include "uifont.h"
/* container */
#include <hashtbl.h>
#include <properties.h>

#ifdef __cplusplus
extern "C" {
#endif

/* psy_ui_ThemeMode */
typedef enum psy_ui_ThemeMode {
	psy_ui_LIGHTTHEME = 0,
	psy_ui_DARKTHEME  = 1	
} psy_ui_ThemeMode;

/* psy_ui_Display */
typedef enum psy_ui_Display {
	psy_ui_DISPLAY_NONE = 0
} psy_ui_Display;

/* psy_ui_BackgroundRepeatMode */
typedef enum psy_ui_BackgroundRepeatMode {
	psy_ui_NOREPEAT,
	psy_ui_REPEAT,	
} psy_ui_BackgroundRepeatMode;

typedef struct psy_ui_BackgroundRepeat {
	bool horizontal;
	bool vertical;
} psy_ui_BackgroundRepeat;

void psy_ui_backgroundrepeat_init(psy_ui_BackgroundRepeat*);

INLINE bool psy_ui_backgroundrepeat_full(const psy_ui_BackgroundRepeat* self)
{
	assert(self);

	return (self->horizontal != 0) && (self->vertical != 0);
}

INLINE bool psy_ui_backgroundrepeat_horizontal(const psy_ui_BackgroundRepeat* self)
{
	assert(self);

	return (self->horizontal != 0);
}

INLINE bool psy_ui_backgroundrepeat_vertical(const psy_ui_BackgroundRepeat* self)
{
	assert(self);

	return (self->vertical != 0);
}

psy_ui_BackgroundRepeat psy_ui_backgroundrepeat_make_all(
	psy_ui_BackgroundRepeatMode horizontal,
	psy_ui_BackgroundRepeatMode vertical);

/* psy_ui_Background */
typedef struct psy_ui_Background {
	psy_ui_Colour colour;
	uintptr_t image_id;
	char* image_path;
	psy_ui_Bitmap bitmap;
	psy_ui_BackgroundRepeat repeat;
	psy_ui_Alignment align;
	psy_ui_BitmapAnimate animation;	
	psy_ui_RealPoint position;
	bool position_set;
	psy_ui_Size size;
	bool size_set;
} psy_ui_Background;

void psy_ui_background_init(psy_ui_Background*);
void psy_ui_background_dispose(psy_ui_Background*);

void psy_ui_background_copy(psy_ui_Background*,
	const psy_ui_Background* other);

/*!
** @struct psy_ui_Style
** @brief Defines a style that can be assigned to a component
*/
typedef struct psy_ui_Style {
	char* name;	
	psy_ui_Font font;	
	psy_ui_Colour colour;	
	psy_ui_Background background;	
	psy_ui_Border border;
	psy_ui_Margin margin;
	bool margin_set;	
	psy_ui_Margin padding;
	bool padding_set;
	psy_ui_Position position;
	int dbgflag;
	psy_ui_Display display;
	bool display_set;
} psy_ui_Style;

void psy_ui_style_init(psy_ui_Style*);
void psy_ui_style_init_default(psy_ui_Style*, uintptr_t styletype);
void psy_ui_style_init_copy(psy_ui_Style*, const psy_ui_Style* other);
void psy_ui_style_init_colours(psy_ui_Style*, psy_ui_Colour foreground,
	psy_ui_Colour background);
void psy_ui_style_dispose(psy_ui_Style*);
void psy_ui_style_copy(psy_ui_Style*, const psy_ui_Style* other);

psy_ui_Style* psy_ui_style_alloc(void);
psy_ui_Style* psy_ui_style_alloc_init(void);
psy_ui_Style* psy_ui_style_clone(const psy_ui_Style* other);
void psy_ui_style_deallocate(psy_ui_Style*);

void psy_ui_style_set_name(psy_ui_Style* self, const char* name);

INLINE const char* psy_ui_style_name(const psy_ui_Style* self)
{
	if (self->name) {
		return self->name;
	}
	return "";
}

INLINE void psy_ui_style_set_background_colour(psy_ui_Style* self,
	psy_ui_Colour colour)
{
	self->background.colour = colour;
}

INLINE psy_ui_Colour psy_ui_style_background_colour(const psy_ui_Style* self)
{
	return self->background.colour;
}

INLINE void psy_ui_style_set_colour(psy_ui_Style* self, psy_ui_Colour colour)
{
	self->colour = colour;	
}

INLINE psy_ui_Colour psy_ui_style_colour(const psy_ui_Style* self)
{
	return self->colour;
}

INLINE void psy_ui_style_set_background_overlay(psy_ui_Style* self,
	int overlay)
{
	self->background.colour = psy_ui_colour_make_overlay(overlay);
}

void psy_ui_style_set_background_id(psy_ui_Style* self,
	uintptr_t id);

int psy_ui_style_setbackgroundpath(psy_ui_Style* self,
	const char* path);

INLINE void psy_ui_style_animate_background(psy_ui_Style* self,
	uintptr_t interval, psy_ui_RealSize framesize, bool horizontal)
{
	self->background.animation.enabled = TRUE;
	self->background.animation.interval = interval;
	self->background.animation.framesize = framesize;
	self->background.animation.horizontal = horizontal;
}

INLINE void psy_ui_style_set_colours(psy_ui_Style* self,
	psy_ui_Colour colour, psy_ui_Colour background)
{
	psy_ui_style_set_colour(self, colour);
	psy_ui_style_set_background_colour(self, background);	
}

void psy_ui_style_set_font(psy_ui_Style*, const char* family, double size);
void psy_ui_style_set_font_string(psy_ui_Style*, const char*);
void psy_ui_style_set_font_info(psy_ui_Style*, psy_ui_FontInfo);

INLINE void psy_ui_style_set_margin(psy_ui_Style* self, psy_ui_Margin margin)
{
	self->margin = margin;
	self->margin_set = TRUE;
}

INLINE psy_ui_Margin psy_ui_style_margin(const psy_ui_Style* self)
{
	return self->margin;	
}

INLINE void psy_ui_style_set_margin_em(psy_ui_Style* self,
	double top, double right, double bottom, double left)
{
	psy_ui_style_set_margin(self, psy_ui_margin_make_em(top, right, bottom,
		left));	
}

INLINE void psy_ui_style_set_padding(psy_ui_Style* self,
	psy_ui_Margin padding)
{
	self->padding = padding;
	self->padding_set = TRUE;
}

INLINE void psy_ui_style_set_padding_em(psy_ui_Style* self,
	double top, double right, double bottom, double left)
{
	psy_ui_style_set_padding(self, psy_ui_margin_make_em(top, right, bottom,
		left));	
}

INLINE void psy_ui_style_set_padding_px(psy_ui_Style* self,
	double top, double right, double bottom, double left)
{
	psy_ui_style_set_padding(self, psy_ui_margin_make_px(top, right, bottom,
		left));
}

INLINE psy_ui_Margin psy_ui_style_padding(const psy_ui_Style* self)
{
	return self->padding;
}

INLINE void psy_ui_style_set_position(psy_ui_Style* self,
	psy_ui_Rectangle position)
{
	psy_ui_position_set_rectangle(&self->position, position);
}

INLINE psy_ui_Rectangle psy_ui_style_position(const psy_ui_Style* self)
{
	return psy_ui_position_rectangle(&self->position);
}

INLINE void psy_ui_style_set_size(psy_ui_Style* self, psy_ui_Size size)
{
	psy_ui_position_set_size(&self->position, size);	
}

INLINE psy_ui_Size psy_ui_style_size(psy_ui_Style* self)
{
	return psy_ui_position_size(&self->position);	
}

INLINE void psy_ui_style_set_background_repeat(psy_ui_Style* self,
	psy_ui_BackgroundRepeat repeat)
{
	self->background.repeat = repeat;	
}

INLINE void psy_ui_style_set_background_size_px(psy_ui_Style* self,
	psy_ui_RealSize size)	
{
	self->background.size = psy_ui_size_make_px(size.width, size.height);
	self->background.size_set = TRUE;
}

INLINE void psy_ui_style_set_background_size(psy_ui_Style* self,
	psy_ui_Size size)	
{
	self->background.size = size;
	self->background.size_set = TRUE;
}

INLINE psy_ui_Size psy_ui_style_background_size(const psy_ui_Style* self)
{
	return self->background.size;
}

INLINE void psy_ui_style_set_background_position_px(psy_ui_Style* self,
	double x, double y)
{
	self->background.position = psy_ui_realpoint_make(x, y);
	self->background.position_set = TRUE;
}

INLINE void psy_ui_style_set_border(psy_ui_Style* self,
	const psy_ui_Border* border)
{
	assert(border);

	self->border = *border;
}

INLINE void psy_ui_style_set_display(psy_ui_Style* self, psy_ui_Display display)
{
	assert(self);

	self->display = display;
	self->display_set = TRUE;
}

INLINE void psy_ui_style_unset_display(psy_ui_Style* self)
{
	self->display_set = FALSE;
}

INLINE bool psy_ui_style_hidden(const psy_ui_Style* self)
{
	return self->display_set && (self->display == psy_ui_DISPLAY_NONE);
}

#ifdef __cplusplus
}
#endif

#endif /* psy_ui_STYLE_H */
