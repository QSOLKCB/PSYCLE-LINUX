/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_audio_WIRE_H
#define psy_audio_WIRE_H

#include "connections.h"
#include <hashtbl.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
** @struct psy_audio_LegacyWire
** @brief load helper structure for psy files
**
** @detail
** The audio engine uses instead psy_audio_Connections
*/
typedef struct psy_audio_LegacyWire
{
	/* Input ports */	
	/* Incoming connections Machine number */
	int32_t input_machine;
	/* Incoming connections activated */
	uint8_t input_con;
	/* Incoming connections Machine vol */
	float input_convol;
	/*
	** Value to multiply input_convol[] with to have a 0.0...1.0 range
	** The reason of the wire_multiplier variable is because VSTs output
	** wave data in the range -1.0 to +1.0, while natives and internals
	** output at -32768.0 to +32768.0
	** Initially (when the format was made), Psycle did convert this in the
	** "Work" function, but since it already needs to multiply the output by
	** inputConVol, I decided to remove that extra conversion and use
	** directly the volume to do so.
	*/
	float wire_multiplier;
	/* Pin mapping (for loading) */
	psy_audio_PinMapping pinmapping;
	/* Output ports */		
	/* Outgoing connections Machine number */
	int32_t output_machine;
	/* Outgoing connections activated */
	uint8_t connection;	
} psy_audio_LegacyWire;

void psy_audio_legacywire_init(psy_audio_LegacyWire* self);
void psy_audio_legacywire_init_all(psy_audio_LegacyWire*,
	int inputmachine,
	bool inputcon,
	float inputconvol,
	float wiremultiplier,
	int outputmachine,
	bool connection);
void psy_audio_legacywire_copy(psy_audio_LegacyWire*, psy_audio_LegacyWire*
	source);
void psy_audio_legacywire_dispose(psy_audio_LegacyWire* self);

psy_audio_LegacyWire* psy_audio_legacywire_alloc(void);
psy_audio_LegacyWire* psy_audio_legacywire_alloc_init(void);
psy_audio_LegacyWire* psy_audio_legacywire_alloc_init_all(
	int inputmachine,
	bool inputcon,
	float inputconvol,
	float wiremultiplier,
	int outputmachine,
	bool connection);
psy_audio_LegacyWire* psy_audio_legacywire_clone(psy_audio_LegacyWire* source);	

typedef psy_Table psy_audio_MachineWires;

void psy_audio_machinewires_init(psy_audio_MachineWires*);
void psy_audio_machinewires_copy(psy_audio_MachineWires*,
	psy_audio_MachineWires* other);
void psy_audio_machinewires_dispose(psy_audio_MachineWires*);
psy_audio_MachineWires* psy_audio_machinewires_alloc(void);
psy_audio_MachineWires* psy_audio_machinewires_alloc_init(void);
psy_audio_MachineWires* psy_audio_machinewires_clone(psy_audio_MachineWires*
	source);
void psy_audio_machinewires_deallocate(psy_audio_MachineWires*);
void psy_audio_machinewires_clear(psy_audio_MachineWires*);
void psy_audio_machinewires_insert(psy_audio_MachineWires*,
	uintptr_t connectionid, psy_audio_LegacyWire*);
psy_audio_LegacyWire* psy_audio_machinewires_at(psy_audio_MachineWires*,
	uintptr_t connectionid);

/*!
** @struct psy_audio_LegacyWires
** @brief Load helper
**
** @details
** Stores the connections of all machines with machineslot as key and as
** value MachineWires, the wires of one machine. Audio Engine uses
** instead psy_audio_Connections
*/

typedef struct psy_audio_LegacyWires {
	/* ConnectionID X psy_audio_LegacyWire* */
	psy_Table legacy_wires_;
} psy_audio_LegacyWires;

void psy_audio_legacywires_init(psy_audio_LegacyWires*);
void psy_audio_legacywires_dispose(psy_audio_LegacyWires*);
void psy_audio_legacywires_insert(psy_audio_LegacyWires*, uintptr_t macid,
	psy_audio_MachineWires* machinewires);
psy_Table* psy_audio_legacywires_at(psy_audio_LegacyWires*,
	uintptr_t machineslot);
/*
** searches for an existing output connection in the machinewires of a machine
** and if found returns the connection id (wire number) else -1
*/
int psy_audio_legacywires_find_legacy_output(psy_audio_LegacyWires*,
	int source_mac, int mac_index);

INLINE psy_TableIterator psy_audio_legacywires_begin(
	psy_audio_LegacyWires* self)
{
	return psy_table_begin(&self->legacy_wires_);
}

#ifdef __cplusplus
}
#endif

#endif /* psy_audio_WIRE_H */
