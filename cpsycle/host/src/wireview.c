/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "wireview.h"
/* local */
#include "resources/resource.h"
/* audio */
#include <operations.h>
/* dsp */
#include <convert.h>
/* ui */
#include <uiframe.h>
/* std */
#include <math.h>
/* platform */
#include "../../detail/portable.h"

#define SCOPE_SPEC_BANDS 256

static const int SCOPE_BARS_WIDTH = 256 / SCOPE_SPEC_BANDS;
static const uint32_t CLBARDC = 0x001010DC;
static const uint32_t CLBARPEAK = 0x00C0C0C0;
static const uint32_t CLLEFT = 0x00C06060;
static const uint32_t CLRIGHT = 0x0060C060;
static const uint32_t CLBOTH = 0x00C0C060;
static const uint32_t linepenbL = 0x00705050;
static const uint32_t linepenbR = 0x00507050;
static const uint32_t linepenL = 0x00c08080;
static const uint32_t linepenR = 0x0080c080;

/* WireView */
enum {
	WIREVIEW_TAB_VUMETER = 0,
	WIREVIEW_TAB_OSCILLOSCOPE,
	WIREVIEW_TAB_SPECTRUM,
	WIREVIEW_TAB_PHASE
};

/* prototypes */
static void wireview_init_volume_slider(WireView*);
static void wireview_init_tab_bar(WireView*);
static void wireview_init_rate_group(WireView*);
static void wireview_init_bottom_group(WireView*);
static void wireview_update_text(WireView*, psy_Translator*);
static void wireview_update_title(WireView*, psy_audio_Machines*);
static void wireview_on_describe_volume(WireView*, psy_ui_Slider*, char* txt);
static void wireview_on_tweak_volume(WireView*, psy_ui_Slider*, double value);
static void wireview_on_value_volume(WireView*, psy_ui_Slider*, double* value);
static void wireview_on_tweak_mode(WireView*, psy_ui_Slider*, double value);
static void wireview_on_value_mode(WireView*, psy_ui_Slider*, double* value);
static void wireview_on_tweak_rate(WireView*, psy_ui_Slider*, double value);
static void wireview_on_value_rate(WireView*, psy_ui_Slider*, double* value);
static void wireview_on_hold(WireView*, psy_ui_Component* sender);
static void wireview_on_delete_connection(WireView*, psy_ui_Component* sender);
static void wireview_on_add_effect(WireView*, psy_ui_Component* sender);
static psy_ui_Component* wireview_scope(WireView*, int index);
static uintptr_t wireview_curr_scope(WireView*);
static void wireview_on_timer(WireView*, uintptr_t timerid);
/* vtable */
static psy_ui_ComponentVtable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(WireView* self)
{
	if (!vtable_initialized) {
		vtable = *(self->component.vtable);
		vtable.on_timer =
			(psy_ui_fp_component_on_timer)
			wireview_on_timer;
		vtable_initialized = TRUE;
	}
	self->component.vtable = &vtable;
}

/* implementation */
void wireview_init(WireView* self, psy_ui_Component* parent,
	psy_ui_Component* view, psy_audio_Wire wire,
	Workspace* workspace)
{						
	psy_ui_component_init(wireview_base(self), parent, view);
	vtable_init(self);
	self->wire = wire;
	self->workspace = workspace;
	self->scope_spec_mode = 0.2f;
	self->scope_spec_rate = 0.f;	
	psy_ui_component_set_align(wireview_base(self),
		psy_ui_ALIGN_CLIENT);	
	wireview_init_volume_slider(self);
	wireview_init_bottom_group(self);
	wireview_init_rate_group(self);
	wireview_init_tab_bar(self);	
	psy_ui_notebook_init(&self->notebook, wireview_base(self));
	psy_ui_component_set_align(psy_ui_notebook_base(&self->notebook),
		psy_ui_ALIGN_CLIENT);
	/* Vuscope */
	vuscope_init(&self->vuscope, psy_ui_notebook_base(&self->notebook), wire,
		workspace);	
	/* Oscilloscope */
	oscilloscopeview_init(&self->oscilloscopeview,
		psy_ui_notebook_base(&self->notebook), wire, workspace);
	oscilloscope_set_zoom(&self->oscilloscopeview.oscilloscope, 0.2f);	
	/* Spectrum */
	psy_ui_component_init(&self->spectrumpane,
		psy_ui_notebook_base(&self->notebook), NULL);
	spectrumanalyzer_init(&self->spectrumanalyzer, &self->spectrumpane,
		wire, workspace);
	psy_ui_component_set_align(&self->spectrumanalyzer.component,
		psy_ui_ALIGN_CENTER);
	/* Stereophase */
	psy_ui_component_init(&self->stereophasepane,
		psy_ui_notebook_base(&self->notebook), NULL);	
	stereophase_init(&self->stereophase, &self->stereophasepane, wire,
		workspace);
	psy_ui_component_set_align(&self->stereophase.component,
		psy_ui_ALIGN_CENTER);
	/* Channel Mapping */
	channelmappingview_init(&self->channelmappingview,
		psy_ui_notebook_base(&self->notebook), wire, workspace);
	psy_ui_notebook_connect_controller(&self->notebook,
		&self->tabbar.signal_change);
	psy_ui_tabbar_select(&self->tabbar, WIREVIEW_TAB_VUMETER);
	psy_signal_connect(&self->component.signal_timer, self, wireview_on_timer);
	if (workspace_song(self->workspace)) {
		wireview_update_title(self, psy_audio_song_machines(
			workspace_song(self->workspace)));
	}
	psy_ui_component_start_timer(&self->component, 0, 50);
}

void wireview_init_tab_bar(WireView* self)
{
	psy_ui_component_init(&self->top, &self->component, NULL);
	psy_ui_component_set_align(&self->top, psy_ui_ALIGN_TOP);
	psy_ui_component_set_align_expand(&self->top, psy_ui_HEXPAND);	
	psy_ui_tabbar_init(&self->tabbar, &self->top);
	psy_ui_tabbar_append_tabs(&self->tabbar, "Vu", "Osc", "Spectrum", "Stereo Phase",
		"Channel Mapping", NULL);
	psy_ui_component_set_align(psy_ui_tabbar_base(&self->tabbar), psy_ui_ALIGN_LEFT);	
}

void wireview_init_volume_slider(WireView* self)
{
	psy_ui_component_init(&self->slidergroup, wireview_base(self), NULL);	
	psy_ui_component_set_align(&self->slidergroup, psy_ui_ALIGN_RIGHT);
	psy_ui_component_set_margin(&self->slidergroup,
		psy_ui_margin_make_em(0.0, 0.0, 0.0, 2.0));	
	psy_ui_button_init(&self->percvol, &self->slidergroup);
	psy_ui_button_set_text(&self->percvol, "100%");
	psy_ui_button_prevent_translation(&self->percvol);
	psy_ui_button_set_char_number(&self->percvol, 6.0);	
	psy_ui_component_set_align(&self->percvol.component, psy_ui_ALIGN_BOTTOM);
	psy_ui_button_init(&self->dbvol, &self->slidergroup);	
	psy_ui_button_set_text(&self->dbvol, "db 100");
	psy_ui_button_prevent_translation(&self->dbvol);
	psy_ui_button_set_char_number(&self->dbvol, 6.0);	
	psy_ui_component_set_align(&self->dbvol.component, psy_ui_ALIGN_BOTTOM);
	psy_ui_slider_init(&self->volslider, &self->slidergroup);
	psy_ui_slider_set_char_number(&self->volslider, 0.0);
	psy_ui_component_hide(&self->volslider.value.component);
	psy_ui_slider_show_vertical(&self->volslider);
	psy_ui_component_resize(&self->volslider.component,
		psy_ui_size_make_em(2.0, 0.0));
	psy_ui_component_set_align(&self->volslider.component, psy_ui_ALIGN_CLIENT);	
	psy_ui_slider_connect(&self->volslider, self,
		(ui_slider_fpdescribe)wireview_on_describe_volume,
		(ui_slider_fptweak)wireview_on_tweak_volume,
		(ui_slider_fpvalue)wireview_on_value_volume);	
	psy_ui_slider_start_poll(&self->volslider);
}

void wireview_init_rate_group(WireView* self)
{
	psy_ui_Margin margin;

	psy_ui_margin_init_em(&margin, 0.5, 0.0, 0.5, 0.0);
	psy_ui_component_init(&self->rategroup, wireview_base(self), NULL);
	psy_ui_component_set_align(&self->rategroup, psy_ui_ALIGN_BOTTOM);
	psy_ui_component_set_margin(&self->rategroup, margin);
	psy_ui_button_init_connect(&self->hold, &self->rategroup,
		self, wireview_on_hold);
	psy_ui_button_set_text(&self->hold, "Hold");
	psy_ui_component_set_align(&self->hold.component, psy_ui_ALIGN_RIGHT);
	psy_ui_slider_init(&self->modeslider, &self->rategroup);
	psy_ui_slider_set_default_value(&self->modeslider, 0.2);
	psy_ui_slider_show_horizontal(&self->modeslider);
	psy_ui_slider_hide_value_label(&self->modeslider);
	psy_ui_component_set_margin(&self->modeslider.component, margin);
	psy_ui_component_set_align(&self->modeslider.component, psy_ui_ALIGN_TOP);
	psy_ui_slider_connect(&self->modeslider, self,
		(ui_slider_fpdescribe)NULL,
		(ui_slider_fptweak)wireview_on_tweak_mode,
		(ui_slider_fpvalue)wireview_on_value_mode);
	psy_ui_slider_init(&self->rateslider, &self->rategroup);
	psy_ui_slider_set_default_value(&self->rateslider, 0.0);
	psy_ui_slider_show_horizontal(&self->rateslider);	
	psy_ui_slider_show_horizontal(&self->rateslider);
	psy_ui_slider_hide_value_label(&self->rateslider);
	psy_ui_component_set_align(&self->rateslider.component, psy_ui_ALIGN_TOP);
	psy_ui_slider_connect(&self->rateslider, self,
		(ui_slider_fpdescribe)NULL,		
		(ui_slider_fptweak)wireview_on_tweak_rate,
		(ui_slider_fpvalue)wireview_on_value_rate);
	psy_ui_slider_update(&self->rateslider);
	psy_ui_slider_update(&self->modeslider);
}

void wireview_init_bottom_group(WireView* self)
{
	psy_ui_component_init(&self->bottomgroup, wireview_base(self), NULL);
	psy_ui_component_set_align(&self->bottomgroup, psy_ui_ALIGN_BOTTOM);
	psy_ui_component_set_default_align(&self->bottomgroup, psy_ui_ALIGN_LEFT,
		psy_ui_defaults_hmargin(psy_ui_defaults()));
	psy_ui_button_init_connect(&self->deletewire, &self->bottomgroup,
		self, wireview_on_delete_connection);
	psy_ui_button_set_text(&self->deletewire, "Delete Connection");
	psy_ui_button_init_connect(&self->addeffect, &self->bottomgroup,
		self, wireview_on_add_effect);	
	psy_ui_button_set_text(&self->addeffect, "Add Effect");
}

void wireview_update_title(WireView* self, psy_audio_Machines* machines)
{
	char title[128];
	psy_audio_Machine* src;
	psy_audio_Machine* dst;
	
	src = psy_audio_machines_at(machines, self->wire.src);
	dst = psy_audio_machines_at(machines, self->wire.dst);
	psy_snprintf(title, 128, "[%d] %s -> %s Connection Volume",
		(int)psy_audio_connections_wireindex(&machines->connections,
			self->wire),
		(src) ? psy_audio_machine_edit_name(src) : "ERR",
		(dst) ? psy_audio_machine_edit_name(dst) : "ERR");
	psy_ui_component_set_title(&self->component, title);
}

void wireview_on_describe_volume(WireView* self, psy_ui_Slider* slider, char* txt)
{
	psy_audio_Connections* connections;
	char text[128];
	double volume;

	connections = &workspace_song(self->workspace)->machines_.connections;
	volume = psy_audio_connections_wire_volume(connections, self->wire);	
	psy_snprintf(text, 128, "%.1f dB", psy_dsp_convert_amp_to_db(volume));
	psy_ui_button_set_text(&self->dbvol, text);
	psy_snprintf(text, 128, "%.2f %%", (volume * 100.0));
	psy_ui_button_set_text(&self->percvol, text);	
}

void wireview_on_tweak_volume(WireView* self, psy_ui_Slider* slider, double value)
{
	psy_audio_Connections* connections;	

	connections = &workspace_song(self->workspace)->machines_.connections;
	psy_audio_connections_set_wire_volume(connections, self->wire,
		value * value * 4.0);
}

void wireview_on_value_volume(WireView* self, psy_ui_Slider* slider, double* value)
{
	psy_audio_Connections* connections;	

	connections = &workspace_song(self->workspace)->machines_.connections;
		*value = sqrt(psy_audio_connections_wire_volume(connections, self->wire)) * 0.5;
}

void wireview_on_tweak_mode(WireView* self, psy_ui_Slider* slider, double value)
{
	self->scope_spec_mode = value;
	oscilloscope_set_zoom(&self->oscilloscopeview.oscilloscope, value);
}

void wireview_on_value_mode(WireView* self, psy_ui_Slider* slider, double* value)
{
	*value = self->scope_spec_mode;
}

void wireview_on_tweak_rate(WireView* self, psy_ui_Slider* slider, double value)
{
	self->scope_spec_rate = value;
	oscilloscope_set_begin(&self->oscilloscopeview.oscilloscope, value);
}

void wireview_on_value_rate(WireView* self, psy_ui_Slider* slider, double* value)
{
	*value = self->scope_spec_rate;
}

void wireview_on_hold(WireView* self, psy_ui_Component* sender)
{	
	switch (psy_ui_tabbar_selected(&self->tabbar)) {
		case WIREVIEW_TAB_VUMETER:
		break;
		case WIREVIEW_TAB_OSCILLOSCOPE:
			if (oscilloscope_stopped(&self->oscilloscopeview.oscilloscope)) {
				oscilloscope_continue(&self->oscilloscopeview.oscilloscope);
			} else {
				self->scope_spec_rate = 0.0;
				oscilloscope_set_begin(&self->oscilloscopeview.oscilloscope,
					self->scope_spec_rate);
				oscilloscope_hold(&self->oscilloscopeview.oscilloscope);
			}
		break;
		case WIREVIEW_TAB_SPECTRUM:
			if (spectrumanalyzer_stopped(&self->spectrumanalyzer)) {
				spectrumanalyzer_continue(&self->spectrumanalyzer);
			} else {								
				spectrumanalyzer_hold(&self->spectrumanalyzer);
			}
		break;
		case WIREVIEW_TAB_PHASE:
		break;
		default:
		break;
	}
	if (psy_ui_button_highlighted(&self->hold)) {
		psy_ui_button_disable_highlight(&self->hold);
	} else {
		psy_ui_button_highlight(&self->hold);
	}
}

void wireview_on_delete_connection(WireView* self, psy_ui_Component* sender)
{
	if (self->workspace && workspace_song(self->workspace)) {		
		psy_audio_machines_disconnect(&workspace_song(self->workspace)->machines_,
			self->wire);
	}
}

void wireview_on_add_effect(WireView* self, psy_ui_Component* sender)
{
	if (self->workspace && workspace_song(self->workspace)) {
		machineinsert_append(&self->workspace->insert, self->wire);
		workspace_select_view(self->workspace,
			viewindex_make_all(
				VIEW_ID_MACHINES,
				SECTION_ID_MACHINEVIEW_NEWMACHINE,
				psy_INDEX_INVALID,
				psy_INDEX_INVALID));
	}
}

bool wireview_wire_exists(const WireView* self)
{
	return workspace_song(self->workspace) &&
		psy_audio_machines_connected(
			psy_audio_song_machines(workspace_song(self->workspace)),
			self->wire);
}

psy_ui_Component* wireview_scope(WireView* self, int index)
{
	psy_ui_Component* rv;

	switch (index) {
		case WIREVIEW_TAB_VUMETER:
			rv = &self->vuscope.component;
		break;
		case WIREVIEW_TAB_OSCILLOSCOPE:
			rv = &self->oscilloscopeview.component;
		break;
		case WIREVIEW_TAB_SPECTRUM:
			rv = &self->spectrumanalyzer.component;
		break;
		case WIREVIEW_TAB_PHASE:
			rv = &self->stereophase.component;
		break;
		default:
			rv = 0;
		break;
	}
	return rv;
}

uintptr_t wireview_curr_scope(WireView* self)
{	
	return psy_ui_tabbar_selected(&self->tabbar);
}

void wireview_on_timer(WireView* self, uintptr_t timerid)
{	
	switch (psy_ui_notebook_page_index(&self->notebook)) {
	case 0:		
		/* slider vu needs vuscope values */
		break;
	case 1:		
		oscilloscopeview_idle(&self->oscilloscopeview);		
		break;
	case 2:
		spectrumanalyzer_idle(&self->spectrumanalyzer);
		break;
	case 3:
		stereophase_idle(&self->stereophase);
		break;
	default:
		break;
	}
	vuscope_idle(&self->vuscope);
	psy_ui_component_invalidate(&self->volslider.pane.component);
}


/* WireFrame */

/* implementation */
void wireframe_init(WireFrame* self, psy_ui_Component* parent,
	psy_audio_Wire wire, Workspace* workspace)
{	
	assert(workspace);
	assert(workspace->song);
	assert(psy_audio_wire_valid(&wire));

	assert(self);

	psy_ui_toolframe_init(wireframe_base(self), (parent->view)
		? parent->view : parent);
	psy_ui_component_doublebuffer(wireframe_base(self));
	psy_ui_component_set_icon_ressource(wireframe_base(self), IDI_MACPARAM);
	wireview_init(&self->wire_view, wireframe_base(self),
		wireframe_base(self), wire, workspace);
	psy_ui_component_set_title(wireframe_base(self), 
		psy_ui_component_title(wireview_base(&self->wire_view)));
	psy_ui_component_set_position(wireframe_base(self),
		psy_ui_rectangle_make(
			psy_ui_point_make_em(18.0, 15.0),
			psy_ui_size_make_em(80.0, 25.0)));	
}
