/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "mainstatusbar.h"
/* host */
#include "resources/resource.h"
#include "styles.h"
/* ui */
#include <uiapp.h>
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static void statuslabel_init_label(StatusLabel*);
static void statuslabel_init_progress_bar(StatusLabel*);
static void statuslabel_on_align(StatusLabel*);
static void statuslabel_on_output(StatusLabel*, const char* str);

/* vtable */
static psy_ui_ComponentVtable status_label_vtable;
static bool status_label_vtable_initialized = FALSE;

static void status_label_vtable_init(StatusLabel* self)
{
	assert(self);

	if (!status_label_vtable_initialized) {
		status_label_vtable = *(self->component.vtable);
		status_label_vtable.onalign =
			(psy_ui_fp_component)
			statuslabel_on_align;
		status_label_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(statuslabel_base(self),
		&status_label_vtable);
}

/* vtable */
static psy_LoggerVTable logger_vtable;
static bool logger_initialized = FALSE;

static void logger_vtable_init(StatusLabel* self)
{
	if (!logger_initialized) {
		logger_vtable = *(self->logger.vtable);
		logger_vtable.output =
			(fp_string_output)
			statuslabel_on_output;
		logger_vtable.warn =
			(fp_string_output)
			statuslabel_on_output;
		logger_vtable.error =
			(fp_string_output)
			statuslabel_on_output;
		logger_initialized = TRUE;
	}
	self->logger.vtable = &logger_vtable;
	self->logger.context_ = (void*)self;
}

void statuslabel_init(StatusLabel* self, psy_ui_Component* parent)
{
	assert(self);

	psy_ui_component_init(&self->component, parent, NULL);
	status_label_vtable_init(self);
	psy_logger_init(&self->logger);
	logger_vtable_init(self);
	statuslabel_init_progress_bar(self);
	statuslabel_init_label(self);
}

void statuslabel_init_label(StatusLabel* self)
{
	assert(self);

	psy_ui_label_init(&self->label_, &self->component);
	psy_ui_label_prevent_translation(&self->label_);
	psy_ui_label_set_text(&self->label_, "Ready");
	psy_ui_label_prevent_wrap(&self->label_);
}

void statuslabel_init_progress_bar(StatusLabel* self)
{
	assert(self);

	psy_ui_progressbar_init(&self->progress_bar_, &self->component);
}

void statuslabel_on_align(StatusLabel* self)
{
	psy_ui_Size size;
	
	assert(self);

	size = psy_ui_component_scroll_size(&self->component);
	psy_ui_component_resize(&self->progress_bar_.component, size);
	psy_ui_component_resize(psy_ui_label_base(&self->label_), size);
}

void statuslabel_on_output(StatusLabel* self, const char* str)
{
	assert(self);
	
	psy_ui_label_set_text(&self->label_, str);
	psy_ui_label_fadeout(&self->label_);
}

void statuslabel_set_default_text(StatusLabel* self, const char* text)
{
	assert(self);

	psy_ui_label_set_text(&self->label_, text);
	psy_ui_label_set_default_text(&self->label_, text);
}


/* MainStatusBar */

/* prototypes */
static void mainstatusbar_init_sizer(MainStatusBar*);
static void mainstatusbar_init_zoom(MainStatusBar*);
static void mainstatusbar_init_status_label(MainStatusBar*);
static void mainstatusbar_init_right(MainStatusBar*);
static void mainstatusbar_init_clock(MainStatusBar*);
static void mainstatusbar_init_kbd_help_button(MainStatusBar*);
static void mainstatusbar_init_terminal_button(MainStatusBar*);
static void mainstatusbar_init_cpu(MainStatusBar*);
static void mainstatusbar_init_midi(MainStatusBar*);
static void mainstatusbar_connect_progress_bar(MainStatusBar*);
static void mainstatusbar_update_terminal_button(MainStatusBar*);

/* implementation */
void mainstatusbar_init(MainStatusBar* self, psy_ui_Component* parent,
	Workspace* workspace)
{	
	assert(self);

	psy_ui_component_init(&self->component, parent, NULL);	
	self->workspace = workspace;
	psy_ui_component_set_style_type(&self->component, STYLE_STATUSBAR);	
	psy_ui_component_set_default_align(&self->component, psy_ui_ALIGN_LEFT,
		psy_ui_margin_make_em(0.0, 1.0, 0.0, 0.0));	
	mainstatusbar_init_zoom(self);	
	mainstatusbar_init_status_label(self);	
	mainstatusbar_init_right(self);
	mainstatusbar_init_terminal_button(self);
	mainstatusbar_init_kbd_help_button(self);
	mainstatusbar_init_midi(self);
	mainstatusbar_init_cpu(self);
	mainstatusbar_init_clock(self);	
	mainstatusbar_init_sizer(self);
	mainstatusbar_connect_progress_bar(self);	
}

void mainstatusbar_init_sizer(MainStatusBar* self)
{
	assert(self);

	psy_ui_sizer_init(&self->sizer_, &self->right);
	psy_ui_sizer_set_resize_component(&self->sizer_,
		psy_ui_app_main(psy_ui_app()));
}

void mainstatusbar_init_zoom(MainStatusBar* self)
{
	assert(self);

	zoombox_init(&self->zoom_, &self->component);
	self->zoom_.component.dbg = 11;
	zoombox_exchange(&self->zoom_, psy_configuration_at(psycleconfig_visual(
		workspace_cfg(self->workspace)), "zoom"));
	psy_ui_component_set_tab_index(zoombox_base(&self->zoom_), 0);
}

void mainstatusbar_init_status_label(MainStatusBar* self)
{
	assert(self);

	statuslabel_init(&self->status_label_, &self->component);
	self->status_label_.component.dbg = 12;
	psy_ui_component_set_align(statuslabel_base(&self->status_label_),
		psy_ui_ALIGN_CLIENT);
}

void mainstatusbar_init_right(MainStatusBar* self)
{
	assert(self);

	psy_ui_component_init_align(&self->right, &self->component, NULL,
		psy_ui_ALIGN_RIGHT);	
	psy_ui_component_set_default_align(&self->right, psy_ui_ALIGN_LEFT,
		psy_ui_margin_make_em(0.0, 1.0, 0.0, 0.0));
}

void mainstatusbar_init_clock(MainStatusBar* self)
{
	assert(self);
	
	clockbar_init(&self->clock_, &self->right);	
	clockbar_start(&self->clock_);
}

void mainstatusbar_init_kbd_help_button(MainStatusBar* self)
{
	assert(self);

	psy_ui_button_init_text_resource(&self->togglekbd, &self->right,
		"main.kbd", "img.keyboard");
	psy_ui_component_prevent_app_focus_out(psy_ui_button_base(
		&self->togglekbd));
	psy_ui_component_set_margin(psy_ui_button_base(&self->togglekbd),
		psy_ui_margin_make_em(0.0, 2.0, 0.0, 0.0));	
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->togglekbd), 0);
}

void mainstatusbar_init_terminal_button(MainStatusBar* self)
{		
	assert(self);

	psy_ui_button_init_text_resource(&self->terminal_, &self->right,
		"Terminal", "img.terminal");
	psy_ui_component_set_style_type(&self->terminal_.component,
		STYLE_TERM_BUTTON);	
	self->terminal_status = PSY_OK;	
	psy_ui_component_prevent_app_focus_out(psy_ui_button_base(
		&self->terminal_));
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->terminal_), 0);
}

void mainstatusbar_init_midi(MainStatusBar* self)
{
	assert(self);

	psy_ui_button_init_text(&self->midi, &self->right,
		"machinebar.midi");
	psy_ui_button_exchange(&self->midi,
		psy_configuration_at(
			psycleconfig_general(workspace_cfg(self->workspace)),
			"bench.showmidi"));
	psy_ui_component_set_margin(psy_ui_button_base(&self->midi),
		psy_ui_margin_make_em(0.0, 2.0, 0.0, 0.0));
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->midi), 0);
}

void mainstatusbar_init_cpu(MainStatusBar * self)
{
	assert(self);

	psy_ui_button_init_text(&self->cpu, &self->right, "machinebar.cpu");
	psy_ui_button_exchange(&self->cpu,
		psy_configuration_at(
			psycleconfig_general(workspace_cfg(self->workspace)),
			"bench.showcpu"));
	psy_ui_component_set_margin(psy_ui_button_base(&self->cpu),
		psy_ui_margin_make_em(0.0, 2.0, 0.0, 0.0));		
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->cpu), 0);
}

void mainstatusbar_connect_progress_bar(MainStatusBar* self)
{
	assert(self);
	
	workspace_set_progress_logger(self->workspace,
		&self->status_label_.progress_bar_.logger);	
}

void mainstatusbar_set_default_status_text(MainStatusBar* self,
	const char* text)
{
	assert(self);

	statuslabel_set_default_text(&self->status_label_, text);	
}

void mainstatusbar_idle(MainStatusBar* self)
{		
	assert(self);
	
	mainstatusbar_update_terminal_button(self);
}

void mainstatusbar_update_song(MainStatusBar* self)
{
	assert(self);

	clockbar_reset(&self->clock_);
}

void mainstatusbar_update_terminal_button(MainStatusBar* self)
{
	psy_Logger* logger;
	
	assert(self);

	logger = psy_ui_component_logger(&self->component, psy_ui_LOG_STATUS);
	if (logger) {		
		int status;
		uintptr_t style_id;
		
		status = psy_logger_status(logger);
		if (status != self->terminal_status) {
			self->terminal_status = status;
			switch (status) {
			case PSY_ERRRUN: style_id = STYLE_TERM_BUTTON; break;
			case PSY_WARN: style_id = STYLE_TERM_BUTTON_WARNING; break;
			default: style_id = STYLE_TERM_BUTTON; break;
			}
			psy_ui_component_set_style_type(psy_ui_button_base(
				&self->terminal_), style_id);
			psy_ui_component_invalidate(psy_ui_button_base(&self->terminal_));
		}
	}	
}
