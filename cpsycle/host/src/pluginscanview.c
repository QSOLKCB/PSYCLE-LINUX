/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "pluginscanview.h"
/* host */
#include "resources/resource.h"
#include "styles.h"
/* platform */
#include "../../detail/portable.h"


/* PluginScanDescView */
void pluginscandescview_init(PluginScanDescView* self, psy_ui_Component* parent)
{
	assert(self);
	
	psy_ui_component_init(&self->component, parent, NULL);
	psy_ui_component_set_preferred_size(&self->component,
		psy_ui_size_make_em(40.0, 0.0));
}


/* PluginScanStatusView */

void pluginscanstatusview_init(PluginScanStatusView* self, psy_ui_Component* parent)
{
	assert(self);
	
	psy_ui_component_init(&self->component, parent, NULL);	
	psy_ui_component_set_preferred_size(&self->component,
		psy_ui_size_make_em(40.0, 0.0));
	psy_ui_label_init_text(&self->time_desc, &self->component,
		"Scan time");
	psy_ui_component_set_align(psy_ui_label_base(&self->time_desc),
		psy_ui_ALIGN_TOP);
	clockbar_init(&self->time, &self->component);
	clockbar_display_minutes(&self->time);
	psy_ui_component_set_align(clockbar_base(&self->time),
		psy_ui_ALIGN_TOP);
	psy_ui_label_init_text(&self->filenum_desc, &self->component,
		"Scanned files");
	psy_ui_component_set_margin(psy_ui_label_base(&self->filenum_desc),
		psy_ui_margin_make_em(1.0, 0.0, 0.0, 0.0));
	psy_ui_component_set_align(psy_ui_label_base(&self->filenum_desc),
		psy_ui_ALIGN_TOP);	
	psy_ui_label_init(&self->filenum, &self->component);
	psy_ui_component_set_align(psy_ui_label_base(&self->filenum),
		psy_ui_ALIGN_TOP);
	/* plugin number */
	psy_ui_label_init_text(&self->pluginnum_desc, &self->component,
		"Found plugins");
	psy_ui_component_set_margin(psy_ui_label_base(&self->pluginnum_desc),
		psy_ui_margin_make_em(1.0, 0.0, 0.0, 0.0));
	psy_ui_component_set_align(psy_ui_label_base(&self->pluginnum_desc),
		psy_ui_ALIGN_TOP);
	psy_ui_label_init(&self->pluginnum, &self->component);
	psy_ui_component_set_align(psy_ui_label_base(&self->pluginnum),
		psy_ui_ALIGN_TOP);
	pluginscanstatusview_reset(self);
}

void pluginscanstatusview_update(PluginScanStatusView* self,
	const psy_audio_PluginCatcher* catcher)
{	
	char text[80];
	
	assert(self);
		
	psy_snprintf(text, 40, "%ld", 
		(int)psy_audio_plugincatcher_file_count(catcher));
	psy_ui_label_set_text(&self->filenum, text);	
	psy_snprintf(text, 40, "%ld", (int)
		psy_audio_plugincatcher_plugin_count(catcher));	
	psy_ui_label_set_text(&self->pluginnum, text);
}

void pluginscanstatusview_reset(PluginScanStatusView* self)
{
	assert(self);
	
	psy_ui_label_set_text(&self->filenum, "0");
	psy_ui_label_set_text(&self->pluginnum, "0");
}

/* PluginScanTaskView */

/* prototypes */
static void pluginscantaskview_init_task_list(PluginScanTaskView*);

/* implementation */
void pluginscantaskview_init(PluginScanTaskView* self, psy_ui_Component* parent,
	psy_audio_PluginCatcher* plugincatcher)
{
	assert(self);
	
	psy_ui_component_init(&self->component, parent, NULL);
	psy_ui_component_set_style_type(&self->component, STYLE_SCANTASK);
	self->plugincatcher = plugincatcher;
	psy_ui_component_init(&self->tasks, &self->component, NULL);
	psy_ui_component_set_align(&self->tasks, psy_ui_ALIGN_TOP);
	psy_ui_component_set_default_align(&self->tasks, psy_ui_ALIGN_TOP,
		psy_ui_margin_make_em(1.0, 0.0, 0.0, 0.0));
	pluginscantaskview_init_task_list(self);
	/* buttons */
	psy_ui_component_init(&self->buttons, &self->component, NULL);
	psy_ui_component_set_margin(&self->buttons,
		psy_ui_margin_make_em(2.0, 0.0, 0.0, 0.0));
	psy_ui_component_set_align(&self->buttons, psy_ui_ALIGN_BOTTOM);
	psy_ui_button_init_text(&self->pause, &self->buttons,
		"Pause");
	psy_ui_component_set_align(psy_ui_button_base(&self->pause),
		psy_ui_ALIGN_LEFT);
	psy_ui_component_set_style_types(psy_ui_button_base(&self->pause),
		STYLE_DIALOG_BUTTON, STYLE_DIALOG_BUTTON_HOVER,
		STYLE_DIALOG_BUTTON_SELECT, psy_INDEX_INVALID);
	psy_ui_component_set_padding(psy_ui_button_base(&self->pause),
		psy_ui_margin_make_em(0.5, 4.0, 0.5, 4.0));
	psy_ui_button_init_text(&self->abort, &self->buttons,
		"newmachine.stop");	
	psy_ui_component_set_align(psy_ui_button_base(&self->abort),
		psy_ui_ALIGN_LEFT);	
	psy_ui_component_set_padding(psy_ui_button_base(&self->abort),
		psy_ui_margin_make_em(0.5, 4.0, 0.5, 4.0));
	psy_ui_component_set_margin(psy_ui_button_base(&self->abort),
		psy_ui_margin_make_em(0.0, 0.0, 0.0, 4.0));
	psy_ui_component_set_style_types(psy_ui_button_base(&self->abort),
		STYLE_DIALOG_BUTTON, STYLE_DIALOG_BUTTON_HOVER,
		STYLE_DIALOG_BUTTON_SELECT, psy_INDEX_INVALID);
}

void pluginscantaskview_init_task_list(PluginScanTaskView* self)
{
	const psy_List* p;

	assert(self);
	
	for (p = self->plugincatcher->scan_tasks_; p != NULL; p = p->next) {
		const psy_audio_PluginScanTask* task;
		psy_ui_Label* label;

		task = (const psy_audio_PluginScanTask*)psy_list_entry_const(p);
		label = psy_ui_label_allocinit(&self->tasks);
		psy_ui_label_prevent_translation(label);
		psy_ui_label_set_text(label, task->label);		
	}
}

void pluginscantaskview_select(PluginScanTaskView* self,
	psy_audio_PluginScanTask* task)
{
	uintptr_t index;
	psy_List* p;

	assert(self);
	
	index = 0;
	for (p = self->plugincatcher->scan_tasks_; p != NULL; p = p->next,
			++index) {
		psy_audio_PluginScanTask* curr;

		curr = (psy_audio_PluginScanTask*)p->entry;
		if (strcmp(curr->label, task->label) == 0) {
			break;
		}
	}
	if (p) {
		psy_ui_Component* component;

		psy_ui_component_removestylestate_children(&self->tasks,
			psy_ui_STYLESTATE_SELECT);
		component = psy_ui_component_at(&self->tasks, index);
		if (component) {
			psy_ui_component_add_style_state(component, psy_ui_STYLESTATE_SELECT);
		}
	}
}


/* PluginScanProcessView */

void pluginscanprocessview_init(PluginScanProcessView* self,
	psy_ui_Component* parent, psy_audio_PluginCatcher* plugin_catcher)
{
	assert(self);
	
	psy_ui_component_init(&self->component, parent, NULL);
	psy_ui_label_init_text(&self->scan, &self->component,
		"newmachine.scanning");
	psy_ui_component_set_margin(psy_ui_label_base(&self->scan),
		psy_ui_margin_make_em(2.0, 0.0, 2.0, 40.0));
	psy_ui_component_set_align(psy_ui_label_base(&self->scan),
		psy_ui_ALIGN_TOP);
	psy_ui_component_init(&self->client, &self->component, NULL);	
	psy_ui_component_set_align(&self->client, psy_ui_ALIGN_TOP);
	pluginscandescview_init(&self->descview, &self->client);
	psy_ui_component_set_align(&self->descview.component,
		psy_ui_ALIGN_LEFT);	
	pluginscanstatusview_init(&self->statusview, &self->client);
	psy_ui_component_set_align(&self->statusview.component,
		psy_ui_ALIGN_RIGHT);
	psy_ui_component_set_margin(&self->statusview.component,
		psy_ui_margin_make_em(0.0, 0.0, 0.0, 4.0));
	pluginscantaskview_init(&self->taskview, &self->client,
		plugin_catcher);
	psy_ui_component_set_align(&self->taskview.component,
		psy_ui_ALIGN_TOP);	
}

void pluginscanprocessview_stop(PluginScanProcessView* self)
{
	assert(self);
	
	clockbar_stop(&self->statusview.time);
}

void pluginscanprocessview_reset(PluginScanProcessView* self)
{
	assert(self);
	
	psy_ui_button_set_text(&self->taskview.abort, "newmachine.stop");	
	pluginscanstatusview_reset(&self->statusview);
	clockbar_restart(&self->statusview.time);	
}


/* PluginScanView */

/* prototypes */
static void pluginscanview_init_task_list(PluginScanView*);
static void pluginscanview_on_abort(PluginScanView*, psy_ui_Button* sender);
static void pluginscanview_on_scan_progress(PluginScanView*,
	PluginScanThread* sender, intptr_t progress);
static void pluginscanview_on_scan_file(PluginScanView*,
	PluginScanThread* sender, const char* path, intptr_t type);
static void pluginscanview_on_scan_task_start(PluginScanView* self,
	PluginScanThread* sender, psy_audio_PluginScanTask* task);
static void pluginscanview_output_num_scanned_files(PluginScanView*);

/* implementation */
void pluginscanview_init(PluginScanView* self, psy_ui_Component* parent,
	PluginScanThread* plugin_scan_thread)
{
	assert(self);
	assert(plugin_scan_thread);	
	
	psy_ui_component_init(&self->component, parent, NULL);	
	self->plugin_scan_thread = plugin_scan_thread;
	self->plugin_catcher = plugin_scan_thread->plugincatcher;
	psy_ui_component_set_padding(&self->component,
		psy_ui_margin_make_em(0.5, 0.5, 0.0, 0.0));
	pluginscanprocessview_init(&self->processview, &self->component,
		self->plugin_catcher);
	psy_ui_component_set_align(&self->processview.component,
		psy_ui_ALIGN_CLIENT);
	psy_signal_connect(&self->processview.taskview.abort.signal_clicked,
		self, pluginscanview_on_abort);
	psy_signal_connect(&plugin_scan_thread->signal_scanprogress, self,
		pluginscanview_on_scan_progress);
	psy_signal_connect(&plugin_scan_thread->signal_scanfile, self,
		pluginscanview_on_scan_file);
	psy_signal_connect(&plugin_scan_thread->signal_scantaskstart, self,
		pluginscanview_on_scan_task_start);	
}

void pluginscanview_on_abort(PluginScanView* self, psy_ui_Button* sender)
{
	assert(self);
	
	psy_ui_button_set_text(&self->processview.taskview.abort,
		"newmachine.stopping");
	psy_ui_component_align_full(&self->processview.taskview.buttons);
	psy_ui_component_invalidate(&self->processview.taskview.buttons);
	psy_audio_plugincatcher_abort(self->plugin_catcher);
}

void pluginscanview_reset(PluginScanView* self)
{
	assert(self);
		
	pluginscanprocessview_reset(&self->processview);		
}

void pluginscanview_stop(PluginScanView* self)
{	
	assert(self);
	
	pluginscanprocessview_stop(&self->processview);
	pluginscanview_output_num_scanned_files(self);
}

void pluginscanview_output_num_scanned_files(PluginScanView* self)
{
	psy_Logger* logger;
	
	assert(self);
		
	logger = psy_ui_component_logger(&self->component, psy_ui_LOG_STATUS);
	if (logger) {	
		char text[80];
		
		psy_snprintf(text, 80, "Found %d plugins (scanned %d files)",
			(int)(psy_audio_plugincatcher_plugin_count(self->plugin_catcher)),
			(int)(psy_audio_plugincatcher_file_count(self->plugin_catcher)));
		psy_logger_output(logger, text);
	}
}

void pluginscanview_on_scan_progress(PluginScanView* self,
	PluginScanThread* sender, intptr_t progress)
{	
	assert(self);
	
	pluginscanprocessview_update(&self->processview, pluginscanthread_catcher(
		sender));
}

void pluginscanview_on_scan_file(PluginScanView* self, PluginScanThread* sender,
	const char* path, intptr_t type)
{	
	psy_Logger* logger;
	
	assert(self);
	
	logger = psy_ui_component_logger(&self->component, psy_ui_LOG_STATUS);
	if (logger) {		
		psy_logger_output(logger, path);
	}	
}

void pluginscanview_on_scan_task_start(PluginScanView* self,
	PluginScanThread* sender, psy_audio_PluginScanTask* task)
{
	assert(self);
	
	pluginscantaskview_select(&self->processview.taskview, task);
}
