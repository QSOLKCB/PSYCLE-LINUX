/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uieditor.h"

#define BLOCKSIZE 128 * 1024

/* file */
#include <filereader.h>
/* platform */
#include "../../detail/portable.h"


static void psy_ui_editor_set_font(psy_ui_Editor*, psy_ui_Font*);
static void psy_ui_editor_get_range(psy_ui_Editor*, intptr_t start,
	intptr_t end, char* text);

/* implementation */
void psy_ui_editor_init(psy_ui_Editor* self, psy_ui_Component* parent)
{  	
	psy_ui_component_init(&self->component, parent, NULL);	
	psy_ui_textarea_init(&self->text_area_, &self->component);
	psy_ui_component_set_style_type(&self->text_area_.pane.component,
		psy_ui_STYLE_EDITOR);
	psy_ui_component_set_style_type_hover(&self->text_area_.pane.component,
		psy_ui_STYLE_EDITOR);
	psy_ui_component_set_style_type_focus(&self->text_area_.pane.component,
		psy_ui_STYLE_EDITOR);
	psy_ui_textarea_prevent_wrap(&self->text_area_);
	psy_ui_textarea_line_wrap(&self->text_area_);
	psy_ui_component_set_align(&self->text_area_.component, psy_ui_ALIGN_CLIENT);
}

void psy_ui_editor_load(psy_ui_Editor* self, const char* path)
{	
	psy_FileReader file_reader;
	char* buffer;

	assert(self);

	psy_filereader_init(&file_reader);
	if (psy_filereader_load(&file_reader, path, &buffer) == PSY_OK) {
		psy_ui_textarea_set_text(&self->text_area_, buffer);
		psy_ui_component_align(&self->component);
		psy_ui_component_invalidate(psy_ui_textarea_base(&self->text_area_));
	}
	free(buffer);
	buffer = NULL;
	psy_filereader_dispose(&file_reader);
}

void psy_ui_editor_save(psy_ui_Editor* self, const char* path)
{
	FILE* fp;

	fp = fopen(path, "wb");
	if (fp) {
		char data[BLOCKSIZE + 1];
		intptr_t lengthdoc;
		intptr_t i;

		lengthdoc = psy_ui_textarea_length(&self->text_area_);
		for (i = 0; i < lengthdoc; i += BLOCKSIZE) {
			intptr_t grabsize;
			
			grabsize = lengthdoc - i;
			if (grabsize > BLOCKSIZE)
				grabsize = BLOCKSIZE;
			psy_ui_editor_get_range(self, i, i + grabsize, data);
			fwrite(data, grabsize, 1, fp);
		}
		fclose(fp);		
	}
}

void psy_ui_editor_get_range(psy_ui_Editor* self, intptr_t start, intptr_t end,
	char* text)
{
	psy_ui_textarea_range(&self->text_area_, start, end, text);
}

void psy_ui_editor_set_text(psy_ui_Editor* self, const char* text)
{
	psy_ui_textarea_set_text(&self->text_area_, text);	
}

void psy_ui_editor_add_text(psy_ui_Editor* self, const char* text)
{
	psy_ui_textarea_add_text(&self->text_area_, text);	
}

char* psy_ui_editor_text(psy_ui_Editor* self, uintptr_t maxlength, char* rv)
{
	const char* text;


	text = psy_ui_textarea_text(&self->text_area_);
	psy_snprintf(rv, maxlength, text);
	return rv;
}

uintptr_t psy_ui_editor_length(psy_ui_Editor* self)
{
	return psy_ui_textarea_length(&self->text_area_);	
}

void psy_ui_editor_clear(psy_ui_Editor* self)
{
	psy_ui_textarea_set_text(&self->text_area_, "");
}

void psy_ui_editor_prevent_edit(psy_ui_Editor* self)
{	
	psy_ui_textarea_prevent(&self->text_area_);	
}

void psy_ui_editor_enable_edit(psy_ui_Editor* self)
{
	psy_ui_textarea_enable(&self->text_area_);
}

void psy_ui_editor_wrap(psy_ui_Editor* self)
{
	psy_ui_textarea_word_wrap(&self->text_area_);	
}

void psy_ui_editor_prevent_wrap(psy_ui_Editor* self)
{
	psy_ui_textarea_prevent_wrap(&self->text_area_);	
}

void psy_ui_editor_gotoline(psy_ui_Editor* self, uintptr_t line)
{
	// sci(self, SCI_GOTOLINE, line, 0);
}

uintptr_t psy_ui_editor_cursor_line(const psy_ui_Editor* self)
{
	return psy_ui_textarea_cursor_line(&self->text_area_);
}
