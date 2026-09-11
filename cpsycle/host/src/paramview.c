/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "paramview.h"
/* host */
#include "headerui.h"
#include "labelui.h"
#include "knobui.h"
#include "slidergroupui.h"
#include "styles.h"
#include "switchui.h"
#include "machineui.h"
/* audio */
#include <plugin_interface.h>
/* std */
#include <assert.h>
/* platform */
#include "../../detail/portable.h"

/* paramview */
/* prototypes */
static void paramview_on_destroyed(ParamView*);
static void paramview_update_skin(ParamView*);
static void paramview_on_timer(ParamView*, uintptr_t timerid);
static uintptr_t paramview_num_rows(const ParamView*);
static void paramview_build(ParamView*);
static uintptr_t paramview_num_cols(const ParamView*);
static void paramview_on_knob_edit(ParamView*, KnobUi* sender);

/* vtable */
static psy_ui_ComponentVtable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(ParamView* self)
{
	assert(self);

	if (!vtable_initialized) {
		vtable = *(self->component.vtable);
		vtable.on_destroyed =
			(psy_ui_fp_component)
			paramview_on_destroyed;
		vtable.on_timer =
			(psy_ui_fp_component_on_timer)
			paramview_on_timer;
		vtable_initialized = TRUE;
	}
	self->component.vtable = &vtable;
}

/* implementation */
void paramview_init(ParamView* self, psy_ui_Component* parent,
	psy_audio_Machine* machine, psy_Configuration* param_cfg,
	psy_ui_Component** frameview, bool use_column_hint)
{	
	assert(self);	
	
	psy_ui_component_init(&self->component, parent, NULL);
	vtable_init(self);
	psy_ui_component_doublebuffer(&self->component);
	psy_ui_component_init(&self->pane, &self->component, 
		(frameview) ? &self->component : NULL);
	psy_ui_component_set_align(&self->pane, psy_ui_ALIGN_CLIENT);
	psy_ui_component_set_align_expand(&self->pane, psy_ui_HEXPAND);
	psy_signal_init(&self->signal_edit);
	self->cfg = param_cfg;	
	self->machine = machine;	
	self->paramstrobe = 0;
	self->sizechanged = 1;
	self->frameview = frameview;	
	self->use_column_hint = use_column_hint;
	paramview_update_skin(self);
	paramview_build(self);		
}

void paramview_on_destroyed(ParamView* self)
{	
	assert(self);

	psy_signal_dispose(&self->signal_edit);
}

ParamView* paramview_alloc(void)
{
	return (ParamView*) malloc(sizeof(ParamView));
}

ParamView* paramview_allocinit(psy_ui_Component* parent,
	psy_audio_Machine* machine, psy_Configuration* param_cfg,
	psy_ui_Component** frameview,
	bool use_column_hint)
{
	ParamView* rv;

	rv = paramview_alloc();
	if (rv) {
		paramview_init(rv, parent, machine, param_cfg, frameview,
			use_column_hint);
		psy_ui_component_deallocate_after_destroyed(&rv->component);
	}
	return rv;
}

void paramview_update_skin(ParamView* self)
{
	psy_Property* p;	
	
	assert(self);

	
	if (self->cfg) {
		p = psy_configuration_at(self->cfg, "font");
		if (p) {	
			psy_ui_fontinfo_init_string(&self->fontinfo,
				psy_property_item_str(p));
		} else {
			psy_ui_fontinfo_init_string(&self->fontinfo, "tahoma; 16");		
		}		
	} else {
		psy_ui_fontinfo_init_string(&self->fontinfo, "tahoma; 16");
	}
	psy_ui_component_set_font_info(&self->component, self->fontinfo);
}

void paramview_on_timer(ParamView* self, uintptr_t timerid)
{
	if (self->machine && psy_audio_machine_param_strobe(self->machine)
			!= self->paramstrobe) {
		paramview_build(self);
		self->paramstrobe = psy_audio_machine_param_strobe(self->machine);
	}
	psy_ui_component_invalidate(&self->component);
}

void paramview_build(ParamView* self)
{
	psy_ui_component_clear(&self->pane);
	if (self->machine && psy_audio_machine_num_parameters(self->machine) > 0
			&& paramview_num_cols(self) > 0) {
		uintptr_t row = 0;
		uintptr_t col = 0;
		uintptr_t numrows = 0;
		uintptr_t paramnum;
		psy_ui_Component* currcolumn;
		SliderGroupUi* currslider;
		
		numrows = paramview_num_rows(self);
		currcolumn = NULL;
		currslider = NULL;
		for (paramnum = 0; paramnum < psy_audio_machine_num_parameters(
			self->machine); ++paramnum) {			
			psy_audio_MachineParam* machineparam;

			if (row == 0) {
				currcolumn = psy_ui_component_allocinit(&self->pane, NULL);
				psy_ui_component_set_align(currcolumn, psy_ui_ALIGN_LEFT);
			}
			machineparam = psy_audio_machine_parameter(self->machine, paramnum);
			if (machineparam) {
				psy_ui_Component* component;
				bool slider;
				bool checkinslidergroup;

				component = NULL;
				slider = FALSE;
				checkinslidergroup = FALSE;
				switch (psy_audio_machine_parameter_type(self->machine, machineparam) & 0x1FF) {
				case MPF_HEADER: {
					HeaderUi* header;

					header = headerui_allocinit(currcolumn,						
						self->machine, paramnum, machineparam);
					if (header) {
						component = &header->component;
					}
					break; }
				case MPF_INFOLABEL: {
					LabelUi* label;

					label = labelui_allocinit(currcolumn, self->machine,
						paramnum, machineparam);
					if (label) {
						component = &label->component;
					}
					break; }
				case MPF_STATE: {
					KnobUi* knob;

					knob = knobui_alloc_init(currcolumn,	self->machine,
						paramnum, machineparam);
					if (knob) {
						component = &knob->component;
						psy_signal_connect(&knob->signal_edit, self,
							paramview_on_knob_edit);
					}
					break; }
				case MPF_SLIDER: {
					currslider = slidergroupui_allocinit(currcolumn,
						self->machine, paramnum, machineparam,
						psy_INDEX_INVALID, NULL);
					if (currslider) {
						component = &currslider->component;
					}
					break; }
				case MPF_LEVEL: {
					if (currslider) { // && machineparam->isslidergroup)
						currslider->level.param = machineparam;
					}
					break; }
				case MPF_CHECK: {
					SwitchUi* switchui;

					if (currslider && machineparam->isslidergroup) {
						switchui = switchui_allocinit(&currslider->controls,
							self->machine, paramnum, machineparam,
							STYLE_MACPARAM_CHECKOFF,
							STYLE_MACPARAM_CHECKON);
						checkinslidergroup = TRUE;
					} else {
						switchui = switchui_allocinit(currcolumn,
							self->machine, paramnum, machineparam,
							STYLE_MACPARAM_CHECKON,
							STYLE_MACPARAM_CHECKOFF);
					}
					if (switchui) {
						component = &switchui->component;
					}
					break; }				
				case MPF_SWITCH: {
					SwitchUi* switchui;

					if (currslider && machineparam->isslidergroup) {
						switchui = switchui_allocinit(&currslider->controls,
							self->machine, paramnum, machineparam,
							STYLE_MACPARAM_SWITCHOFF,
							STYLE_MACPARAM_SWITCHON);						
						checkinslidergroup = TRUE;
					} else {
						switchui = switchui_allocinit(currcolumn,
							self->machine, paramnum, machineparam,
							STYLE_MACPARAM_SWITCHOFF,
							STYLE_MACPARAM_SWITCHON);
					}
					if (switchui) {
						switchui->maxheight = 2.0;
						component = &switchui->component;
					}
					break; }
				case MPF_IGNORE:					
					break;
				default: {
					component = psy_ui_component_allocinit(currcolumn, NULL);
					psy_ui_component_set_preferred_size(component,
						psy_ui_size_make_em(PARAMWIDTH_SMALL, 2.0));
					psy_ui_component_set_aligner(component, NULL);
					break; }
				}
				if (component) {
					if ((psy_audio_machine_parameter_type(self->machine, machineparam) &
							MPF_BOTTOM) == MPF_BOTTOM) {						
						psy_ui_component_set_align(component, psy_ui_ALIGN_BOTTOM);						
					} else {
						psy_ui_component_set_align(component, psy_ui_ALIGN_TOP);
					}
					if (slider) {
						psy_ui_component_set_minimum_size(component,
							psy_ui_size_make_em(20.0, 15.0));
					}
				}
			}
			++row;
			if (row >= numrows) {
				row = 0;
				++col;
			}
		}
	}
	psy_ui_component_align(&self->pane);
	self->sizechanged = 1;
	psy_signal_emit(&self->component.signal_preferred_size_changed, self, 0);
}

uintptr_t paramview_num_rows(const ParamView* self)
{		
	if (self->machine && paramview_num_cols(self) > 0) {
		return (psy_audio_machine_num_parameters(self->machine) - 1) /
			paramview_num_cols(self) + 1;
	}
	return 0;
}

uintptr_t paramview_num_cols(const ParamView* self)
{
	if (self->use_column_hint) {
		return psy_audio_machine_num_parameter_cols(self->machine);
	}
	return 1;		
}

void paramview_set_zoom(ParamView* self, double zoomrate)
{
	psy_ui_Font font;	
	psy_ui_FontInfo fontinfo;

	fontinfo = self->fontinfo;
	fontinfo.lfHeight = (int32_t)(self->fontinfo.lfHeight * zoomrate);
	psy_ui_font_init(&font, &fontinfo);
	psy_ui_component_set_font(&self->component, &font);
	psy_ui_font_dispose(&font);
	self->sizechanged = 1;
}

void paramview_on_knob_edit(ParamView* self, KnobUi* sender)
{
	assert(self);

	psy_signal_emit(&self->signal_edit, self, 1, sender->paramidx);
}
