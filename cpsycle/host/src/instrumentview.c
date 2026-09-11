/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "instrumentview.h"
/* host */
#include "styles.h"
/* audio */
#include <exclusivelock.h>
#include <instruments.h>
#include <songio.h>
#include <machinefactory.h>
#include <virtualgenerator.h>
#include <xmsongloader.h>
/* dsp */
#include <valuemapper.h>
/* std */
#include <math.h>
/* platform */
#include "../../detail/portable.h"


/* VirtualGeneratorsBox */

/* prototypes */
static void virtualgeneratorbox_update_generator(VirtualGeneratorsBox*);
static void virtualgeneratorbox_on_generators_changed(VirtualGeneratorsBox*,
	psy_ui_ComboBox* sender);
static void virtualgeneratorbox_on_samplers_changed(VirtualGeneratorsBox*,
	psy_ui_ComboBox* sender);
static void virtualgeneratorbox_on_active_changed(VirtualGeneratorsBox*,
	psy_ui_Component* sender);

/* implementation */
void virtualgeneratorbox_init(VirtualGeneratorsBox* self,
	psy_ui_Component* parent, Workspace* workspace)
{	
	psy_ui_component_init(&self->component, parent, NULL);
	self->workspace = workspace;
	psy_ui_component_set_default_align(&self->component, psy_ui_ALIGN_LEFT,
		psy_ui_defaults_hmargin(psy_ui_defaults()));
	psy_ui_checkbox_init(&self->active, &self->component);
	psy_ui_checkbox_set_text(&self->active, "Virtual generator");
	psy_signal_connect(&self->active.signal_clicked, self,
		virtualgeneratorbox_on_active_changed);
	psy_ui_combobox_init(&self->generators, &self->component);
	psy_signal_connect(&self->generators.signal_selchanged, self,
		virtualgeneratorbox_on_generators_changed);
	psy_ui_label_init_text(&self->on, &self->component, "on");
	psy_ui_combobox_set_char_number(&self->generators, 10);
	psy_ui_combobox_init(&self->samplers, &self->component);
	psy_signal_connect(&self->samplers.signal_selchanged, self,
		virtualgeneratorbox_on_samplers_changed);
	psy_ui_combobox_set_char_number(&self->samplers, 20);
	virtualgeneratorbox_update_generators(self);
}

void virtualgeneratorbox_update_samplers(VirtualGeneratorsBox* self)
{
	if (self->workspace->song) {
		uintptr_t i;
		uintptr_t c;
		uintptr_t maxkey;

		maxkey = psy_table_maxkey(&self->workspace->song->machines_.slots);
		for (c = 0, i = 0; i < maxkey; ++i) {
			psy_audio_Machine* machine;

			machine = (psy_audio_Machine*)psy_audio_machines_at(
				&self->workspace->song->machines_, i);
			if (machine && machine_supports(machine,
				psy_audio_SUPPORTS_INSTRUMENTS)) {
				char text[512];

				psy_snprintf(text, 512, "%X: %s", (int)i,
					psy_audio_machine_edit_name(machine));
				psy_ui_combobox_add_text(&self->samplers, text);
				psy_ui_combobox_set_item_id(&self->samplers,  c++, i);
			}
		}
	}
}

void virtualgeneratorbox_update_generators(VirtualGeneratorsBox* self)
{
	uintptr_t slot;
	uintptr_t start;
	uintptr_t end;

	start = 0x81;
	end = 0xFE;
	for (slot = start; slot <= end; ++slot) {
		char text[512];

		psy_snprintf(text, 512, "%X", (int)slot);
		psy_ui_combobox_add_text(&self->generators, text);
		psy_ui_combobox_set_item_id(&self->generators, slot - start, slot);
	}
}

void virtualgeneratorbox_on_generators_changed(VirtualGeneratorsBox* self,
	psy_ui_ComboBox* sender)
{
	virtualgeneratorbox_update_generator(self);
}

void virtualgeneratorbox_on_samplers_changed(VirtualGeneratorsBox* self,
	psy_ui_ComboBox* sender)
{
	virtualgeneratorbox_update_generator(self);
}

void virtualgeneratorbox_on_active_changed(VirtualGeneratorsBox* self,
	psy_ui_Component* sender)
{
	virtualgeneratorbox_update_generator(self);
}

void virtualgeneratorbox_update_generator(VirtualGeneratorsBox* self)
{
	if (psy_ui_checkbox_checked(&self->active)) {
		if (psy_ui_combobox_cur_sel(&self->generators) != psy_INDEX_INVALID &&
				psy_ui_combobox_cur_sel(&self->samplers) != psy_INDEX_INVALID) {			
			psy_audio_song_insert_virtual_generator(self->workspace->song,
				psy_ui_combobox_cur_sel(&self->generators) + 0x81,
					psy_ui_combobox_item_id(&self->samplers,
					psy_ui_combobox_cur_sel(&self->samplers)),
				psy_audio_instruments_selected(
					&self->workspace->song->instruments_).subslot);
		}
	} else if (psy_ui_combobox_cur_sel(&self->generators) != psy_INDEX_INVALID) {
		psy_audio_Machine* generator;

		generator = psy_audio_machines_at(&self->workspace->song->machines_,
			psy_ui_combobox_cur_sel(&self->generators) + 0x81);
		if (generator) {
			psy_audio_machines_remove(&self->workspace->song->machines_,
				psy_ui_combobox_cur_sel(&self->generators) + 0x81, FALSE);
		}		
	}	
}

void virtualgeneratorbox_update(VirtualGeneratorsBox* self)
{
	psy_TableIterator it;	

	psy_ui_combobox_select(&self->generators, psy_INDEX_INVALID);
	psy_ui_combobox_select(&self->samplers, psy_INDEX_INVALID);
	psy_ui_checkbox_disable_check(&self->active);
	for (it = psy_audio_machines_begin(&self->workspace->song->machines_);
		!psy_tableiterator_equal(&it, psy_table_end());
		psy_tableiterator_inc(&it)) {
		psy_audio_Machine* machine;

		machine = (psy_audio_Machine*)psy_tableiterator_value(&it);
		if (psy_audio_machine_type(machine) == psy_audio_VIRTUALGENERATOR) {
			psy_audio_MachineParam* param;
			
			param = psy_audio_machine_parameter(machine, 0);
			if (param) {
				intptr_t instindex;

				instindex = psy_audio_machine_parameter_scaled_value(machine,
					param);
				if (instindex == psy_audio_instruments_selected(
						&self->workspace->song->instruments_).subslot) {
					param = psy_audio_machine_parameter(machine, 1);
					if (param) {
						uintptr_t macindex;

						macindex = psy_audio_machine_parameter_scaled_value(
							machine, param);
						psy_ui_combobox_select(&self->samplers, macindex);					
					}
					psy_ui_combobox_select(&self->generators,
						psy_audio_machine_slot(machine) - 0x81);
					psy_ui_checkbox_check(&self->active);
					break;
				}
			} 
		}		
	}	
}


/* InstrumentPredefsBar */

/* prototypes */
static void instrumentpredefsbar_on_predefs(InstrumentPredefsBar* self,
	psy_ui_Button* sender);
	
/* implementation */
void instrumentpredefsbar_init(InstrumentPredefsBar* self,
	psy_ui_Component* parent, psy_audio_Instrument* instrument,
	InstrumentView* view, Workspace* workspace)
{
	psy_ui_Button* buttons[] = { &self->predef_1, &self->predef_2,
		&self->predef_3, &self->predef_4, &self->predef_5, &self->predef_6,
		NULL};	
	uintptr_t c;

	psy_ui_component_init(&self->component, parent, NULL);
	self->instrument = instrument;
	self->workspace = workspace;
	self->view = view;
	psy_ui_component_set_default_align(&self->component, psy_ui_ALIGN_LEFT,
		psy_ui_defaults_hmargin(psy_ui_defaults()));
	psy_ui_component_set_align_expand(&self->component,
		psy_ui_HEXPAND);
	psy_ui_label_init(&self->predefs, &self->component);	
	psy_ui_label_prevent_translation(&self->predefs);
	psy_ui_label_set_text(&self->predefs, "Predef.");	
	for (c = 0; buttons[c] != NULL; ++c) {
		char text[2];

		psy_snprintf(text, 2, "%i", (int)c + 1);
		psy_ui_button_init_text(buttons[c], &self->component, text);
		psy_ui_component_set_id(psy_ui_button_base(buttons[c]), c);
		psy_ui_button_allow_right_click(buttons[c]);		
		psy_signal_connect(&buttons[c]->signal_clicked, self,
			instrumentpredefsbar_on_predefs);		
	}	
}

void instrumentpredefsbar_on_predefs(InstrumentPredefsBar* self,
	psy_ui_Button* sender)
{
	if (self->instrument) {		
		uintptr_t index;		
				
		index = psy_ui_component_id(psy_ui_button_base(sender));
		if (index == psy_INDEX_INVALID) {
			return;
		}
		if (psy_ui_button_click_state(sender) == 1) {
			predefsconfig_predef(
				psycleconfig_predefs(workspace_cfg(self->workspace)),
				index, &self->instrument->volumeenvelope);		
		} else {
			predefsconfig_store_predef(
				psycleconfig_predefs(workspace_cfg(self->workspace)),
				index, &self->instrument->volumeenvelope);		
		}
		psy_ui_component_invalidate(&self->view->component);
	}
}


/* InstrumentHeaderView */

/* prototypes */
static void instrumentheaderview_on_prev_instrument(InstrumentHeaderView*,
	psy_ui_Component* sender);
static void instrumentheaderview_on_next_instrument(InstrumentHeaderView*,
	psy_ui_Component* sender);
static void instrumentheaderview_on_edit_accept(InstrumentHeaderView*,
	psy_ui_Edit* sender);
static void instrumentheaderview_on_edit_reject(InstrumentHeaderView*,
	psy_ui_Edit* sender);

/* implementation */
void instrumentheaderview_init(InstrumentHeaderView* self,
	psy_ui_Component* parent, psy_audio_Instruments* instruments,
	InstrumentView* view, Workspace* workspace)
{
	self->view = view;
	self->instrument = NULL;
	self->instruments = instruments;
	psy_ui_component_init(&self->component, parent, NULL);
	psy_ui_component_set_style_type(&self->component, STYLE_VIEW_HEADER);
	psy_ui_component_set_default_align(&self->component, psy_ui_ALIGN_LEFT,
		psy_ui_defaults_hmargin(psy_ui_defaults()));
	psy_ui_label_init_text(&self->namelabel, &self->component,
		"instview.instrument-name");
	psy_ui_edit_init(&self->nameedit, &self->component);
	psy_ui_edit_enable_input_field(&self->nameedit);
	psy_ui_edit_set_char_number(&self->nameedit, 20);
	psy_signal_connect(&self->nameedit.pane.signal_accept, self,
		instrumentheaderview_on_edit_accept);	
	psy_signal_connect(&self->nameedit.pane.signal_reject, self,
		instrumentheaderview_on_edit_reject);
	psy_ui_button_init_resource_connect(&self->prevbutton, &self->component,
		"img.icon-less", self, instrumentheaderview_on_prev_instrument);
	psy_ui_button_init_resource_connect(&self->nextbutton, &self->component,
		"img.icon-more", self, instrumentheaderview_on_next_instrument);
	psy_ui_component_init(&self->more, &self->component, NULL);
	psy_ui_component_set_default_align(&self->more, psy_ui_ALIGN_LEFT,
		psy_ui_defaults_hmargin(psy_ui_defaults()));
	virtualgeneratorbox_init(&self->virtualgenerators, &self->more,
		workspace);
	instrumentpredefsbar_init(&self->predefs, &self->more, NULL,
		view, workspace);	
	psy_ui_component_hide(&self->more);	
}

void instrumentheaderview_set_instrument(InstrumentHeaderView* self,
	psy_audio_Instrument* instrument)
{
	self->instrument = instrument;
	self->predefs.instrument = instrument;	
	psy_ui_edit_set_text(&self->nameedit,
		(instrument)
		? instrument->name
		: "");	
	if (instrument) {
		psy_ui_component_show_align(&self->more);
	} else {
		psy_ui_component_hide(&self->more);
	}
}

void instrumentheaderview_on_edit_accept(InstrumentHeaderView* self,
	psy_ui_Edit* sender)
{
	if (self->instrument) {
		char text[40];
		psy_audio_InstrumentIndex index;
		
		index = instrumentsbox_selected(&self->view->instrumentsbox);
		if (psy_strlen(psy_ui_edit_text(sender)) == 0) {
			psy_ui_edit_set_text(sender, "Untitled");
		}
		psy_audio_instrument_set_name(self->instrument,
			psy_ui_edit_text(sender));
		psy_snprintf(text, 20, "%02X:%s", 
			(int)index.subslot, psy_audio_instrument_name(self->instrument));
		psy_ui_listbox_set_text(&self->view->instrumentsbox.instrumentlist,
			text, index.subslot);		
	} else if (psy_strlen(psy_ui_edit_text(sender)) > 0) {
		if (workspace_song(self->view->workspace)) {
			psy_audio_Instrument* instrument;
			psy_audio_InstrumentIndex selected;

			selected = instrumentsbox_selected(&self->view->instrumentsbox);
			instrument = psy_audio_instrument_allocinit();
			psy_audio_instrument_set_name(instrument,
				psy_ui_edit_text(sender));
			psy_audio_instrument_setindex(instrument, selected.subslot);
			psy_audio_exclusivelock_enter();
			psy_audio_instruments_insert(
				&workspace_song(self->view->workspace)->instruments_, instrument,
				selected);
			psy_audio_instruments_select(
				&workspace_song(self->view->workspace)->instruments_, selected);
			psy_audio_exclusivelock_leave();
		}
	}
	psy_ui_component_set_focus(psy_ui_component_parent(&self->component));
}

void instrumentheaderview_on_edit_reject(InstrumentHeaderView* self,
	psy_ui_Edit* sender)
{
	if (self->instrument) {
		psy_ui_edit_set_text(&self->nameedit,
			(self->instrument)
			? self->instrument->name
			: "");
	}
	psy_ui_component_set_focus(psy_ui_component_parent(&self->component));
}

void instrumentheaderview_on_prev_instrument(InstrumentHeaderView* self,
	psy_ui_Component* sender)
{
	psy_audio_InstrumentIndex index;

	index = psy_audio_instruments_selected(self->instruments);
	if (!psy_audio_instrumentindex_invalid(&index) &&
			psy_audio_instrumentindex_subslot(&index) > 0) {
		index.subslot = psy_audio_instrumentindex_subslot(&index) - 1;
		psy_audio_instruments_select(self->instruments, index);
	}
}

void instrumentheaderview_on_next_instrument(InstrumentHeaderView* self,
	psy_ui_Component* sender)
{
	psy_audio_InstrumentIndex index;

	index = psy_audio_instruments_selected(self->instruments);
	if (!psy_audio_instrumentindex_invalid(&index)) {
		index.subslot = psy_audio_instrumentindex_subslot(&index) + 1;
		psy_audio_instruments_select(self->instruments, index);
	}
}


/* InstrumentViewButtons */

/* implementation */
void instrumentviewbuttons_init(InstrumentViewButtons* self,
	psy_ui_Component* parent)
{
	psy_ui_component_init(&self->component, parent, NULL);
	psy_ui_component_set_default_align(&self->component, psy_ui_ALIGN_LEFT,
		psy_ui_defaults_hmargin(psy_ui_defaults()));
	psy_ui_component_set_padding(&self->component,	
		psy_ui_margin_make_em(0.5, 0.0, 0.5, 0.0));
	psy_ui_button_init_text(&self->create, &self->component, "file.new");
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->create), 0);
	psy_ui_button_init_text(&self->load, &self->component, "file.load");
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->load), 0);
	psy_ui_button_init_text(&self->save, &self->component, "file.save");
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->save), 0);
	psy_ui_button_init_text(&self->duplicate, &self->component,
		"edit.duplicate");
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->duplicate), 0);
	psy_ui_button_init_text(&self->del, &self->component, "edit.delete");
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->del), 0);
}


/* InstrumentView */

/* prototypes */
static void instrumentview_on_destroyed(InstrumentView*);
static void instrumentview_on_create_instrument(InstrumentView*,
	psy_ui_Component* sender);
static void instrumentview_on_load_instrument_button(InstrumentView*,
	psy_ui_Component* sender);
static void instrumentview_on_load_instrument_changed(InstrumentView*,
	psy_FileSelect* sender);
static void instrumentview_load_instrument(InstrumentView*, const char* path);
static void instrumentview_on_save_instrument_button(InstrumentView*,
	psy_ui_Component* sender);
static void instrumentview_on_save_instrument_changed(InstrumentView*,
	psy_FileSelect* sender);
static void instrumentview_save_instrument(InstrumentView*, const char* path,
	psy_audio_Instrument* instrument);
static void instrumentview_on_delete_instrument(InstrumentView*,
	psy_ui_Component* sender);
static void instrumentview_on_instrument_insert(InstrumentView*,
	psy_audio_Instruments* sender,
	const psy_audio_InstrumentIndex* index);
static void instrumentview_on_instrument_removed(InstrumentView*,
	psy_audio_Instruments* sender,
	const psy_audio_InstrumentIndex* index);
static void instrumentview_on_instrument_slot_changed(InstrumentView*,
	psy_audio_Instrument* sender, const psy_audio_InstrumentIndex*);
static void instrumentview_set_instrument(InstrumentView*,
	psy_audio_InstrumentIndex index);
static void instrumentview_on_machines_insert(InstrumentView*,
	psy_audio_Machines* sender, int slot);
static void instrumentview_on_machines_removed(InstrumentView*,
	psy_audio_Machines* sender, int slot);
static void instrumentview_on_song_changed(InstrumentView*,
	psy_audio_Player* sender);
static void instrumentview_on_select_section(InstrumentView*,
	psy_ui_Component* sender, uintptr_t param1, uintptr_t param2);
static uintptr_t instrumentview_section(const InstrumentView*);
static void instrumentview_on_tabbar_changed(InstrumentView*,
	psy_ui_TabBar* sender, uintptr_t index);
	
/* vtable */
static psy_ui_ComponentVtable instrumentview_vtable;
static bool instrumentview_vtable_initialized = FALSE;

static void instrumentview_vtable_init(InstrumentView* self)
{
	if (!instrumentview_vtable_initialized) {
		instrumentview_vtable = *(self->component.vtable);		
		instrumentview_vtable.on_destroyed =
			(psy_ui_fp_component)
			instrumentview_on_destroyed;		
		instrumentview_vtable.section =
			(psy_ui_fp_component_section)
			instrumentview_section;
		instrumentview_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(instrumentview_base(self),
		&instrumentview_vtable);
}

/* implementation */
void instrumentview_init(InstrumentView* self, psy_ui_Component* parent,
	psy_ui_Component* tabbarparent, Workspace* workspace)
{
	assert(self);
	assert(workspace);

	psy_ui_component_init(&self->component, parent, NULL);
	instrumentview_vtable_init(self);	
	self->player = workspace_player(workspace);
	self->workspace = workspace;
	psy_ui_component_set_id(instrumentview_base(self), VIEW_ID_INSTRUMENTS);
	psy_ui_component_set_title(instrumentview_base(self), "main.instruments");
	psy_ui_component_set_style_type(&self->component, STYLE_CLIENT_VIEW);	
	psy_ui_component_init(&self->viewtabbar, tabbarparent, NULL);
	psy_ui_component_set_id(&self->viewtabbar, VIEW_ID_INSTRUMENTS);	
	/* header */
	instrumentheaderview_init(&self->header, &self->component,
		&workspace->song->instruments_, self, workspace);
	psy_ui_component_set_align(&self->header.component, psy_ui_ALIGN_TOP);
	/* left */
	psy_ui_component_init(&self->left, &self->component, NULL);
	psy_ui_component_set_align(&self->left, psy_ui_ALIGN_LEFT);	
	instrumentviewbuttons_init(&self->buttons, &self->left);
	psy_ui_component_set_align(&self->buttons.component, psy_ui_ALIGN_TOP);
	instrumentsbox_init(&self->instrumentsbox, &self->left, (&workspace->song)
		? &workspace->song->instruments_
		: NULL);
	psy_ui_component_set_margin(&self->instrumentsbox.component,
		psy_ui_margin_make_em(0.25, 0.0, 0.0, 0.0));
	psy_ui_component_set_align(&self->instrumentsbox.component,
		psy_ui_ALIGN_CLIENT);	
	/* client notebook */
	psy_ui_notebook_init(&self->clientnotebook, &self->component);
	psy_ui_component_set_align(psy_ui_notebook_base(&self->clientnotebook),
		psy_ui_ALIGN_CLIENT);
	psy_ui_component_set_margin(psy_ui_notebook_base(&self->clientnotebook),
		psy_ui_margin_make_em(0.30, 2.0, 0.0, 2.0));
	/* empty */
	psy_ui_label_init_text(&self->empty,  psy_ui_notebook_base(
		&self->clientnotebook), "instview.empty");
	psy_ui_label_set_text_alignment(&self->empty, psy_ui_ALIGNMENT_CENTER);	
	/* client */
	psy_ui_component_init(&self->client, psy_ui_notebook_base(
		&self->clientnotebook), NULL);	
	psy_ui_component_set_align(&self->client, psy_ui_ALIGN_CLIENT);
	psy_ui_tabbar_init(&self->tabbar, &self->client);
	psy_ui_component_set_align(psy_ui_tabbar_base(&self->tabbar),
		psy_ui_ALIGN_TOP);	
	psy_ui_tabbar_append_tabs(&self->tabbar, "instview.general",
		"instview.volume", "instview.pan", "instview.filter",
		"instview.pitch", NULL);
	psy_signal_connect(&self->tabbar.signal_change, self,
		instrumentview_on_tabbar_changed);
	psy_ui_notebook_init(&self->notebook, &self->client);
	psy_ui_component_set_align(psy_ui_notebook_base(&self->notebook),
		psy_ui_ALIGN_CLIENT);
	psy_ui_component_set_margin(psy_ui_notebook_base(&self->notebook),
		psy_ui_margin_make_em(1.0, 0.0, 0.0, 0.0));
	instrumentgeneralview_init(&self->general,
		psy_ui_notebook_base(&self->notebook), &workspace->song->instruments_,
		workspace);
	instrumentvolumeview_init(&self->volume,
		psy_ui_notebook_base(&self->notebook), &workspace->song->instruments_);
	instrumentpanview_init(&self->pan, psy_ui_notebook_base(&self->notebook),
		&workspace->song->instruments_, workspace);
	instrumentfilterview_init(&self->filter,
		psy_ui_notebook_base(&self->notebook),
		&workspace->song->instruments_);	
	instrumentpitchview_init(&self->pitch,
		psy_ui_notebook_base(&self->notebook),
		&workspace->song->instruments_);	
	/* connect signals */	
	psy_signal_connect(
		&workspace_song(self->workspace)->instruments_.signal_insert, self,
		instrumentview_on_instrument_insert);
	psy_signal_connect(
		&workspace_song(self->workspace)->instruments_.signal_removed, self,
		instrumentview_on_instrument_removed);
	psy_signal_connect(&workspace_song(
		self->workspace)->instruments_.signal_slotchange, self,
		instrumentview_on_instrument_slot_changed);
	psy_signal_connect(
		&workspace_song(self->workspace)->machines_.signal_insert, self,
		instrumentview_on_machines_insert);
	psy_signal_connect(
		&workspace_song(self->workspace)->machines_.signal_removed, self,
		instrumentview_on_machines_removed);
	psy_ui_notebook_select(&self->notebook, 0);
	psy_signal_connect(&workspace->player_.signal_song_changed, self,
		instrumentview_on_song_changed);
	samplesbox_set_samples(
		&self->general.notemapview.samplesbox, &workspace->song->samples_);
	psy_signal_connect(&self->buttons.create.signal_clicked, self,
		instrumentview_on_create_instrument);
	psy_signal_connect(&self->buttons.load.signal_clicked, self,
		instrumentview_on_load_instrument_button);
	psy_signal_connect(&self->buttons.save.signal_clicked, self,
		instrumentview_on_save_instrument_button);
	psy_signal_connect(&self->buttons.del.signal_clicked, self,
		instrumentview_on_delete_instrument);	
	psy_signal_connect(&self->component.signal_select_section, self,
		instrumentview_on_select_section);
	psy_ui_tabbar_select(&self->tabbar, 0);
	instrumentview_set_instrument(self,
		psy_audio_instrumentindex_make_invalid());	
}

void instrumentview_on_destroyed(InstrumentView* self)
{
	assert(self);
	
}

void instrumentview_on_instrument_insert(InstrumentView* self,
	psy_audio_Instruments* sender, const psy_audio_InstrumentIndex* index)
{
	if (index) {
		instrumentview_set_instrument(self, *index);
	}
}

void instrumentview_on_instrument_removed(InstrumentView* self,
	psy_audio_Instruments* sender, const psy_audio_InstrumentIndex* index)
{
	if (index) {
		instrumentview_set_instrument(self, *index);
	}
}

void instrumentview_on_instrument_slot_changed(InstrumentView* self,
	psy_audio_Instrument* sender, const psy_audio_InstrumentIndex* index)
{
	if (index) {
		instrumentview_set_instrument(self, *index);
	}
}

void instrumentview_on_machines_insert(InstrumentView* self,
	psy_audio_Machines* sender, int slot)
{
	virtualgeneratorbox_update_samplers(&self->header.virtualgenerators);
}

void instrumentview_on_machines_removed(InstrumentView* self,
	psy_audio_Machines* sender, int slot)
{
	virtualgeneratorbox_update_samplers(&self->header.virtualgenerators);
	virtualgeneratorbox_update(&self->header.virtualgenerators);
}

void instrumentview_set_instrument(InstrumentView* self,
	psy_audio_InstrumentIndex index)
{
	psy_audio_Instrument* instrument;

	instrument = psy_audio_instruments_at(
		&workspace_song(self->workspace)->instruments_, index);
	instrumentheaderview_set_instrument(&self->header, instrument);
	instrumentgeneralview_setinstrument(&self->general, instrument);
	instrumentvolumeview_set_instrument(&self->volume, instrument);
	instrumentpanview_set_instrument(&self->pan, instrument);
	instrumentfilterview_set_instrument(&self->filter, instrument);	
	instrumentpitchview_set_instrument(&self->pitch, instrument);
	virtualgeneratorbox_update(&self->header.virtualgenerators);
	if (instrument) {
		psy_ui_notebook_select(&self->clientnotebook, 1);
	} else {		
		psy_ui_notebook_select(&self->clientnotebook, 0);
	}
}

void instrumentview_on_song_changed(InstrumentView* self,
	psy_audio_Player* sender)
{
	psy_audio_Song* song;
	
	song = psy_audio_player_song(sender);
	if (song) {
		self->header.instruments = &song->instruments_;
		self->general.instruments = &song->instruments_;
		self->volume.instruments = &song->instruments_;
		self->pan.instruments = &song->instruments_;
		self->filter.instruments = &song->instruments_;
		psy_signal_connect(&song->instruments_.signal_slotchange, self,
			instrumentview_on_instrument_slot_changed);
		psy_signal_connect(&song->instruments_.signal_insert, self,
			instrumentview_on_instrument_insert);
		psy_signal_connect(&song->instruments_.signal_removed, self,
			instrumentview_on_instrument_removed);
		psy_signal_connect(&song->machines_.signal_insert,
			self, instrumentview_on_machines_insert);
		psy_signal_connect(&song->machines_.signal_removed,
			self, instrumentview_on_machines_removed);
		instrumentsbox_set_instruments(&self->instrumentsbox,
			&song->instruments_);
		samplesbox_set_samples(&self->general.notemapview.samplesbox,
			&song->samples_);
	} else {
		instrumentsbox_set_instruments(&self->instrumentsbox, 0);
		samplesbox_set_samples(&self->general.notemapview.samplesbox, NULL);
	}
	virtualgeneratorbox_update_samplers(&self->header.virtualgenerators);
	instrumentview_set_instrument(self, psy_audio_instrumentindex_make(0, 0));
}

void instrumentview_on_create_instrument(InstrumentView* self,
	psy_ui_Component* sender)
{
	if (workspace_song(self->workspace)) {
		psy_audio_Instrument* instrument;
		psy_audio_InstrumentIndex selected;

		selected = instrumentsbox_selected(&self->instrumentsbox);
		instrument = psy_audio_instrument_allocinit();
		psy_audio_instrument_set_name(instrument, "Untitled");
		psy_audio_instrument_setindex(instrument, selected.subslot);
		psy_audio_exclusivelock_enter();
		psy_audio_instruments_insert(
			&workspace_song(self->workspace)->instruments_, instrument,
			selected);
		psy_audio_instruments_select(
			&workspace_song(self->workspace)->instruments_, selected);
		psy_audio_exclusivelock_leave();
	}
}

void instrumentview_on_delete_instrument(InstrumentView* self,
	psy_ui_Component* sender)
{
	assert(self);

	if (!workspace_song(self->workspace)) {
		return;
	}
	psy_audio_exclusivelock_enter();
	psy_audio_instruments_remove(
		psy_audio_song_instruments(workspace_song(self->workspace)),
		instrumentsbox_selected(&self->instrumentsbox));
	psy_audio_exclusivelock_leave();
	psy_audio_machines_notify_aux_change(psy_audio_song_machines(
		workspace_song(self->workspace)));
}

void instrumentview_on_load_instrument_button(InstrumentView* self,
	psy_ui_Component* sender)
{
	psy_FileSelect load;
	
	assert(self);
	
	if (!workspace_song(self->workspace)) {
		return;
	}				
	psy_fileselect_init(&load);
	psy_fileselect_connect_change(&load, self,
		(psy_fp_fileselect_change)instrumentview_on_load_instrument_changed);
	psy_fileselect_set_title(&load, "Load Instrument");
	psy_fileselect_set_default_extension(&load,
		psy_audio_songfile_standardinstloadfilter());
	psy_fileselect_add_filter(&load, "pins", "*.pins");
	/* psy_audio_songfile_standardinstloadfilter() */
	workspace_disk_op(self->workspace, &load, NULL, NULL);		
	/* dirconfig_samples(&self->workspace->config.directories */
	psy_fileselect_dispose(&load);	
}

void instrumentview_on_load_instrument_changed(InstrumentView* self,
	psy_FileSelect* sender)
{
	instrumentview_load_instrument(self, psy_fileselect_value(sender));	
	workspace_restore_view(self->workspace);
}

void instrumentview_load_instrument(InstrumentView* self, const char* path)
{	
	psy_audio_SongFile songfile;	
	
	assert(self);
		
	if (!workspace_song(self->workspace)) {
		return;
	}
	psy_audio_songfile_init_song(&songfile, workspace_song(self->workspace));
	psy_audio_songfile_load_instrument(&songfile, path,
		psy_audio_instruments_selected(psy_audio_song_instruments(
			workspace_song(self->workspace))));	
	psy_audio_songfile_dispose(&songfile);
	instrumentsbox_rebuild(&self->instrumentsbox);
}

void instrumentview_on_save_instrument_button(InstrumentView* self,
	psy_ui_Component* sender)
{	
	psy_audio_Song* song;
	psy_audio_Instrument* instrument;
	psy_FileSelect save;

	assert(self);
	
	song = workspace_song(self->workspace);		
	if (!song) {
		return;
	}
	instrument = psy_audio_instruments_at(&song->instruments_,
		psy_audio_instruments_selected(&song->instruments_));
	if (!instrument) {
		return;
	}	
	psy_fileselect_init(&save);
	psy_fileselect_connect_change(&save, self,
		(psy_fp_fileselect_change)instrumentview_on_save_instrument_changed);
	psy_fileselect_set_title(&save, "Save Instrument");
	psy_fileselect_set_default_extension(&save,
		psy_audio_songfile_standardinstloadfilter());
	psy_fileselect_add_filter(&save, "pins", "*.pins");
	/* psy_audio_songfile_standardinstloadfilter() */
	workspace_disk_op(self->workspace, NULL, &save, NULL);
	/* dirconfig_samples(&self->workspace->config.directories */
	psy_fileselect_dispose(&save);	
}

void instrumentview_on_save_instrument_changed(InstrumentView* self,
	psy_FileSelect* sender)
{
	psy_audio_Song* song;	

	song = workspace_song(self->workspace);		
	if (song) {
		psy_audio_Instrument* instrument;
		
		instrument = psy_audio_instruments_at(psy_audio_song_instruments(song),
			psy_audio_instruments_selected(psy_audio_song_instruments(song)));
		if (instrument) {			
			instrumentview_save_instrument(self, psy_fileselect_value(sender),
				instrument);	
		}
	}
	workspace_restore_view(self->workspace);
}

void instrumentview_save_instrument(InstrumentView* self, const char* path,
	psy_audio_Instrument* instrument)
{
	assert(self);
	
	if (workspace_song(self->workspace)) {
		psy_audio_SongFile song_file;		

		psy_audio_songfile_init_song(&song_file, workspace_song(
			self->workspace));
		psy_audio_songfile_save_instrument(&song_file, path, instrument);
		psy_audio_songfile_dispose(&song_file);
	}
}

void instrumentview_on_tabbar_changed(InstrumentView* self,
	psy_ui_TabBar* sender, uintptr_t index)
{
	assert(self);
	
	if (index != psy_INDEX_INVALID) {
		workspace_select_view(self->workspace, viewindex_make_section(
			VIEW_ID_INSTRUMENTS, index));
	}
}

void instrumentview_on_select_section(InstrumentView* self,
	psy_ui_Component* sender, uintptr_t section, uintptr_t options)
{
	assert(self);	

	if (section != psy_INDEX_INVALID) {
		psy_ui_tabbar_mark(&self->tabbar, section);
		psy_ui_notebook_select(&self->notebook, section);
	}
}

uintptr_t instrumentview_section(const InstrumentView* self)
{
	assert(self);

	return psy_ui_notebook_page_index(&self->notebook);
}
