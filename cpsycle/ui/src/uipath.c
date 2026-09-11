/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uipath.h"
/* container */
#include <tokenizer.h>
/* std */
#include <stdlib.h>


/* psy_ui_PathCmd */

psy_ui_PathCmd* psy_ui_pathcmd_alloc(void)
{
	return (psy_ui_PathCmd*)malloc(sizeof(psy_ui_PathCmd));
}


/* psy_ui_Path */

/* prototypes */
static intptr_t psy_ui_path_parse_move(psy_ui_Path*, psy_Tokenizer*, bool rel);
static intptr_t psy_ui_path_parse_line(psy_ui_Path*, psy_Tokenizer*, bool rel);
static intptr_t psy_ui_path_parse_curve(psy_ui_Path*, psy_Tokenizer*, bool rel);
static intptr_t psy_ui_path_parse_point(psy_ui_Path*, psy_Tokenizer*, bool rel,
	psy_ui_RealPoint*);

/* implementation */
void psy_ui_path_init(psy_ui_Path* self)
{
	assert(self);
	
	self->elements = NULL;
	self->fill = psy_ui_colour_white();
	self->stroke = psy_ui_colour_white();
	self->cp = psy_ui_realpoint_zero();
}

void psy_ui_path_dispose(psy_ui_Path* self)
{
	assert(self);
	
	psy_list_deallocate(&self->elements, NULL);
}

psy_ui_Path* psy_ui_path_alloc(void)
{
	return (psy_ui_Path*)malloc(sizeof(psy_ui_Path));
}

psy_ui_Path* psy_ui_path_alloc_init(void)
{
	psy_ui_Path* rv;

	rv = psy_ui_path_alloc();
	if (rv) {
		psy_ui_path_init(rv);
	}
	return rv;
}

void psy_ui_path_copy(psy_ui_Path* self, const psy_ui_Path* other)
{
	psy_List* p;

	assert(self);

	psy_ui_path_clear(self);
	for (p = other->elements; p != NULL; p = p->next) {
		psy_ui_PathCmd* cmd;
	
		cmd = (psy_ui_PathCmd*)p->entry;
		if (cmd) {
			psy_ui_path_add(self, *cmd);
		} else {
			assert(cmd);
		}
	}
	self->fill = other->fill;
	self->stroke = other->stroke;
}

void psy_ui_path_clear(psy_ui_Path* self)
{
	assert(self);

	psy_list_deallocate(&self->elements, NULL);
}

psy_ui_Path* psy_ui_path_clone(const psy_ui_Path* src)
{
	psy_ui_Path* new_path;

	new_path = psy_ui_path_alloc_init();
	if (new_path) {
		psy_ui_path_copy(new_path, src);
	} else {
		assert(new_path);
	}
	return new_path;
}

void psy_ui_path_add(psy_ui_Path* self, psy_ui_PathCmd cmd)
{
	psy_ui_PathCmd* new_cmd;

	assert(self);

	new_cmd = psy_ui_pathcmd_alloc();
	if (new_cmd) {
		*new_cmd = cmd;
		psy_list_append(&self->elements, new_cmd);
	}
}

void psy_ui_path_move_to(psy_ui_Path* self, psy_ui_RealPoint pt)
{
	assert(self);

	psy_ui_path_add(self, psy_ui_pathcmd_make_move_to(pt));
	self->cp = pt;
}

void psy_ui_path_line_to(psy_ui_Path* self, psy_ui_RealPoint pt)
{
	assert(self);

	psy_ui_path_add(self, psy_ui_pathcmd_make_line_to(pt));
	self->cp = pt;
}

void psy_ui_path_close(psy_ui_Path* self)
{
	assert(self);

	psy_ui_path_add(self, psy_ui_pathcmd_make_close());
}

bool psy_ui_path_closed(const psy_ui_Path* self)
{
	const psy_ui_PathCmd* cmd;

	assert(self);

	if (!self->elements) {
		return FALSE;
	}
	cmd = (const psy_ui_PathCmd*)(psy_list_last(self->elements)->entry);
	return (cmd->type == psy_ui_PATHCMD_TYPE_CLOSE);
}

void psy_ui_path_set_stroke(psy_ui_Path* self, psy_ui_Colour colour)
{
	assert(self);

	self->stroke = colour;
}

void psy_ui_path_set_fill(psy_ui_Path* self, psy_ui_Colour colour)
{
	assert(self);

	self->fill = colour;
}

void psy_ui_path_curve_to(psy_ui_Path* self, psy_ui_RealPoint c1,
	psy_ui_RealPoint c2, psy_ui_RealPoint pt)
{
	assert(self);

	psy_ui_path_add(self, psy_ui_pathcmd_make_curve_to(c1, c2, pt));
}

void psy_ui_path_circle(psy_ui_Path* self, psy_ui_RealPoint pt, double r)
{
	assert(self);

	psy_ui_path_add(self, psy_ui_pathcmd_make_circle(pt, r));
}

intptr_t psy_ui_path_parse(psy_ui_Path* self, const char* str)
{		
	intptr_t status;
	psy_ui_RealPoint sp;
	bool first;
	psy_Tokenizer tokenizer;	

	assert(self);
	
	if (!str) {
		return PSY_OK;
	}
	sp = psy_ui_realpoint_zero();
	first = TRUE;
	psy_tokenizer_init(&tokenizer, str);
	psy_tokenizer_prevent_ident_numbers(&tokenizer);	
	status = PSY_OK;
	while (TRUE) {
		psy_tokenizer_next(&tokenizer);
		if (!psy_tokenizer_has_next(&tokenizer)) {
			break;
		}
		if (psy_tokenizer_check_ident(&tokenizer, "M")) {
			status = psy_ui_path_parse_move(self, &tokenizer, FALSE);
			if (status != PSY_OK) {				
				break;
			}
			if (first) {
				sp = self->cp;
			}
		} else if (psy_tokenizer_check_ident(&tokenizer, "m")) {
			status = psy_ui_path_parse_move(self, &tokenizer, TRUE);
			if (status != PSY_OK) {
				break;
			}			
			if (first) {
				sp = self->cp;
			}			
		} else if (psy_tokenizer_check_ident(&tokenizer, "L")) {
			status = psy_ui_path_parse_line(self, &tokenizer, FALSE);
			if (status != PSY_OK) {
				break;
			}						
		} else if (psy_tokenizer_check_ident(&tokenizer, "l")) {
			status = psy_ui_path_parse_line(self, &tokenizer, TRUE);
			if (status != PSY_OK) {
				break;
			}						
		} else if (psy_tokenizer_check_ident(&tokenizer, "V")) {
			psy_tokenizer_next(&tokenizer);
			if (psy_tokenizer_check_number(&tokenizer)) {
				double y;

				y = atof(psy_tokenizer_lexeme(&tokenizer));
				psy_ui_path_line_to(self, psy_ui_realpoint_make(self->cp.x, y));
			}
		} else if (psy_tokenizer_check_ident(&tokenizer, "v")) {
			psy_tokenizer_next(&tokenizer);
			if (psy_tokenizer_check_number(&tokenizer)) {
				double y;

				y = atof(psy_tokenizer_lexeme(&tokenizer));
				y += self->cp.y;
				psy_ui_path_line_to(self, psy_ui_realpoint_make(self->cp.x, y));
			}
		} else if (psy_tokenizer_check_ident(&tokenizer, "H")) {
			psy_tokenizer_next(&tokenizer);
			if (psy_tokenizer_check_number(&tokenizer)) {
				double x;

				x = atof(psy_tokenizer_lexeme(&tokenizer));
				psy_ui_path_line_to(self, psy_ui_realpoint_make(x, self->cp.y));
			}
		} else if (psy_tokenizer_check_ident(&tokenizer, "h")) {
			psy_tokenizer_next(&tokenizer);
			if (psy_tokenizer_check_number(&tokenizer)) {
				double x;

				x = atof(psy_tokenizer_lexeme(&tokenizer));
				x += self->cp.x;
				psy_ui_path_line_to(self, psy_ui_realpoint_make(x, self->cp.y));
			}
		} else if (psy_tokenizer_check_ident(&tokenizer, "C")) {
			status = psy_ui_path_parse_curve(self, &tokenizer, FALSE);
			if (status != PSY_OK) {
				break;
			}
		} else if (psy_tokenizer_check_ident(&tokenizer, "c")) {
			status = psy_ui_path_parse_curve(self, &tokenizer, TRUE);
			if (status != PSY_OK) {
				break;
			}
		} else if (psy_tokenizer_check_ident(&tokenizer, "Z")) {
			psy_ui_path_line_to(self, sp);			
			first = TRUE;
		}
	}
	psy_tokenizer_dispose(&tokenizer);
	return status;
}

intptr_t psy_ui_path_parse_move(psy_ui_Path* self, psy_Tokenizer* tokenizer,
	bool rel)
{
	intptr_t status;
	psy_ui_RealPoint pt;

	status = psy_ui_path_parse_point(self, tokenizer, rel, &pt);
	if (status != PSY_OK) {
		return status;
	}	
	psy_ui_path_move_to(self, pt);	
	return status;
}

intptr_t psy_ui_path_parse_line(psy_ui_Path* self, psy_Tokenizer* tokenizer,
	bool rel)
{
	intptr_t status;
	psy_ui_RealPoint pt;

	status = psy_ui_path_parse_point(self, tokenizer, rel, &pt);
	if (status != PSY_OK) {
		return status;
	}
	psy_ui_path_line_to(self, pt);		
	return status;
}

intptr_t psy_ui_path_parse_curve(psy_ui_Path* self, psy_Tokenizer* tokenizer,
	bool rel)
{	
	intptr_t status;
	psy_ui_RealPoint c1;
	psy_ui_RealPoint c2;
	psy_ui_RealPoint pt;

	status = psy_ui_path_parse_point(self, tokenizer, rel, &c1);
	if (status != PSY_OK) {
		return status;
	}		
	status = psy_ui_path_parse_point(self, tokenizer, rel, &c2);
	if (status != PSY_OK) {
		return PSY_ERRRUN;
	}	
	status = psy_ui_path_parse_point(self, tokenizer, rel, &pt);
	if (status != PSY_OK) {
		return PSY_ERRRUN;
	}	
	psy_ui_path_curve_to(self, c1, c2, pt);
	return status;
}

intptr_t psy_ui_path_parse_point(psy_ui_Path* self, psy_Tokenizer* tokenizer,
	bool rel, psy_ui_RealPoint* rv)
{	
	assert(self);
	assert(rv);

	psy_tokenizer_next(tokenizer);
	if (psy_tokenizer_check_number(tokenizer)) {
		rv->x = atof(psy_tokenizer_lexeme(tokenizer));
		psy_tokenizer_next(tokenizer);
		if (psy_tokenizer_check_number(tokenizer)) {
			rv->y = atof(psy_tokenizer_lexeme(tokenizer));
			if (rel) {
				psy_ui_realpoint_add(rv, self->cp);
			}
			return PSY_OK;
		}
	}
	return PSY_ERRRUN;	
}
