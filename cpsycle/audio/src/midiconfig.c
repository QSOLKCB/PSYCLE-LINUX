/*
** This source is free software; you can redistribute it and /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "midiconfig.h"

/* platform */
#include "../../detail/portable.h"

void psy_audio_midiconfiggroup_init(psy_audio_MidiConfigGroup* self,
	intptr_t grouptype, intptr_t command)
{
	assert(self);

	self->grouptype = grouptype;
	self->record = FALSE;
	self->type = psy_audio_MIDICONFIG_T_COMMAND;
	self->command = command;
	self->from = 0;
	self->to = 0xFF;
	if (grouptype == psy_audio_MIDICONFIG_GT_VELOCITY) {
		self->message = 0;
	} else if (grouptype == psy_audio_MIDICONFIG_GT_PITCHWHEEL) {
		self->message = 1;
	} else {
		self->message = 0;
	}
}

psy_audio_MidiConfigGroup* psy_audio_midiconfiggroup_alloc(void)
{
	return (psy_audio_MidiConfigGroup*)malloc(sizeof(
		psy_audio_MidiConfigGroup));
}

psy_audio_MidiConfigGroup* psy_audio_midiconfiggroup_allocinit(intptr_t grouptype,
	intptr_t command)
{
	psy_audio_MidiConfigGroup* rv;

	rv = psy_audio_midiconfiggroup_alloc();
	if (rv) {
		psy_audio_midiconfiggroup_init(rv, grouptype, command);
	}
	return rv;
}

const char* psy_audio_midiconfiggroup_tostring(const psy_audio_MidiConfigGroup*
	self)
{	
	assert(self);

	psy_snprintf(((psy_audio_MidiConfigGroup*)(self))->encode, 64,
		"%d %d %d %d %d %d ", (int)self->message, (int)self->type,
		(int)self->command, (int)self->record, (int)self->from, (int)self->to);
	return self->encode;
}


/* psy_audio_MidiConfig */

/* prototypes */
static void psy_audio_midiconfig_setdefaultsettings(psy_audio_MidiConfig*);
static void psy_audio_midiconfig_add_controller_to_config(psy_audio_MidiConfig* self,
	uintptr_t c, psy_audio_MidiConfigGroup* group);
static void psy_audio_midiconfig_on_mac_select(psy_audio_MidiConfig*,
	psy_Property* sender);
static void psy_audio_midiconfig_on_aux_select(psy_audio_MidiConfig*,
	psy_Property* sender);
static void psy_audio_midiconfig_on_record_raw_midi_as_mcm(
	psy_audio_MidiConfig*, psy_Property* sender);
static void psy_audio_midiconfig_on_add_controller(psy_audio_MidiConfig*,
	psy_Property* sender);
static void psy_audio_midiconfig_on_remove_controller(psy_audio_MidiConfig*,
	psy_Property* sender);
static void psy_audio_midiconfig_on_group_type(psy_audio_MidiConfig*,
	psy_Property* sender);
static void psy_audio_midiconfig_on_group_record(psy_audio_MidiConfig*,
	psy_Property* sender);
static void psy_audio_midiconfig_on_group_cmd(psy_audio_MidiConfig*,
	psy_Property* sender);
static void psy_audio_midiconfig_on_group_from(psy_audio_MidiConfig*,
	psy_Property* sender);
static void psy_audio_midiconfig_on_group_to(psy_audio_MidiConfig*,
	psy_Property* sender);
static uintptr_t psy_audio_midiconfig_group_index(const psy_Property* group);

/* implementation */
void psy_audio_midiconfig_init(psy_audio_MidiConfig* self,
	psy_Configuration* cfg)
{
	psy_Property* configuration;

	assert(self);

	self->groups = NULL;
	self->cfg = cfg;
	self->groups = NULL;
	self->raw = TRUE;
	self->gen_select_with = psy_audio_MIDICONFIG_MS_USE_SELECTED;
	self->inst_select_with = psy_audio_MIDICONFIG_MS_USE_SELECTED;
	if (self->cfg && psy_configuration_at(self->cfg, "controllerdata")) {
		if (!psy_audio_midiconfig_read_controllers(self)) {
			psy_audio_midiconfig_setdefaultsettings(self);
		}
	} else {
		psy_audio_midiconfig_setdefaultsettings(self);
	}
	if (self->cfg) {
		configuration = psy_configuration_at(cfg, "");
	} else {
		configuration = NULL;
	}
	if (configuration) {
		const psy_Property* property;

		property = psy_property_at_const(configuration, "macselect",
			PSY_PROPERTY_TYPE_CHOICE);
		if (property) {
			if (psy_property_item_int(property) >= 0 &&
				psy_property_item_int(property) <=
				psy_audio_MIDICONFIG_MS_MIDI_CHAN) {
				self->gen_select_with = psy_property_item_int(property);
			}
		}
		property = psy_property_at_const(configuration, "auxselect",
			PSY_PROPERTY_TYPE_CHOICE);
		if (property) {
			if (psy_property_item_int(property) >= 0 &&
				psy_property_item_int(property) <=
				psy_audio_MIDICONFIG_MS_MIDI_CHAN) {
				self->inst_select_with = psy_property_item_int(property);
			}
		}
		self->raw = psy_property_at_bool(configuration, "recordrawmidiasmcm", 0);
	}
	psy_configuration_connect(cfg, "macselect",
		self, psy_audio_midiconfig_on_mac_select);
	psy_configuration_connect(cfg, "auxselect",
		self, psy_audio_midiconfig_on_aux_select);
	psy_configuration_connect(cfg, "recordrawmidiasmcm",
		self, psy_audio_midiconfig_on_record_raw_midi_as_mcm);
	psy_configuration_connect(cfg, "addcontroller",	
		self, psy_audio_midiconfig_on_add_controller);
	psy_configuration_connect(cfg, "removecontrollers",	
		self, psy_audio_midiconfig_on_remove_controller);	
}

void psy_audio_midiconfig_dispose(psy_audio_MidiConfig* self)
{
	assert(self);

	psy_list_deallocate(&self->groups, (psy_fp_disposefunc)NULL);	
}

void psy_audio_midiconfig_reset(psy_audio_MidiConfig* self)
{
	assert(self);

	psy_audio_midiconfig_setdefaultsettings(self);
}

void psy_audio_midiconfig_setdefaultsettings(psy_audio_MidiConfig* self)
{	
	psy_audio_MidiConfigGroup group;

	assert(self);

	psy_list_deallocate(&self->groups, (psy_fp_disposefunc)NULL);
	self->groups = NULL;
	
	psy_audio_midiconfiggroup_init(&group, psy_audio_MIDICONFIG_GT_VELOCITY,
		0x0c);
	psy_audio_midiconfig_add_controller(self, group);
	psy_audio_midiconfiggroup_init(&group, psy_audio_MIDICONFIG_GT_PITCHWHEEL,
		0x01);
	psy_audio_midiconfig_add_controller(self, group);	
	self->raw = TRUE;
	self->gen_select_with = psy_audio_MIDICONFIG_MS_USE_SELECTED;
	self->inst_select_with = psy_audio_MIDICONFIG_MS_USE_SELECTED;
}

bool psy_audio_midiconfig_read_controllers(psy_audio_MidiConfig* self)
{
	psy_Property* property;

	assert(self);

	/* read and configure controller groups */
	property = psy_configuration_at(self->cfg, "controllerdata");
	if (property) {
		char* text;
		char seps[] = " ";
		char* token;
		intptr_t vals[6];
		intptr_t c;
		intptr_t i;

		assert(self);
		
		text = psy_strdup(psy_property_item_str(property));
		if (psy_strlen(text) == 0) {
			return FALSE;
		}
		token = strtok(text, seps);
		c = 0; i = 0;
		while (token != 0) {
			vals[c] = atoi(token);
			if (c == 5) {
				if (psy_audio_midiconfig_at(self, i)) {
					psy_audio_MidiConfigGroup* group;

					group = psy_audio_midiconfig_at(self, i);
					group->message = vals[0];
					if (i == 1) {
						group->command = vals[2];
					}
					group->record = (vals[3] != 0);
					group->from = vals[4];
					group->to = vals[5];
				}
				else {
					psy_audio_MidiConfigGroup group;

					if (i == 0) {
						group.grouptype =
							psy_audio_MIDICONFIG_GT_VELOCITY;
					}
					else if (i == 1) {
						group.grouptype =
							psy_audio_MIDICONFIG_GT_PITCHWHEEL;
					}
					else {
						group.grouptype =
							psy_audio_MIDICONFIG_GT_CUSTOM;
					}
					group.message = vals[0];
					group.type = vals[1];
					group.command = vals[2];
					group.record = (vals[3] != 0);
					group.from = vals[4];
					group.to = vals[5];
					psy_audio_midiconfig_add_controller(self, group);
				}
				c = 0;
				++i;
			}
			else {
				++c;
			}
			token = strtok(0, seps);
		}
		free(text);
	}
	return TRUE;
}

void psy_audio_midiconfig_write_controllers(psy_audio_MidiConfig* self)
{	
	psy_Property* p;
	
	assert(self);

	p = psy_configuration_at(self->cfg, "controllerdata");
	if (p) {
		char_dyn_t* str;

		str = psy_audio_midiconfig_controllers_to_string(self);
		psy_property_set_item_str(p, str);
		free(str);
		str = NULL;
	}
}

void psy_audio_midiconfig_add_controller(psy_audio_MidiConfig* self,
	psy_audio_MidiConfigGroup group)
{
	psy_audio_MidiConfigGroup* newgroup;
	uintptr_t numcontrollers;
	intptr_t command;

	assert(self);

	numcontrollers = psy_list_size(self->groups);
	if (numcontrollers == 0) {
		/* Velocity Controller */
		group.grouptype = psy_audio_MIDICONFIG_GT_VELOCITY;
		command = 0x0c;
	} else if (numcontrollers == 1) {
		/* Pitchwheel Controller */
		group.grouptype = psy_audio_MIDICONFIG_GT_PITCHWHEEL;
		command = 0x01;
	} else {
		command = (intptr_t)numcontrollers - 2;
	}
	newgroup = psy_audio_midiconfiggroup_allocinit(group.grouptype, command);
	if (newgroup) {
		*newgroup = group;
		psy_list_append(&self->groups, newgroup);
		psy_audio_midiconfig_add_controller_to_config(self, 
			numcontrollers, newgroup);
	}	
}

void psy_audio_midiconfig_add_controller_to_config(psy_audio_MidiConfig* self,
	uintptr_t c, psy_audio_MidiConfigGroup* midigroup)
{
	psy_Property* groups;
	psy_Property* group;
	psy_Property* choice;
	psy_Property* range;
	bool isvelocity;
	bool ispitchwheel;
	bool hasmessage;
	char text[256];

	assert(self);

	groups = psy_configuration_at(self->cfg, "groups");
	if (!groups) {
		return;
	}
	isvelocity = c == 0;
	ispitchwheel = c == 1;
	hasmessage = c > 1;
	
	psy_snprintf(text, 256, "controller%X", (c + 1));	
	group = psy_property_set_id(psy_property_set_hint(psy_property_set_text(
		psy_property_append_section(groups, text),
		(isvelocity)
		? "Velocity"
		: (ispitchwheel)
		? "Pitch Wheel"
		: "Controller"),
		PSY_PROPERTY_HINT_FLOW),
		c);
	psy_property_hide(psy_property_append_int(group, "id", c, 0, 0));
	if (hasmessage) {
		psy_property_set_hint(psy_property_set_text(psy_property_append_bool(
			group, "select", midigroup->record),
			"settings.midicontrollers.select"),
			PSY_PROPERTY_HINT_CHECK);
	}
	psy_property_connect(psy_property_set_text(
		psy_property_append_bool(group, "record",
		midigroup->record), "Map"),
		self, psy_audio_midiconfig_on_group_record);
	
	psy_property_set_hint(psy_property_setreadonly(psy_property_set_text(
		psy_property_append_int(group, "message", midigroup->message, 0, 127),
		"Controller Number"), (!hasmessage)), PSY_PROPERTY_HINT_EDITHEX);
	if (hasmessage || ispitchwheel) {
		choice = psy_property_connect(
			psy_property_set_hint(psy_property_set_text(
			psy_property_append_choice(group, "type", midigroup->type),
			"to"), PSY_PROPERTY_HINT_COMBO),
			self, psy_audio_midiconfig_on_group_type);
		psy_property_append_str(choice, "cmd", "");
		psy_property_append_str(choice, "twk", "");
		psy_property_append_str(choice, "tws", "");
		psy_property_append_str(choice, "mcm", "");
	}
	psy_property_connect(psy_property_set_hint(psy_property_set_text(
		psy_property_append_int(group, "cmd", midigroup->command, 0, 0xF),
		"value"), PSY_PROPERTY_HINT_EDITHEX),
		self, psy_audio_midiconfig_on_group_cmd);
	range = psy_property_set_hint(psy_property_append_section(
		group, "range"), PSY_PROPERTY_HINT_RANGE);
	psy_property_connect(psy_property_set_hint(
		psy_property_append_int(range, "from", midigroup->from, 0, 0xFF),
		PSY_PROPERTY_HINT_EDITHEX),
		self, psy_audio_midiconfig_on_group_from);
	psy_property_connect(psy_property_set_hint(psy_property_append_int(
		range, "to", midigroup->to, 0, 0xFF),
		PSY_PROPERTY_HINT_EDITHEX),
		self, psy_audio_midiconfig_on_group_to);
}

void psy_audio_midiconfig_remove_controller(psy_audio_MidiConfig* self, intptr_t id)
{
	assert(self);

	/* doesn't remove velocity or pitchwheel */
	if (self->groups && id > 1) {
		psy_List* i;

		i = psy_list_at(self->groups, id);
		if (i) {
			free(i->entry);
			psy_list_remove(&self->groups, i);
		}
	}
}

char_dyn_t* psy_audio_midiconfig_controllers_to_string(
	const psy_audio_MidiConfig* self)
{
	char_dyn_t* rv = NULL;
	const psy_List* i;

	assert(self);
		
	for (i = self->groups; i != NULL; psy_list_next_const(&i)) {
		const psy_audio_MidiConfigGroup* group;

		group = (psy_audio_MidiConfigGroup*)psy_list_entry_const(i);
		rv = psy_strcat_realloc(rv, psy_audio_midiconfiggroup_tostring(group));
	}	
	return rv;
}

void psy_audio_midiconfig_on_mac_select(psy_audio_MidiConfig* self,
	psy_Property* sender)
{		
	assert(self);
	
	if (psy_property_item_int(sender) >= 0 && psy_property_item_int(sender) <=
			psy_audio_MIDICONFIG_MS_MIDI_CHAN) {
		self->gen_select_with = psy_property_item_int(sender);
	}
}

void psy_audio_midiconfig_on_aux_select(psy_audio_MidiConfig* self,
	psy_Property* sender)
{	
	assert(self);
	
	if (psy_property_item_int(sender) >= 0 && psy_property_item_int(sender) <=
			psy_audio_MIDICONFIG_MS_MIDI_CHAN) {
		self->inst_select_with = psy_property_item_int(sender);
	}
}

void psy_audio_midiconfig_on_record_raw_midi_as_mcm(psy_audio_MidiConfig* self,
	psy_Property* sender)
{	
	assert(self);

	self->raw = psy_property_item_bool(sender);
}

void psy_audio_midiconfig_on_add_controller(psy_audio_MidiConfig* self,
	psy_Property* sender)
{
	psy_audio_MidiConfigGroup group;
	psy_Property* groups;
	
	assert(self);

	psy_audio_midiconfiggroup_init(&group, psy_audio_MIDICONFIG_GT_CUSTOM, 1);
	psy_audio_midiconfig_add_controller(self, group);
	groups = psy_configuration_at(self->cfg, "groups");
	if (groups) {		
		psy_property_rebuild(groups);
	}
}

void psy_audio_midiconfig_on_remove_controller(psy_audio_MidiConfig* self,
	psy_Property* sender)
{	
	psy_Property* groups;
	psy_List* p;
	psy_List* q;
	bool removed;
	
	assert(self);
		
	groups = psy_configuration_at(self->cfg, "groups");
	if (!groups) {
		return;
	}
	q = NULL;
	removed = FALSE;
	for (p = psy_property_begin(groups); p != NULL; p = q) {
		psy_Property* group;
		bool selected;

		q = p->next;
		group = (psy_Property*)p->entry;
		selected = psy_property_at_bool(group, "select", FALSE);
		if (selected) {			
			intptr_t id;
						
			id = psy_property_at_int(group, "id", -1);			
			psy_property_remove(groups, group);			
			psy_audio_midiconfig_remove_controller(self, id);
			removed = TRUE;
		}
	}
	if (removed) {
		psy_property_rebuild(groups);
	}	
}

void psy_audio_midiconfig_on_group_type(psy_audio_MidiConfig* self,
	psy_Property* sender)
{
	psy_audio_MidiConfigGroup* group;	

	if (!psy_property_parent(sender)) {
		return;
	}
	group = psy_audio_midiconfig_at(self, psy_audio_midiconfig_group_index(
		psy_property_parent(sender)));
	if (group) {
		group->type = (intptr_t)psy_property_item_int(sender);
	}
}

void psy_audio_midiconfig_on_group_record(psy_audio_MidiConfig* self,
	psy_Property* sender)
{	
	psy_audio_MidiConfigGroup* group;
	
	assert(self);

	group = psy_audio_midiconfig_at(self, psy_audio_midiconfig_group_index(
		psy_property_parent(sender)));
	if (group) {
		group->record = psy_property_item_bool(sender);
	}
}

void psy_audio_midiconfig_on_group_cmd(psy_audio_MidiConfig* self,
	psy_Property* sender)
{
	psy_audio_MidiConfigGroup* group;
	
	assert(self);

	group = psy_audio_midiconfig_at(self, psy_audio_midiconfig_group_index(
		psy_property_parent(sender)));
	if (group) {
		group->command = psy_property_item_int(sender);
	}
}

void psy_audio_midiconfig_on_group_from(psy_audio_MidiConfig* self,
	psy_Property* sender)
{
	psy_audio_MidiConfigGroup* group;

	assert(self);

	if (!psy_property_parent(sender) && !psy_property_parent(
			psy_property_parent(sender))) {
		return;
	}
	group = psy_audio_midiconfig_at(self, psy_audio_midiconfig_group_index(
		psy_property_parent(psy_property_parent(sender))));
	if (group) {
		group->from = psy_property_item_int(sender);
	}
}

void psy_audio_midiconfig_on_group_to(psy_audio_MidiConfig* self,
	psy_Property* sender)
{
	psy_audio_MidiConfigGroup* group;

	assert(self);

	if (!psy_property_parent(sender) && !psy_property_parent(
			psy_property_parent(sender))) {
		return;
	}
	group = psy_audio_midiconfig_at(self, psy_audio_midiconfig_group_index(
			psy_property_parent(psy_property_parent(sender))));
	if (group) {
		group->to = psy_property_item_int(sender);
	}
}

uintptr_t psy_audio_midiconfig_group_index(const psy_Property* group)
{
	if (group) {
		intptr_t tmp;

		tmp = psy_property_id(group);
		if (tmp >= 0) {
			return (uintptr_t)tmp;
		}
	}
	return psy_INDEX_INVALID;
}


/* MidiViewConfig */

/* prototypes */
static void psy_audio_midiviewconfig_make(psy_audio_MidiViewConfig*, psy_Property*);

/* implementation */
void psy_audio_midiviewconfig_init(psy_audio_MidiViewConfig* self, psy_Property* parent)
{
	assert(self);
	assert(parent);

	psy_customconfiguration_init(&self->configuration);	
	psy_audio_midiviewconfig_make(self, parent);
	psy_customconfiguration_set_root(&self->configuration, self->controllers);
}

void psy_audio_midiviewconfig_dispose(psy_audio_MidiViewConfig* self)
{
	assert(self);
}

void psy_audio_midiviewconfig_make(psy_audio_MidiViewConfig* self, psy_Property* parent)
{
	psy_Property* choice;

	assert(self);

	self->controllers = psy_property_set_text(
		psy_property_append_section(parent, "controllers"),
		"settings.midicontrollers.controllers");
	psy_property_set_svg(self->controllers, "img.machines");
	/* macselect */
	choice = psy_property_set_hint(psy_property_set_text(
		psy_property_append_choice(self->controllers,
			"macselect", 0),
		"settings.midicontrollers.macselect"),
		PSY_PROPERTY_HINT_COMBO);
	psy_property_set_text(
		psy_property_append_str(choice, "inpsycle", ""),
		"settings.midicontrollers.select-inpsycle");
	psy_property_set_text(
		psy_property_append_str(choice, "bybank", ""),
		"settings.midicontrollers.select-bybank");
	psy_property_set_text(
		psy_property_append_str(choice, "bybank", ""),
		"settings.midicontrollers.select-byprogram");
	psy_property_set_text(
		psy_property_append_str(choice, "bybank", ""),
		"settings.midicontrollers.select-bychannel");
	/* auxselect */
	choice = psy_property_set_hint(psy_property_set_text(
		psy_property_append_choice(self->controllers,
			"auxselect", 0),
		"settings.midicontrollers.auxselect"),
		PSY_PROPERTY_HINT_COMBO);
	psy_property_set_text(
		psy_property_append_str(choice, "inpsycle", ""),
		"settings.midicontrollers.select-inpsycle");
	psy_property_set_text(
		psy_property_append_str(choice, "bybank", ""),
		"settings.midicontrollers.select-bybank");
	psy_property_set_text(
		psy_property_append_str(choice, "bybank", ""),
		"settings.midicontrollers.select-byprogram");
	psy_property_set_text(
		psy_property_append_str(choice, "bybank", ""),
		"settings.midicontrollers.select-bychannel");
	psy_property_set_text(
		psy_property_append_bool(self->controllers,
			"recordrawmidiasmcm", TRUE),
		"settings.midicontrollers.recordrawmidiasmcm");
	/* map controllers */
	psy_property_set_text(
			psy_property_append_action(self->controllers,
			"addcontroller"),
			"settings.midicontrollers.add");
	psy_property_set_text(
		psy_property_append_action(self->controllers,
			"removecontrollers"), "settings.midicontrollers.remove");		
	psy_property_hide(psy_property_append_str(self->controllers,
		"controllerdata", ""));
	psy_property_prevent_save(psy_property_set_text(
		psy_property_append_section(self->controllers,
			"groups"), "Mapping"));
}
