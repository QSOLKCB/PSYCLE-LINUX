/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "machineframe.h"
/* host */
#include "paramviews.h"
#include "resources/resource.h"
#include "machineeditorview.h"
#include "paramview.h"
#include "styles.h"
/* ui */
#include <uiframe.h>
/* platform */
#include "../../detail/portable.h"
#include "../../detail/trace.h"


/* ParameterBar */

/* implementation */
void parameterbar_init(ParameterBar* self, psy_ui_Component* parent,
	const psy_ui_SVG* power)
{			
	psy_ui_Margin margin;
	
	psy_ui_component_init(&self->component, parent, NULL);
	psy_ui_component_doublebuffer(&self->component);
	psy_ui_margin_init_em(&margin, 0.0, 1.0, 0.0, 0.0);
	psy_ui_component_set_default_align(&self->component, psy_ui_ALIGN_TOP,
		psy_ui_defaults_vmargin(psy_ui_defaults()));
	/* row0 */
	psy_ui_component_init(&self->row0, &self->component, NULL);
	psy_ui_component_set_align_expand(&self->row0, psy_ui_HEXPAND);	
	zoombox_init(&self->zoombox, &self->row0);
	psy_ui_component_set_align(zoombox_base(&self->zoombox), psy_ui_ALIGN_LEFT);	
	psy_ui_component_init(&self->buttons, &self->row0, NULL);
	psy_ui_component_set_align(&self->buttons, psy_ui_ALIGN_CLIENT);
	psy_ui_component_set_align_expand(&self->buttons, psy_ui_HEXPAND);
	psy_ui_component_set_default_align(&self->buttons, psy_ui_ALIGN_LEFT,
		psy_ui_defaults_hmargin(psy_ui_defaults()));
	psy_ui_button_init(&self->power, &self->buttons);
	psy_ui_button_set_text(&self->power, "machineframe.pwr");
	margin = psy_ui_component_margin(&self->power.component);
	margin.left = psy_ui_value_make_ew(1.0);
	psy_ui_component_set_margin(&self->power.component, margin);	
	psy_ui_button_set_svg(&self->power, power);	
	psy_ui_button_init(&self->parameters, &self->buttons);
	psy_ui_button_set_text(&self->parameters, "machineframe.parameters");
	psy_ui_button_init(&self->parammap, &self->buttons);
	psy_ui_button_set_text(&self->parammap, "machineframe.parammap");	
	psy_ui_button_init(&self->help, &self->buttons);
	psy_ui_button_set_text(&self->help, "machineframe.help");	
	psy_ui_button_init(&self->command, &self->buttons);
	psy_ui_button_set_text(&self->command, "Command");	
	psy_ui_button_init(&self->isbus, &self->buttons);
	psy_ui_button_set_text(&self->isbus, "machineframe.bus");	
	psy_ui_button_init(&self->bank, &self->buttons);
	psy_ui_button_set_text(&self->bank, "machineframe.bank");	
	psy_ui_button_init(&self->more, &self->row0);
	psy_ui_button_prevent_translation(&self->more);
	psy_ui_button_set_text(&self->more, ". . .");
	psy_ui_component_set_align(psy_ui_button_base(&self->more),
		psy_ui_ALIGN_RIGHT);
	psy_ui_component_set_margin(psy_ui_button_base(&self->more),
		psy_ui_margin_zero());	
	/* row1 */
	programbar_init(&self->programbar, &self->component);
	psy_ui_component_set_align(&self->programbar.component, psy_ui_ALIGN_TOP);	
}


/* FrameMachine */

/* prototypes */
static void machineframe_on_destroyed(FrameMachine*);
static void machineframe_init_param_view(FrameMachine*, Workspace*);
static void machineframe_init_help(FrameMachine*, psy_ui_Component* parent);
static void machineframe_toggleparameterbox(FrameMachine*,
	psy_ui_Component* sender);
static void machineframe_toggleparammap(FrameMachine*,
	psy_ui_Component* sender);
static void machineframe_togglepwr(FrameMachine*,
	psy_ui_Component* sender);
static void machineframe_updatepwr(FrameMachine*);
static void machineframe_toggle_help(FrameMachine*,
	psy_ui_Component* sender);
static void machineframe_command(FrameMachine*,
	psy_ui_Component* sender);
static void machineframe_togglebus(FrameMachine*,
	psy_ui_Component* sender);
static void machineframe_on_bank_manager(FrameMachine*,
	psy_ui_Component* sender);
static void machineframe_toggles_show_full_menu(FrameMachine*,
	psy_ui_Component* sender);
static void machineframe_resize(FrameMachine*);
static void machineframe_onalign(FrameMachine*, psy_ui_Component* sender);
static void machineframe_preferredviewsizechanged(FrameMachine*,
	psy_ui_Component* sender);
static void ondefaultfontchanged(FrameMachine*, Workspace* sender);
static void machineframe_on_zoombox_changed(FrameMachine*, ZoomBox* sender);
static void machineframe_on_param_edit(FrameMachine*, ParamView* sender,
	uintptr_t param_idx);
static void machineframe_on_timer(FrameMachine*, uintptr_t timerid);
static bool machineframe_on_close(FrameMachine*);
static void machineframe_on_param_edit_cancel(FrameMachine*,
	psy_ui_Component* sender);

/* vtable */
static psy_ui_ComponentVtable machineframe_vtable;
static bool machineframe_vtable_initialized = FALSE;

static void machineframe_vtable_init(FrameMachine* self)
{
	if (!machineframe_vtable_initialized) {
		machineframe_vtable = *(self->component.vtable);
		machineframe_vtable.onclose =
			(psy_ui_fp_component_onclose)
			machineframe_on_close;
		machineframe_vtable.on_timer =
			(psy_ui_fp_component_on_timer)
			machineframe_on_timer;
		machineframe_vtable_initialized = TRUE;
	}
	self->component.vtable = &machineframe_vtable;
}

/* implementation */
void machineframe_init(FrameMachine* self, psy_ui_Component* parent,
	psy_audio_Machine* machine, ParamViews* paramviews, Workspace* workspace)
{	
	psy_ui_toolframe_init(&self->component, parent);
	machineframe_vtable_init(self);
	psy_ui_component_move(&self->component,
		psy_ui_point_make_em(20.0, 15.0));
	psy_ui_component_set_icon_ressource(&self->component, IDI_MACPARAM);	
	self->view = NULL;
	self->paramview = NULL;
	self->editorview = NULL;
	self->paramviews = paramviews;
	self->machine = NULL;
	self->machineview = parent;
	self->showfullmenu = FALSE;
	self->macid = psy_INDEX_INVALID;
	self->machine = machine;
	self->workspace = workspace;
	/* top */
	psy_ui_component_init(&self->top, &self->component, &self->component);
	psy_ui_component_set_align(&self->top, psy_ui_ALIGN_TOP);
	psy_ui_notebook_init(&self->bar_value_notebook, &self->top);
	psy_ui_component_set_align(&self->bar_value_notebook.component,
		psy_ui_ALIGN_TOP);
	parameterbar_init(&self->parameterbar, &self->bar_value_notebook.component,
		psy_ui_app_svg(psy_ui_app(), "power"));
	psy_ui_component_set_align(&self->parameterbar.component, psy_ui_ALIGN_TOP);
	newvalview_init(&self->newval, &self->bar_value_notebook.component, 0, 0, 0, 0, 0,
		"new val", workspace);
	psy_signal_connect(&self->newval.cancel.signal_clicked, self,
		machineframe_on_param_edit_cancel);			
	psy_ui_component_set_align(&self->newval.component, psy_ui_ALIGN_TOP);
	psy_ui_component_hide(&self->newval.component);		
	/* right */
	psy_ui_component_init(&self->right, &self->component, &self->component);
	psy_ui_component_set_align(&self->right, psy_ui_ALIGN_RIGHT);
	/* parammap */
	parammap_init(&self->parammap, &self->right, NULL);	
	psy_ui_component_set_align(&self->parammap.component, psy_ui_ALIGN_LEFT);
	/* parameters */	
	parameterlistbox_init(&self->parameterbox, &self->right, NULL);	
	psy_ui_component_set_align(&self->parameterbox.component,
		psy_ui_ALIGN_LEFT);
	/* client */	
	psy_ui_notebook_init(&self->notebook, &self->component);
	psy_ui_component_set_align(psy_ui_notebook_base(&self->notebook),
		psy_ui_ALIGN_CLIENT);
	machineframe_init_help(self, psy_ui_notebook_base(&self->notebook));	
	/* connect signals */
	psy_signal_connect(&self->parameterbar.parameters.signal_clicked, self,
		machineframe_toggleparameterbox);
	psy_signal_connect(&self->parameterbar.parammap.signal_clicked, self,
		machineframe_toggleparammap);	
	psy_signal_connect(&self->parameterbar.help.signal_clicked, self,
		machineframe_toggle_help);
	psy_signal_connect(&self->parameterbar.command.signal_clicked, self,
		machineframe_command);
	psy_signal_connect(&self->parameterbar.power.signal_clicked, self,
		machineframe_togglepwr);
	psy_signal_connect(&self->parameterbar.isbus.signal_clicked, self,
		machineframe_togglebus);
	psy_signal_connect(&self->parameterbar.bank.signal_clicked, self,
		machineframe_on_bank_manager);
	psy_signal_connect(&self->parameterbar.more.signal_clicked, self,
		machineframe_toggles_show_full_menu);	
	psy_signal_connect(&self->component.signal_align, self,
		machineframe_onalign);	
	psy_signal_connect(&self->parameterbar.zoombox.signal_changed, self,
		machineframe_on_zoombox_changed);	
	machineframe_init_param_view(self, workspace);
	psy_ui_notebook_select(&self->notebook, 0);
	machineframe_updatepwr(self);	
}

void machineframe_init_help(FrameMachine* self, psy_ui_Component* parent)
{
	psy_ui_component_init(&self->help, parent, parent);
	psy_ui_component_set_align(&self->help, psy_ui_ALIGN_CLIENT);
	psy_ui_component_init(&self->helpview, &self->help, NULL);
	psy_ui_component_set_align(&self->helpview, psy_ui_ALIGN_CLIENT);
	psy_ui_component_set_style_type(&self->helpview, STYLE_MACPARAM_TITLE);	
	psy_ui_label_init_text(&self->help_text, &self->helpview,
		"machineframe.about");	
	psy_ui_component_set_wheel_scroll(&self->help_text.component, 4);	
	psy_ui_component_set_overflow(&self->help_text.component,
		psy_ui_OVERFLOW_SCROLL);
	psy_ui_component_set_align(&self->help_text.component, psy_ui_ALIGN_HCLIENT);
	psy_ui_label_enable_wrap(&self->help_text);
	psy_ui_component_set_scroll_step_height(psy_ui_label_base(&self->help_text),
		psy_ui_value_make_eh(1.0));	
	psy_ui_scroller_init(&self->scroller, &self->helpview, NULL, NULL);
	psy_ui_component_set_style_type(&self->scroller.pane, STYLE_MACPARAM_TITLE);
	psy_ui_scroller_set_client(&self->scroller, &self->help_text.component);
	psy_ui_component_set_align(&self->scroller.component, psy_ui_ALIGN_CLIENT);	
	psy_ui_component_init_align(&self->help_right, &self->helpview, NULL,
		psy_ui_ALIGN_RIGHT);
	psy_ui_component_set_padding(&self->help_right, psy_ui_margin_make_em(
		1.0, 1.0, 1.0, 5.0));
	psy_ui_component_set_maximum_size(&self->help_right, 
			psy_ui_size_make_em(40.0, 0.0));
	if (self->machine) {			
		const psy_audio_MachineInfo* machine_info;
		
		machine_info = psy_audio_machine_info(self->machine);
		if (machine_info) {			
			psy_ui_label_init_text(&self->image_desc, &self->help_right,
				machine_info->image_desc);
			psy_ui_label_enable_wrap(&self->image_desc);
			psy_ui_component_set_align(&self->image_desc.component,
				psy_ui_ALIGN_BOTTOM);			
			psy_ui_image_init_resource(&self->image, &self->help_right,
				machine_info->image_id);
			psy_ui_image_set_bitmap_alignment(&self->image,
				psy_ui_ALIGNMENT_RIGHT);
			psy_ui_component_set_align(psy_ui_image_base(&self->image),
				psy_ui_ALIGN_BOTTOM);
			psy_ui_component_set_margin(psy_ui_image_base(&self->image),
				psy_ui_margin_make_em(0.0, 0.0, 1.0, 0.0));			
		}
	}	
}

bool machineframe_on_close(FrameMachine* self)
{
	if (self->editorview && self->machine) {
		psy_audio_machine_set_editor_handle(self->machine, NULL);
		psy_signal_disconnect_context(
			&self->workspace->signal_machine_edit_resize, self->editorview);
		psy_ui_component_stop_timer(&self->editorview->component, 0);		
	}
	/*
	** Paramview stores pointers of all machineframes erase the frame from
	** paramviews
	*/
	if (self->machine) {
		paramviews_erase(self->paramviews, self->macid);
	}
	self->machine = NULL;
	self->view = NULL;	
	return TRUE;
}

FrameMachine* machineframe_alloc(void)
{
	return (FrameMachine*)malloc(sizeof(FrameMachine));
}

FrameMachine* machineframe_alloc_init(psy_ui_Component* parent,
	psy_audio_Machine* machine, struct ParamViews* paramviews,
	Workspace* workspace)
{
	FrameMachine* rv;

	rv = machineframe_alloc();
	if (rv) {
		machineframe_init(rv, parent, machine, paramviews, workspace);
		psy_ui_component_deallocate_after_destroyed(&rv->component);
	}
	return rv;
}

void machineframe_init_param_view(FrameMachine* self, Workspace* workspace)
{
	if (!self->machine) {
		return;
	}
	if (psy_audio_machine_has_editor(self->machine)) {
		self->editorview = machineeditorview_allocinit(
			psy_ui_notebook_base(&self->notebook), self->machine, workspace);
		if (self->editorview) {
			machineframe_setview(self, &self->editorview->component,
				self->machine);
		}
	} else {
		ParamView* paramview;

		paramview = paramview_allocinit(
			psy_ui_notebook_base(&self->notebook),
			self->machine,
			psycleconfig_macparam(workspace_cfg(workspace)),
			&self->view, TRUE);		
		if (paramview) {
			machineframe_set_param_view(self, paramview, self->machine);
		}
	}
}

void machineframe_set_param_view(FrameMachine* self, ParamView* view,
	psy_audio_Machine* machine)
{
	assert(self);

	self->paramview = view;
	machineframe_setview(self, &view->component, machine);
	machineframe_updatepwr(self);
	psy_signal_connect(&view->signal_edit, self, machineframe_on_param_edit);
}

void machineframe_setview(FrameMachine* self, psy_ui_Component* view,
	psy_audio_Machine* machine)
{	
	char text[128];
	
	self->view = view;
	self->machine = machine;
	psy_ui_component_set_align(self->view, psy_ui_ALIGN_CLIENT);		
	parameterlistbox_setmachine(&self->parameterbox, machine);
	psy_ui_component_hide(&self->parameterbox.component);
	parammap_setmachine(&self->parammap, machine);
	psy_ui_component_hide(&self->parammap.component);
	programbar_set_machine(&self->parameterbar.programbar, machine);	
	if (self->machine) {
		self->macid = psy_audio_machine_slot(self->machine);
	} else {
		self->macid = psy_INDEX_INVALID;
	}
	if (self->machine &&
			psy_audio_machine_mode(self->machine) == psy_audio_MACHMODE_GENERATOR) {
		psy_ui_component_hide(&self->parameterbar.isbus.component);
	}
	if (self->machine && psy_audio_machine_has_editor(self->machine)) {
		psy_ui_component_hide(&self->parameterbar.zoombox.component);
	}
	if (self->machine && psy_audio_machine_edit_name(self->machine)) {
		psy_snprintf(text, 128, "%.2X : %s",
			psy_audio_machine_slot(self->machine),
			psy_audio_machine_edit_name(self->machine));
	} else {
		psy_ui_component_set_title(&self->component, text);
			psy_snprintf(text, 128, "%.2X :",
				psy_audio_machine_slot(self->machine));
	}
	psy_ui_notebook_select(&self->notebook, 1);
	psy_ui_component_set_title(&self->component, text);
	psy_ui_component_align(&self->component);
	psy_signal_connect(&view->signal_preferred_size_changed, self,
		machineframe_preferredviewsizechanged);	
	if (self->machine && psy_audio_machine_info(machine)) {
		const char* text;		
			
		if (psy_audio_machine_type(machine) == psy_audio_PLUGIN) {
			psy_audio_machine_command(machine);
			text = self->workspace->hostmachinecallback.message;
		} else {
			text = (psy_audio_machine_info(machine)->helptext)
			? psy_audio_machine_info(machine)->helptext
			: NULL;		
		}			
		if (text) {
			psy_ui_label_set_text(&self->help_text, text);
		} else {
			psy_ui_label_set_text(&self->help_text, "");			
		}
	} else {
		psy_ui_label_set_text(&self->help_text, "");
	}
	if (self->machine) {
		if (psy_audio_machine_is_bus(self->machine)) {	
			psy_ui_button_highlight(&self->parameterbar.isbus);
		} else {			
			psy_ui_button_disable_highlight(&self->parameterbar.isbus);			
		}
	}	
	machineframe_resize(self);		
	psy_ui_component_start_timer(&self->component, 0, 50);
}

void machineframe_toggleparameterbox(FrameMachine* self,
	psy_ui_Component* sender)
{
	psy_ui_Size viewsize;

	assert(self);

	viewsize = psy_ui_component_preferred_size(self->view, 0);
	if (psy_ui_component_visible(&self->parameterbox.component)) {
		psy_ui_component_hide(&self->parameterbox.component);		
		psy_ui_button_disable_highlight(&self->parameterbar.parameters);
	} else {
		psy_ui_component_show(&self->parameterbox.component);		
		psy_ui_button_highlight(&self->parameterbar.parameters);
	}
	machineframe_resize(self);
	psy_ui_component_align(&self->component);
}

void machineframe_toggleparammap(FrameMachine* self,
	psy_ui_Component* sender)
{
	psy_ui_Size viewsize;

	assert(self);

	viewsize = psy_ui_component_preferred_size(self->view, 0);
	if (psy_ui_component_visible(&self->parammap.component)) {
		psy_ui_component_hide(&self->parammap.component);
		psy_ui_button_disable_highlight(&self->parameterbar.parammap);
	} else {
		psy_ui_component_show(&self->parammap.component);
		psy_ui_button_highlight(&self->parameterbar.parammap);
	}
	machineframe_resize(self);
	psy_ui_component_align(&self->component);
}

void machineframe_toggle_help(FrameMachine* self, psy_ui_Component* sender)
{	
	assert(self);

	if (psy_ui_notebook_page_index(&self->notebook) == 0) {			
		psy_ui_notebook_select(&self->notebook, 1);
		psy_ui_button_disable_highlight(&self->parameterbar.help);
	} else {		
		psy_ui_notebook_select(&self->notebook, 0);
		psy_ui_button_highlight(&self->parameterbar.help);
	}	
}

void machineframe_command(FrameMachine* self, psy_ui_Component* sender)
{
	if (self->machine) {
		psy_audio_machine_command(self->machine);
	}
}

void machineframe_togglepwr(FrameMachine* self,
	psy_ui_Component* sender)
{
	if (self->machine) {
		if (psy_audio_machine_has_standby(self->machine)) {
			psy_audio_machine_deactivate_standby(self->machine);			
		} else {
			psy_audio_machine_standby(self->machine);						
		}
		machineframe_updatepwr(self);
	}	
}

void machineframe_updatepwr(FrameMachine* self) {
	if (self->machine) {
		if (psy_audio_machine_has_standby(self->machine)) {
			psy_ui_button_disable_highlight(&self->parameterbar.power);
		} else {
			psy_ui_button_highlight(&self->parameterbar.power);			
		}
	}
}

void machineframe_togglebus(FrameMachine* self,
	psy_ui_Component* sender)
{
	if (self->machine) {
		if (psy_audio_machine_is_bus(self->machine)) {
			psy_audio_machine_unset_bus(self->machine);
			psy_ui_button_disable_highlight(&self->parameterbar.isbus);
		} else {
			psy_audio_machine_set_bus(self->machine);
			psy_ui_button_highlight(&self->parameterbar.isbus);
		}
	}
}

void machineframe_toggles_show_full_menu(FrameMachine* self,
	psy_ui_Component* sender)
{
	self->showfullmenu = !self->showfullmenu;
	machineframe_resize(self);
}

void machineframe_on_bank_manager(FrameMachine* self,
	psy_ui_Component* sender)
{	
	workspace_select_view(self->workspace, viewindex_make_all(
		VIEW_ID_MACHINES, SECTION_ID_MACHINEVIEW_BANK_MANGER, self->macid,
		psy_INDEX_INVALID));
}

void machineframe_resize(FrameMachine* self)
{
	psy_ui_Size viewsize;
	psy_ui_Size bar;
	psy_ui_Size newval;
	const psy_ui_TextMetric* tm;

	tm = psy_ui_component_textmetric(&self->component);	
	viewsize = psy_ui_component_preferred_size(self->view, NULL);	
	if (psy_ui_component_visible(&self->parameterbox.component)) {
		psy_ui_Size paramsize;		
		uintptr_t numparams;

		paramsize = psy_ui_component_preferred_size(
			&self->parameterbox.component, NULL);
		viewsize.width = psy_ui_add_values(viewsize.width, 
			paramsize.width, tm, NULL);
		numparams = psy_audio_machine_num_tweak_parameters(self->machine);
		viewsize.height = psy_ui_max_values(viewsize.height,
			psy_ui_value_make_eh(psy_min(numparams + 4.0, 8.0)), tm, NULL);
	}
	if (psy_ui_component_visible(&self->parammap.component)) {
		psy_ui_Size paramsize;
		uintptr_t numparams;

		paramsize = psy_ui_component_preferred_size(
			&self->parammap.component, NULL);
		viewsize.width = psy_ui_add_values(viewsize.width,
			paramsize.width, tm, NULL);
		numparams = psy_audio_machine_num_tweak_parameters(self->machine);
		viewsize.height = psy_ui_max_values(viewsize.height,
			psy_ui_value_make_eh(psy_min(numparams + 4.0, 8.0)), tm, NULL);
	}
	if (self->showfullmenu) {
		bar = psy_ui_component_preferred_size(&self->parameterbar.component,
			NULL);
		viewsize.width = psy_ui_max_values(viewsize.width, bar.width, tm, NULL);
	}
	newval = psy_ui_component_preferred_size(&self->newval.component,
		&viewsize);
	bar = psy_ui_component_preferred_size(&self->parameterbar.component,
		&viewsize);
	viewsize.height = psy_ui_add_values(bar.height, viewsize.height, tm, NULL);
	if (psy_ui_component_visible(&self->newval.component)) {
		viewsize.height = psy_ui_add_values(newval.height, viewsize.height, tm, NULL);
	}	
	psy_ui_component_clientresize(&self->component, viewsize);	
}

void machineframe_preferredviewsizechanged(FrameMachine* self,
	psy_ui_Component* sender)
{	
	machineframe_resize(self);	
}

void ondefaultfontchanged(FrameMachine* self, Workspace* sender)
{
	machineframe_resize(self);
}

void machineframe_on_zoombox_changed(FrameMachine* self, ZoomBox* sender)
{
	if (self->paramview) {		
		paramview_set_zoom(self->paramview, zoombox_rate(sender));
		machineframe_resize(self);
	}
}

void machineframe_on_param_edit(FrameMachine* self, ParamView* sender,
	uintptr_t param_idx)
{
	assert(self);

	if (param_idx != psy_INDEX_INVALID) {
		psy_audio_MachineParam* tweakpar;
		intptr_t min_v = 1;
		intptr_t max_v = 1;
		char name[64], title[128];			

		memset(name, 0, 64);
		tweakpar = psy_audio_machine_parameter(self->machine, param_idx);
		if (tweakpar) {
			psy_audio_machine_parameter_range(self->machine, tweakpar, &min_v, &max_v);
			psy_audio_machine_parameter_name(self->machine, tweakpar, name);
			psy_snprintf(
				title, 128, "Param:'%.2x:%s' (Range from %d to %d)\0",
				(int)param_idx, name, min_v, max_v);
			newvalview_reset(&self->newval, psy_audio_machine_slot(self->machine),
				param_idx, psy_audio_machine_parameter_scaled_value(
					self->machine, tweakpar), min_v, max_v, title);			
			psy_ui_component_set_focus(psy_ui_edit_base(&self->newval.edit));
		}
		psy_ui_notebook_select(&self->bar_value_notebook, 1);
	}
}

void machineframe_onalign(FrameMachine* self, psy_ui_Component* sender)
{	
	
}

void machineframe_on_timer(FrameMachine* self, uintptr_t timerid)
{
	assert(self);

	if (self->view) {
		self->view->vtable->on_timer(self->view, 0);
	}	
	machineframe_updatepwr(self);
	if (psy_ui_component_visible(&self->parameterbox.component)) {
		psy_ui_component_invalidate(&self->parameterbox.knob.component);
	}	
}

void machineframe_on_param_edit_cancel(FrameMachine* self,
	psy_ui_Component* sender)
{
	assert(self);

	psy_ui_notebook_select(&self->bar_value_notebook, 0);
}