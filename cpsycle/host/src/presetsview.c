/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "presetsview.h"
/* audio */
#include <presetio.h>
/* platform */
#include "../../detail/cpu.h"
#include "../../detail/portable.h"


/* prototypes */
static void presetsview_on_destroyed(PresetsView*);
static void presetsview_init_buttons(PresetsView*);
static void presetsview_on_save_button(PresetsView*, psy_ui_Component* sender);
static void presetsview_on_delete_button(PresetsView*,
	psy_ui_Component* sender);
static void presetsview_on_import(PresetsView*,
	psy_ui_Component* sender);	
static void presetsview_on_import_changed(PresetsView*, psy_FileSelect* sender);
static void presetsview_import(PresetsView*, const char* path);
static void presetsview_on_export(PresetsView*,
	psy_ui_Component* sender);
static void presetsview_on_export_changed(PresetsView*, psy_FileSelect* sender);
static void presetsview_export(PresetsView*, const char* path);
static void presetsview_update_list(PresetsView*);
static void presetsview_on_use(PresetsView*, psy_ui_Component* sender);
static void presetsview_on_preset_selection_change(PresetsView*,
	psy_ui_ComboBox* sender);
static void presetsview_on_cancel(PresetsView*, psy_ui_Component* sender);
static void presetsview_on_mouse_double_click(PresetsView*, psy_ui_MouseEvent*);
static psy_audio_Machine* presetsview_machine(PresetsView*);
static void presetsview_use(PresetsView*);
static void presetsview_save_presets(PresetsView*);

/* vtable */
static psy_ui_ComponentVtable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(PresetsView* self)
{
	if (!vtable_initialized) {
		vtable = *(self->component.vtable);
		vtable.on_destroyed =
			(psy_ui_fp_component)
			presetsview_on_destroyed;		
		vtable.on_mouse_double_click =
			(psy_ui_fp_component_on_mouse_event)
			presetsview_on_mouse_double_click;		
		vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &vtable);
}

/* implementation */
void presetsview_init(PresetsView* self, psy_ui_Component* parent,
	Workspace* workspace)
{	
	assert(self);
	
	psy_ui_component_init(&self->component, parent, NULL);
	vtable_init(self);
	self->workspace = workspace;
	self->mac_id = psy_INDEX_INVALID;
	psy_audio_preset_init(&self->ini_preset);
	self->preset_changed = FALSE;
	self->presets_file_name = NULL;
	psy_ui_component_set_preferred_width(&self->component,
		psy_ui_value_make_ew(50.0));
	titlebar_init(&self->title_bar, &self->component, "Preset");
	psy_signal_connect(&self->title_bar.close_bar_.hide.signal_clicked, self,
		presetsview_on_cancel);
	presetsview_init_buttons(self);
	psy_ui_component_init_align(&self->client, &self->component, NULL,
		psy_ui_ALIGN_CLIENT);
	psy_ui_component_set_default_align(&self->client, psy_ui_ALIGN_TOP,		
		psy_ui_margin_zero());	
	psy_ui_combobox_init_simple(&self->programbox, &self->client);
	psy_ui_combobox_set_char_number(&self->programbox, 25.0);
	psy_ui_component_set_align(&self->programbox.component,
		psy_ui_ALIGN_CLIENT);		
	psy_signal_connect(&self->programbox.signal_selchanged, self,
		presetsview_on_preset_selection_change);
}

void presetsview_on_destroyed(PresetsView* self)
{	
	assert(self);
			
	psy_audio_preset_dispose(&self->ini_preset);
	free(self->presets_file_name);
	self->presets_file_name = NULL;
}

void presetsview_init_buttons(PresetsView* self)
{
	assert(self);
	
	psy_ui_component_init_align(&self->buttons, &self->component, NULL,
		psy_ui_ALIGN_RIGHT);
	psy_ui_component_set_default_align(&self->buttons, psy_ui_ALIGN_TOP,		
		psy_ui_margin_zero());
	psy_ui_button_init_text_connect(&self->save, &self->buttons,
		"machineframe.saveas", self, presetsview_on_save_button);
	psy_ui_button_init_text_connect(&self->del, &self->buttons,
		"machineframe.delete", self, presetsview_on_delete_button);
	psy_ui_button_init_text_connect(&self->import, &self->buttons,
		"machineframe.import", self, presetsview_on_import);
	psy_ui_button_init_text_connect(&self->export_preset, &self->buttons,
		"machineframe.export", self, presetsview_on_export);
	psy_ui_checkbox_init(&self->preview, &self->buttons);
	psy_ui_checkbox_set_text(&self->preview, "machineframe.preview");
	psy_ui_component_init_align(&self->bottom, &self->buttons, NULL,
		psy_ui_ALIGN_BOTTOM);
	psy_ui_component_set_default_align(&self->bottom, psy_ui_ALIGN_TOP,		
		psy_ui_margin_zero());
	psy_ui_button_init_text_connect(&self->use, &self->bottom,
		"machineframe.use", self, presetsview_on_use);
	psy_ui_button_init_text_connect(&self->close, &self->bottom,
		"machineframe.close", self, presetsview_on_cancel);
}

void presetsview_on_save_button(PresetsView* self, psy_ui_Component* sender)
{		
	psy_audio_Machine* machine;
	psy_audio_Preset* preset;
	psy_audio_Presets* presets;	
	intptr_t prg;
			
	assert(self);
		
	machine = presetsview_machine(self);
	if (!machine) {
		return;
	}
	prg =  psy_ui_combobox_cur_sel(&self->programbox);
	presets = psy_audio_machine_presets(machine);
	if (presets) {
		char text[256];
		
		preset = psy_audio_preset_alloc_init();
		psy_audio_machine_current_preset(machine, preset);
		psy_ui_combobox_text(&self->programbox, text);
		if (psy_strlen(text) == 0) {
			psy_audio_preset_set_name(preset, "untitled");
		} else {
			psy_audio_preset_set_name(preset, text);
		}		
		if (prg == psy_INDEX_INVALID) {
			psy_audio_presets_append(presets, preset);
		} else {
			psy_audio_presets_insert(presets, prg, preset);
		}
		psy_audio_presets_sort(presets);		
		presetsview_update_list(self);
		psy_ui_component_align(&self->programbox.listbox_.component);
		psy_ui_component_invalidate(&self->programbox.listbox_.component);
	}
	presetsview_save_presets(self);
	self->preset_changed = FALSE;
}

void presetsview_on_delete_button(PresetsView* self, psy_ui_Component* sender)
{
	psy_audio_Machine* machine;	
	psy_audio_Presets* presets;	
	uintptr_t sel;
	
	assert(self);
		
	machine = presetsview_machine(self);
	if (!machine) {
		return;
	}
	sel =  psy_ui_combobox_cur_sel(&self->programbox);
	presets = psy_audio_machine_presets(machine);
	if (presets && sel != -1) {
		psy_audio_presets_remove(presets, sel);
		psy_audio_presets_remove_empty(presets);
		psy_audio_presets_sort(presets);
		presetsview_update_list(self);
		psy_ui_component_align(&self->programbox.listbox_.component);
		psy_ui_component_invalidate(&self->programbox.listbox_.component);
		presetsview_save_presets(self);
	}	
}

void presetsview_on_import(PresetsView* self, psy_ui_Component* sender)
{
	psy_FileSelect import;
	
	assert(self);
		
	psy_fileselect_init(&import);		
	psy_fileselect_connect_change(&import, self, (psy_fp_fileselect_change)
		presetsview_on_import_changed);
	psy_fileselect_set_title(&import, "Load Presets");
	psy_fileselect_set_default_extension(&import, "prs");
	psy_fileselect_add_filter(&import, "prs", "*.prs");				
	psy_fileselect_set_directory(&import,
		psy_configuration_value_str(
			psycleconfig_directories(workspace_cfg(self->workspace)),			
			"presets", PSYCLE_USERPRESETS_DEFAULT_DIR));
	workspace_disk_op(self->workspace, &import, NULL, NULL);
	psy_fileselect_dispose(&import);
}

void presetsview_on_import_changed(PresetsView* self, psy_FileSelect* sender)
{		
	assert(self);
	
	if (psy_strlen(psy_fileselect_value(sender)) > 0) {
		presetsview_import(self, psy_fileselect_value(sender));		
	}
	workspace_select_view(self->workspace, viewindex_make(VIEW_ID_MACHINES));
}

void presetsview_import(PresetsView* self, const char* path)
{		
	int status;
	psy_audio_Presets* import;
	psy_audio_Machine* machine;
	const char* platform_plugins_dir;
		
	assert(self);
	
	machine = presetsview_machine(self);
	if (!machine) {
		return;
	}
	import = psy_audio_presets_alloc_init();
#if (DIVERSALIS__CPU__SIZEOF_POINTER == 4)
	platform_plugins_dir = psy_configuration_value_str(
		psy_dirconfig_base(&self->workspace->cfg_.directories),
		"plugins32", PSYCLE_PLUGINS32_DEFAULT_DIR);
#else
	platform_plugins_dir = psy_configuration_value_str(
		psycleconfig_directories(workspace_cfg(self->workspace)),		
		"plugins64", "");
#endif	
	status = psy_audio_presetsio_load(path,
		import,
		psy_audio_machine_num_tweak_parameters(machine),
		psy_audio_machine_data_size(machine),
		platform_plugins_dir);
	if (status != PSY_OK) {
		psy_Logger* logger;
		
		logger = psy_ui_component_logger(&self->component, psy_ui_LOG_TERMINAL);
		if (logger) {					
			psy_logger_error(logger, psy_audio_presetsio_statusstr(status));
		}		
		psy_audio_presets_dispose(import);
		free(import);
		import = NULL;
		free(self->presets_file_name);
		self->presets_file_name = NULL;
	} else {
		psy_audio_Presets* presets;
		
		psy_strreset(&self->presets_file_name, path);	
		presets = psy_audio_machine_presets(machine);
		if (presets) {
			psy_audio_presets_merge(presets, import);
			psy_audio_presets_dispose(import);
			import = NULL;
		} else {
			presets = import;
		}	
		psy_audio_machine_set_presets(machine, presets);
		presetsview_update_list(self);
	}
}

void presetsview_on_export(PresetsView* self, psy_ui_Component* sender)
{	
	psy_FileSelect export;
	
	assert(self);
		
	psy_fileselect_init(&export);		
	psy_fileselect_connect_change(&export, self, (psy_fp_fileselect_change)
		presetsview_on_export_changed);
	psy_fileselect_set_title(&export, "Export Presets");
	psy_fileselect_set_default_extension(&export, "prs");
	psy_fileselect_add_filter(&export, "prs", "*.prs");				
	psy_fileselect_set_directory(&export,
		psy_configuration_value_str(
			psycleconfig_directories(workspace_cfg(self->workspace)),			
			"presets", PSYCLE_USERPRESETS_DEFAULT_DIR));
	workspace_disk_op(self->workspace, NULL, &export, NULL);
	psy_fileselect_dispose(&export);
}	

void presetsview_on_export_changed(PresetsView* self, psy_FileSelect* sender)
{
	assert(self);
	
	if (psy_strlen(psy_fileselect_value(sender)) > 0) {
		presetsview_export(self, psy_fileselect_value(sender));		
	}
	workspace_select_view(self->workspace, viewindex_make(VIEW_ID_MACHINES));
}

void presetsview_export(PresetsView* self, const char* path)
{	
	int status;	
	psy_audio_Machine* machine;
	psy_audio_Presets* presets;
		
	assert(self);
	
	machine = presetsview_machine(self);
	if (!machine) {
		return;
	}
	presets = psy_audio_machine_presets(machine);
	if (presets) {
		status = psy_audio_presetsio_save(path, psy_audio_machine_presets(
			machine));
		if (status) {
			psy_Logger* logger;
		
			logger = psy_ui_component_logger(&self->component,
				psy_ui_LOG_TERMINAL);
			if (logger) {					
				psy_logger_error(logger, psy_audio_presetsio_statusstr(status));
			}
		}
	}
}

void presetsview_update_list(PresetsView* self)
{	
	psy_audio_Machine* machine;
	
	assert(self);
		
	psy_ui_combobox_clear(&self->programbox);
	machine = presetsview_machine(self);
	if (machine) {
		uintptr_t numprograms;
		uintptr_t i;
		
		numprograms = psy_audio_machine_num_programs(machine);
		for (i = 0; i < numprograms; ++i) {
			char name[256];

			psy_audio_machine_program_name(machine, 0, i, name);
			psy_ui_combobox_add_text(&self->programbox, name);
		}
	}
}

void presetsview_set_mac_id(PresetsView* self, uintptr_t mac_id)
{
	if (self->mac_id != mac_id) {
		psy_audio_Machine* machine;
			
		assert(self);
			
		self->mac_id = mac_id;
		machine = presetsview_machine(self);
		psy_audio_preset_clear(&self->ini_preset);
		self->preset_changed = FALSE;
		if (machine) {
			psy_audio_machine_current_preset(machine, &self->ini_preset);		
		}
		presetsview_update_list(self);
	}
}

void presetsview_on_mouse_double_click(PresetsView* self, psy_ui_MouseEvent* ev)
{
	assert(self);
	
	psy_ui_mouseevent_stop_propagation(ev);
	presetsview_use(self);
	psy_ui_component_hide(&self->component);
	psy_ui_component_align(psy_ui_component_parent(
		psy_ui_component_parent(&self->component)));
	psy_ui_component_invalidate(psy_ui_component_parent(
		psy_ui_component_parent(&self->component)));	
}

void presetsview_on_use(PresetsView* self, psy_ui_Component* sender)
{
	presetsview_use(self);
}

void presetsview_use(PresetsView* self)
{
	psy_audio_Machine* machine;	
		
	assert(self);
	
	machine = presetsview_machine(self);
	if (!machine) {
		return;
	}	
	psy_audio_machine_set_curr_program(machine, psy_ui_combobox_cur_sel(
		&self->programbox));
	psy_audio_preset_clear(&self->ini_preset);
	self->preset_changed = FALSE;	
	psy_audio_machine_current_preset(machine, &self->ini_preset);	
}


void presetsview_on_preset_selection_change(PresetsView* self,
	psy_ui_ComboBox* sender)
{
	if (psy_ui_checkbox_checked(&self->preview)) {
		psy_audio_Machine* machine;		
			
		assert(self);
		
		machine = presetsview_machine(self);
		if (!machine) {
			return;
		}		
		psy_audio_machine_set_curr_program(machine,
			psy_ui_combobox_cur_sel(sender));
		self->preset_changed = TRUE;
	}
}

void presetsview_on_cancel(PresetsView* self, psy_ui_Component* sender)
{	
	if (self->preset_changed) {		
		psy_audio_Machine* machine;

		/* restore old preset */		
		machine = presetsview_machine(self);
		if (machine) {
			psy_audio_machine_tweak_preset(machine, &self->ini_preset);
			self->preset_changed = FALSE;
		}
	}	
}

psy_audio_Machine* presetsview_machine(PresetsView* self)
{		
	assert(self);
	
	if (!self->workspace->song) {
		return NULL;
	}
	return psy_audio_machines_at(&self->workspace->song->machines_,
		self->mac_id);	
}

void presetsview_save_presets(PresetsView* self)
{
	int status;	
	psy_audio_Machine* machine;
	psy_audio_Presets* presets;
		
	assert(self);
	
	if (psy_strlen(self->presets_file_name) == 0) {
		return;
	}
	machine = presetsview_machine(self);
	if (!machine) {
		return;
	}
	presets = psy_audio_machine_presets(machine);
	if (presets) {		
		status = psy_audio_presetsio_save(self->presets_file_name, presets);
		if (status) {
			psy_Logger* logger;
		
			logger = psy_ui_component_logger(&self->component,
				psy_ui_LOG_TERMINAL);
			if (logger) {					
				psy_logger_error(logger, psy_audio_presetsio_statusstr(status));
			}
		}
	}
}
