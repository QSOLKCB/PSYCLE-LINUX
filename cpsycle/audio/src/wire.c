/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "wire.h"
/* local */
#include "constants.h"
#include "songio.h"
/* std */
#include <stdlib.h>


/* LegacyWire */

void psy_audio_legacywire_init(psy_audio_LegacyWire* self)
{
	assert(self);

	self->input_machine = -1;
	self->input_con = FALSE;
	self->input_convol = 1.f;;
	self->wire_multiplier = 1.f;
	self->output_machine = -1;
	self->connection = FALSE;
	psy_audio_pinmapping_init(&self->pinmapping, 2);
}

void psy_audio_legacywire_init_all(psy_audio_LegacyWire* self,
	int inputmachine,
	bool inputcon,
	float inputconvol,
	float wiremultiplier,
	int outputmachine,
	bool connection)
{
	assert(self);

	self->input_machine = inputmachine;
	self->input_con = inputcon;
	self->input_convol = inputconvol;
	self->wire_multiplier = wiremultiplier;
	self->output_machine = outputmachine;
	self->connection = connection;
	psy_audio_pinmapping_init(&self->pinmapping, 2);
}


void psy_audio_legacywire_copy(psy_audio_LegacyWire* self,
	psy_audio_LegacyWire* source)
{
	assert(self);
	assert(source);

	self->input_machine = source->input_machine;
	self->input_con = source->input_con;
	self->input_convol = source->input_convol;
	self->wire_multiplier = source->wire_multiplier;
	self->output_machine = source->output_machine;
	self->connection = source->connection;
	psy_audio_pinmapping_dispose(&self->pinmapping);
	psy_audio_pinmapping_init(&self->pinmapping, 0);
	psy_audio_pinmapping_copy(&self->pinmapping, &source->pinmapping);	
}

void psy_audio_legacywire_dispose(psy_audio_LegacyWire* self)
{
	psy_audio_pinmapping_dispose(&self->pinmapping);
}

psy_audio_LegacyWire* psy_audio_legacywire_alloc(void)
{
	return (psy_audio_LegacyWire*)malloc(sizeof(psy_audio_LegacyWire));
}

psy_audio_LegacyWire* psy_audio_legacywire_alloc_init(void)
{
	psy_audio_LegacyWire* rv;

	rv = psy_audio_legacywire_alloc();
	if (rv) {
		psy_audio_legacywire_init(rv);
	}
	return rv;
}

psy_audio_LegacyWire* psy_audio_legacywire_alloc_init_all(
	int inputmachine,
	bool inputcon,
	float inputconvol,
	float wiremultiplier,
	int outputmachine,
	bool connection)
{
	psy_audio_LegacyWire* rv;

	rv = psy_audio_legacywire_alloc();
	if (rv) {
		psy_audio_legacywire_init_all(rv,
			inputmachine,
			inputcon,
			inputconvol,
			wiremultiplier,
			outputmachine,
			connection);
	}
	return rv;
}


psy_audio_LegacyWire* psy_audio_legacywire_clone(psy_audio_LegacyWire* source)
{
	psy_audio_LegacyWire* rv;
	
	assert(source);

	rv = psy_audio_legacywire_alloc_init();
	if (rv) {
		psy_audio_legacywire_copy(rv, source);
	}
	return rv;
}


/* MachineWires */

void psy_audio_machinewires_init(psy_audio_MachineWires* self)
{	
	psy_table_init(self);
}

void psy_audio_machinewires_copy(psy_audio_MachineWires* self,
	psy_audio_MachineWires* other)
{	
	psy_TableIterator it;

	assert(self && other);

	psy_audio_machinewires_clear(self);

	for (it = psy_table_begin(other);
			!psy_tableiterator_equal(&it, psy_table_end());
			psy_tableiterator_inc(&it)) {
		psy_audio_machinewires_insert(self, psy_tableiterator_key(&it),
			psy_audio_legacywire_clone((psy_audio_LegacyWire*)
				psy_tableiterator_value(&it)));
	}	
}

void psy_audio_machinewires_dispose(psy_audio_MachineWires* self)
{
	//psy_table_dispose_all(&self->wires, (psy_fp_disposefunc)
		//psy_audio_legacywire_dispose);
	psy_table_dispose_all(self, (psy_fp_disposefunc)
		psy_audio_legacywire_dispose);
}

psy_audio_MachineWires* psy_audio_machinewires_alloc(void)
{
	return (psy_audio_MachineWires*)malloc(sizeof(psy_audio_MachineWires));
}

psy_audio_MachineWires* psy_audio_machinewires_alloc_init(void)
{
	psy_audio_MachineWires* rv;

	rv = psy_audio_machinewires_alloc();
	if (rv) {
		psy_audio_machinewires_init(rv);
	}
	return rv;
}

psy_audio_MachineWires* psy_audio_machinewires_clone(
	psy_audio_MachineWires* source)
{
	psy_audio_MachineWires* rv;

	assert(source);

	rv = psy_audio_machinewires_alloc_init();
	if (rv) {
		psy_audio_machinewires_copy(rv, source);
	}
	return rv;
}


void psy_audio_machinewires_deallocate(psy_audio_MachineWires* self)
{
	assert(self);

	psy_audio_machinewires_dispose(self);
	free(self);
}

void psy_audio_machinewires_clear(psy_audio_MachineWires* self)
{
	psy_audio_machinewires_dispose(self);
	psy_audio_machinewires_init(self);
}

void psy_audio_machinewires_insert(psy_audio_MachineWires* self,
	uintptr_t connectionid, psy_audio_LegacyWire* wire)
{
	assert(self && wire);

	if (psy_table_exists(self, connectionid)) {
		psy_audio_LegacyWire* wire;

		wire = (psy_audio_LegacyWire*)psy_table_at(self,
			connectionid);
		psy_audio_legacywire_dispose(wire);
		free(wire);
	}
	psy_table_insert(self, connectionid, wire);
}

psy_audio_LegacyWire* psy_audio_machinewires_at(psy_audio_MachineWires* self,
	uintptr_t connectionid)
{
	assert(self);

	return (psy_audio_LegacyWire*)psy_table_at(self, connectionid);
}


/* LegacyWires */

void psy_audio_legacywires_init(psy_audio_LegacyWires* self)
{
	assert(self);

	psy_table_init(&self->legacy_wires_);
}

void psy_audio_legacywires_dispose(psy_audio_LegacyWires* self)
{
	psy_table_dispose_all(&self->legacy_wires_, (psy_fp_disposefunc)
		psy_audio_machinewires_dispose);	
}

void psy_audio_legacywires_insert(psy_audio_LegacyWires* self, uintptr_t macid,
	psy_audio_MachineWires* machinewires)
{
	assert(self && machinewires);

	if (psy_table_exists(&self->legacy_wires_, macid)) {
		psy_audio_MachineWires* wires;

		wires = (psy_audio_MachineWires*)psy_table_at(&self->legacy_wires_,
			macid);
		psy_audio_machinewires_deallocate(wires);
	}
	psy_table_insert(&self->legacy_wires_, macid, machinewires);
}

psy_Table* psy_audio_legacywires_at(psy_audio_LegacyWires* self,
	uintptr_t machineslot)
{
	assert(self);

	return psy_table_at(&self->legacy_wires_, machineslot);
}

int psy_audio_legacywires_find_legacy_output(psy_audio_LegacyWires* self,
	int source_mac, int mac_index)
{
	psy_Table* legacywire_table;
	psy_TableIterator it;

	assert(self);

	legacywire_table = psy_audio_legacywires_at(self, source_mac);
	if (!legacywire_table) {
		return -1;
	}
	for (it = psy_table_begin(legacywire_table);
			!psy_tableiterator_equal(&it, psy_table_end());
			psy_tableiterator_inc(&it)) {
		psy_audio_LegacyWire* legacy_wire;
		
		if (psy_tableiterator_key(&it) >= MAX_CONNECTIONS) {
			continue;
		}
		legacy_wire = psy_tableiterator_value(&it);
		if (legacy_wire && legacy_wire->connection &&
			(legacy_wire->output_machine == mac_index))
		{			
			return (int)(intptr_t)psy_tableiterator_key(&it);
		}
	}
	return -1;
}
