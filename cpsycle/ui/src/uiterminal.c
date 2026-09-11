/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uiterminal.h"
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static void psy_ui_terminal_on_destroyed(psy_ui_Terminal*);
static void psy_ui_terminal_on_timer(psy_ui_Terminal*, uintptr_t timer_id);
static void psy_ui_terminal_on_output(psy_ui_Terminal*, const char* str);
static void psy_ui_terminal_on_output_warn(psy_ui_Terminal*, const char* str);
static void psy_ui_terminal_on_output_error(psy_ui_Terminal*, const char* str);
static int psy_ui_terminal_on_status(const psy_ui_Terminal*);

/* vtable */
static psy_ui_ComponentVtable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(psy_ui_Terminal* self)
{
	if (!vtable_initialized) {
		vtable = *(self->component.vtable);		
		vtable.on_destroyed =
			(psy_ui_fp_component)
			psy_ui_terminal_on_destroyed;
		vtable.on_timer =
			(psy_ui_fp_component_on_timer)
			psy_ui_terminal_on_timer;
		vtable_initialized = TRUE;
	}
	self->component.vtable = &vtable;
}

/* vtable */
static psy_LoggerVTable terminal_logger_vtable;
static bool terminal_logger_initialized = FALSE;

static void terminal_logger_vtable_init(psy_ui_Terminal* self)
{
	if (!terminal_logger_initialized) {
		terminal_logger_vtable = *(self->logger.vtable);
		terminal_logger_vtable.output =
			(fp_string_output)
			psy_ui_terminal_on_output;
		terminal_logger_vtable.warn =
			(fp_string_output)
			psy_ui_terminal_on_output_warn;
		terminal_logger_vtable.error =
			(fp_string_output)
			psy_ui_terminal_on_output_error;
		terminal_logger_vtable.status =
			(fp_logger_status)
			psy_ui_terminal_on_status;

		terminal_logger_initialized = TRUE;
	}
	self->logger.vtable = &terminal_logger_vtable;
	self->logger.context_ = (void*)self;
}

/* implementation */
void psy_ui_terminal_init(psy_ui_Terminal* self, psy_ui_Component* parent)
{			
	psy_ui_component_init(&self->component, parent, NULL);	
	vtable_init(self);
	psy_stringbuffer_init(&self->buffer_);
	self->last_status_ = PSY_OK;
	self->scroll_to_bottom_ = TRUE;
	psy_logger_init(&self->logger);
	terminal_logger_vtable_init(self);
	psy_ui_label_init(&self->output_, &self->component);
	psy_ui_label_enable_wrap(&self->output_);
	psy_ui_label_prevent_translation(&self->output_);
	psy_ui_label_set_text_alignment(&self->output_, psy_ui_ALIGNMENT_LEFT);
	psy_ui_component_set_scroll_step_height(psy_ui_label_base(&self->output_),
		psy_ui_value_make_eh(1.0));
	psy_ui_component_set_wheel_scroll(psy_ui_label_base(&self->output_), 4);
	psy_ui_component_set_align(psy_ui_label_base(&self->output_),
		psy_ui_ALIGN_FIXED);
	psy_ui_component_set_overflow(psy_ui_label_base(&self->output_),
		psy_ui_OVERFLOW_SCROLL);	
	psy_ui_scroller_init(&self->scroller_, &self->component, NULL, NULL);
	psy_ui_scroller_set_client(&self->scroller_, psy_ui_label_base(&self->output_));
	psy_ui_component_set_align(&self->scroller_.component, psy_ui_ALIGN_CLIENT);
	psy_ui_component_set_preferred_size(&self->component,
		psy_ui_size_make_em(20.0, 20.0));
	psy_ui_component_start_timer(&self->component, 0, 50);
}

void psy_ui_terminal_on_destroyed(psy_ui_Terminal* self)
{
	psy_stringbuffer_dispose(&self->buffer_);
}

void psy_ui_terminal_output(psy_ui_Terminal* self, const char* text)
{	
	assert(self);
	
	psy_stringbuffer_write(&self->buffer_, text);
}

void psy_ui_terminal_output_warn(psy_ui_Terminal* self, const char* text)
{	
	assert(self);

	self->last_status_ = PSY_WARN;
	psy_ui_terminal_output(self, text);
}

void psy_ui_terminal_output_error(psy_ui_Terminal* self, const char* text)
{	
	assert(self);

	self->last_status_ = PSY_ERRRUN;
	psy_ui_terminal_output(self, text);
}

void psy_ui_terminal_clear(psy_ui_Terminal* self)
{	
	assert(self);

	psy_ui_label_set_text(&self->output_, "");
}

void psy_ui_terminal_on_timer(psy_ui_Terminal* self, uintptr_t timer_id)
{
	assert(self);

	psy_ui_terminal_flush(self);
}

void psy_ui_terminal_flush(psy_ui_Terminal* self)
{
	const psy_StringBufferNode* p;

	assert(self);

	if (psy_stringbuffer_empty(&self->buffer_)) {
		return;
	}
	psy_stringbuffer_lock(&self->buffer_);
	for (p = psy_stringbuffer_begin(&self->buffer_); p != NULL; p = p->next) {
		psy_ui_label_add_text(&self->output_, psy_stringbuffernode_str(p));
	}
	psy_stringbuffer_clear(&self->buffer_);
	psy_stringbuffer_unlock(&self->buffer_);
	psy_ui_component_align(&self->scroller_.pane);
	if (self->scroll_to_bottom_) {
		psy_ui_Size size;

		size = psy_ui_component_scroll_size(self->scroller_.client);		
		psy_ui_component_set_scroll_top(self->scroller_.client,
			size.height);
	}
	psy_ui_component_invalidate(psy_ui_label_base(&self->output_));
}

void psy_ui_terminal_on_output(psy_ui_Terminal* self, const char* str)
{
	psy_ui_terminal_output(self, str);
}

void psy_ui_terminal_on_output_warn(psy_ui_Terminal* self, const char* str)
{
	psy_ui_terminal_output(self, str);
}

void psy_ui_terminal_on_output_error(psy_ui_Terminal* self, const char* str)
{
	psy_ui_terminal_output(self, str);
}

int psy_ui_terminal_on_status(const psy_ui_Terminal* self)
{
	return self->last_status_;
}
