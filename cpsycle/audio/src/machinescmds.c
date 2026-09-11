/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "machinescmds.h"
/* local */
#include "exclusivelock.h"
#include "machine.h"
/* std */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
/* platform */
#include "../../detail/trace.h"


static void machinecommand_dispose_detached(psy_audio_Machine** machine,
	bool* detached)
{
	if (*detached && *machine) {
		psy_audio_machine_dispose(*machine);
		free(*machine);
		*machine = NULL;
		*detached = FALSE;
	}
}

static void machinecommand_mark_detached(psy_audio_Machine* machine)
{
	if (machine) {
		/*
		** A retained machine is not part of the live connection graph while
		** command undo/redo tears down or restores its wires. Connection-aware
		** machines such as Mixer use their slot to decide whether a global
		** connection notification belongs to them. The caller holds the audio
		** exclusive lock across this temporary invalidation and graph mutation,
		** so the realtime callback can never observe psy_INDEX_INVALID.
		*/
		psy_audio_machine_set_slot(machine, psy_INDEX_INVALID);
	}
}

static void machinecommand_restore_connections(psy_audio_Connections* live,
	psy_audio_Connections* saved)
{
	psy_audio_Connections before;
	psy_TableIterator slot_it;

	/*
	** A raw connections_copy() restores the tables but deliberately emits no
	** connection signals. First reconcile only the topology delta through the
	** normal signal-emitting operations so Mixers and other connection-aware
	** machines rebuild their internal routing state. Then copy the snapshot to
	** restore exact socket metadata (pin mappings, wire volumes, sends, etc.).
	*/
	psy_audio_connections_init(&before);
	psy_audio_connections_copy(&before, live);

	for (slot_it = psy_table_begin(&before.container);
			!psy_tableiterator_equal(&slot_it, psy_table_end());
			psy_tableiterator_inc(&slot_it)) {
		psy_audio_MachineSockets* sockets;
		psy_TableIterator wire_it;
		uintptr_t src;

		src = psy_tableiterator_key(&slot_it);
		sockets = (psy_audio_MachineSockets*)psy_tableiterator_value(&slot_it);
		for (wire_it = psy_audio_wiresockets_begin(&sockets->outputs);
				!psy_tableiterator_equal(&wire_it, psy_table_end());
				psy_tableiterator_inc(&wire_it)) {
			psy_audio_WireSocket* socket;
			psy_audio_Wire wire;

			socket = (psy_audio_WireSocket*)psy_tableiterator_value(&wire_it);
			wire = psy_audio_wire_make(src, socket->slot);
			if (!psy_audio_connections_connected(saved, wire)) {
				psy_audio_connections_disconnect(live, wire);
			}
		}
	}

	for (slot_it = psy_table_begin(&saved->container);
			!psy_tableiterator_equal(&slot_it, psy_table_end());
			psy_tableiterator_inc(&slot_it)) {
		psy_audio_MachineSockets* sockets;
		psy_TableIterator wire_it;
		uintptr_t src;

		src = psy_tableiterator_key(&slot_it);
		sockets = (psy_audio_MachineSockets*)psy_tableiterator_value(&slot_it);
		for (wire_it = psy_audio_wiresockets_begin(&sockets->outputs);
				!psy_tableiterator_equal(&wire_it, psy_table_end());
				psy_tableiterator_inc(&wire_it)) {
			psy_audio_WireSocket* socket;
			psy_audio_Wire wire;

			socket = (psy_audio_WireSocket*)psy_tableiterator_value(&wire_it);
			wire = psy_audio_wire_make(src, socket->slot);
			if (!psy_audio_connections_connected(live, wire)) {
				psy_audio_connections_connect(live, wire);
			}
		}
	}

	psy_audio_connections_copy(live, saved);
	psy_audio_connections_dispose(&before);
}

/* InsertMachineCommand */

/* vtable */
static psy_CommandVtable insertmachinecommand_vtable;
static bool insertmachinecommand_vtable_initialized = FALSE;

static void insertmachinecommand_vtable_init(InsertMachineCommand* self)
{
	if (!insertmachinecommand_vtable_initialized) {
		insertmachinecommand_vtable = *(self->command.vtable);
		insertmachinecommand_vtable.dispose =
			(psy_fp_command)
			insertmachinecommand_dispose;
		insertmachinecommand_vtable.execute =
			(psy_fp_command_param)
			insertmachinecommand_execute;
		insertmachinecommand_vtable.revert =
			(psy_fp_command)
			insertmachinecommand_revert;
		insertmachinecommand_vtable_initialized = TRUE;
	}
	self->command.vtable = &insertmachinecommand_vtable;
}

/* implementation */
InsertMachineCommand* insertmachinecommand_allocinit(psy_audio_Machines*
	machines, uintptr_t slot, psy_audio_Machine* machine)
{
	InsertMachineCommand* rv;

	rv = malloc(sizeof(InsertMachineCommand));
	if (rv) {
		psy_command_init(&rv->command);
		insertmachinecommand_vtable_init(rv);		
		rv->machines = machines;
		rv->machine = machine;
		rv->slot = slot;
		rv->restoreconnection = FALSE;
		rv->machine_detached = TRUE;
		psy_audio_connections_init(&rv->connections);
	}
	return rv;
}

void insertmachinecommand_dispose(InsertMachineCommand* self)
{
	machinecommand_dispose_detached(&self->machine, &self->machine_detached);
	psy_audio_connections_dispose(&self->connections);
}

void insertmachinecommand_execute(InsertMachineCommand* self,
	uintptr_t param)
{
	self->machines->preventundoredo = TRUE;
	psy_audio_exclusivelock_enter();
	psy_audio_machines_insert(self->machines, self->slot,
		self->machine);
	self->machine_detached = FALSE;
	if (self->restoreconnection) {
		/* Keep the retained machine from consuming its own synthetic restore
		** notifications; its preserved internal state already represents those
		** incident wires. Other machines still receive the topology delta. */
		machinecommand_mark_detached(self->machine);
		machinecommand_restore_connections(&self->machines->connections,
			&self->connections);
		psy_audio_machine_set_slot(self->machine, self->slot);
		psy_audio_machines_updatepath(self->machines);
	}
	psy_audio_exclusivelock_leave();
	self->machines->preventundoredo = FALSE;
}

void insertmachinecommand_revert(InsertMachineCommand* self)
{
	psy_audio_Machine* machine;

	self->machines->preventundoredo = TRUE;
	psy_audio_exclusivelock_enter();
	machine = psy_audio_machines_at(self->machines, self->slot);
	if (machine) {
		psy_audio_connections_dispose(&self->connections);
		psy_audio_connections_init(&self->connections);
		psy_audio_connections_copy(&self->connections, &self->machines->connections);
		self->restoreconnection = TRUE;
		self->machine = machine;
		machinecommand_mark_detached(machine);
		psy_audio_machines_erase(self->machines, self->slot);
		self->machine_detached = TRUE;
	}	
	psy_audio_exclusivelock_leave();
	self->machines->preventundoredo = FALSE;
}

/* DeleteMachineCommand */

/* vtable */

static psy_CommandVtable deletemachinecommand_vtable;
static int deletemachinecommand_vtable_initialized = FALSE;

static void deletemachinecommand_vtable_init(DeleteMachineCommand* self)
{
	if (!deletemachinecommand_vtable_initialized) {
		deletemachinecommand_vtable = *(self->command.vtable);
		deletemachinecommand_vtable.dispose =
			(psy_fp_command)
			deletemachinecommand_dispose;
		deletemachinecommand_vtable.execute =
			(psy_fp_command_param)
			deletemachinecommand_execute;
		deletemachinecommand_vtable.revert =
			(psy_fp_command)
			deletemachinecommand_revert;
		deletemachinecommand_vtable_initialized = TRUE;
	}
	self->command.vtable = &deletemachinecommand_vtable;
}

/* implementation */
DeleteMachineCommand* deletemachinecommand_allocinit(
	psy_audio_Machines* machines, uintptr_t slot)
{
	DeleteMachineCommand* rv;

	rv = malloc(sizeof(DeleteMachineCommand));
	if (rv) {
		psy_command_init(&rv->command);
		deletemachinecommand_vtable_init(rv);		
		rv->machines = machines;
		rv->machine = NULL;
		rv->slot = slot;
		rv->machine_detached = FALSE;
		psy_audio_connections_init(&rv->connections);
	}
	return rv;
}

void deletemachinecommand_dispose(DeleteMachineCommand* self)
{
	machinecommand_dispose_detached(&self->machine, &self->machine_detached);
	psy_audio_connections_dispose(&self->connections);
}

void deletemachinecommand_execute(DeleteMachineCommand* self,
	uintptr_t param)
{
	psy_audio_Machine* machine;

	self->machines->preventundoredo = TRUE;
	psy_audio_exclusivelock_enter();
	machine = psy_audio_machines_at(self->machines, self->slot);
	if (machine) {
		psy_audio_connections_dispose(&self->connections);
		psy_audio_connections_init(&self->connections);
		psy_audio_connections_copy(&self->connections, &self->machines->connections);
		self->machine = machine;
		psy_audio_connections_rewire(&self->machines->connections,
			psy_audio_connections_at(&self->machines->connections, self->slot));
		machinecommand_mark_detached(machine);
		psy_audio_machines_erase(self->machines, self->slot);
		self->machine_detached = TRUE;
	}
	psy_audio_exclusivelock_leave();
	self->machines->preventundoredo = FALSE;
}

void deletemachinecommand_revert(DeleteMachineCommand* self)
{
	self->machines->preventundoredo = TRUE;
	psy_audio_exclusivelock_enter();
	if (self->machine && self->machine_detached) {
		psy_audio_machines_insert(self->machines, self->slot,
			self->machine);
		self->machine_detached = FALSE;
		/* The retained machine already owns the channel state for its original
		** incident wires. Suppress only its own restore callbacks while the
		** signal-emitting delta repairs downstream connection-aware machines. */
		machinecommand_mark_detached(self->machine);
		machinecommand_restore_connections(&self->machines->connections,
			&self->connections);
		psy_audio_machine_set_slot(self->machine, self->slot);
		psy_audio_machines_updatepath(self->machines);
	}	
	psy_audio_exclusivelock_leave();
	self->machines->preventundoredo = FALSE;
}


/* ConnectMachineCommand */

/* prototypes */
static void connectmachinecommand_execute(ConnectMachineCommand*,
	uintptr_t param);
static void connectmachinecommand_revert(ConnectMachineCommand*);

/* vtable */
static psy_CommandVtable connectmachinecommand_vtable;
static bool connectmachinecommand_vtable_initialized = FALSE;

static void connectmachinecommand_vtable_init(ConnectMachineCommand* self)
{
	if (!connectmachinecommand_vtable_initialized) {
		connectmachinecommand_vtable = *(self->command.vtable);
		connectmachinecommand_vtable.dispose =
			(psy_fp_command)
			connectmachinecommand_dispose;
		connectmachinecommand_vtable.execute =
			(psy_fp_command_param)
			connectmachinecommand_execute;
		connectmachinecommand_vtable.revert =
			(psy_fp_command)
			connectmachinecommand_revert;
		connectmachinecommand_vtable_initialized = TRUE;
	}
	self->command.vtable = &connectmachinecommand_vtable;
}

/* implementation */
ConnectMachineCommand* connectmachinecommand_alloc_init(
	psy_audio_Machines* machines, psy_audio_Wire wire)
{
	ConnectMachineCommand* rv;

	rv = malloc(sizeof(ConnectMachineCommand));
	if (rv) {
		psy_command_init(&rv->command);
		connectmachinecommand_vtable_init(rv);		
		rv->machines = machines;
		rv->wire = wire;
		rv->volume = 1.0;
		psy_audio_pinmapping_init(&rv->pins, 2);
		rv->restore = FALSE;
	}
	return rv;
}

void connectmachinecommand_dispose(ConnectMachineCommand* self)
{
	psy_audio_pinmapping_dispose(&self->pins);
}

void connectmachinecommand_execute(ConnectMachineCommand* self,
	uintptr_t param)
{	
	self->machines->preventundoredo = TRUE;
	psy_audio_machines_connect(self->machines, self->wire);
	if (self->restore) {
		psy_audio_exclusivelock_enter();
		psy_audio_connections_setpinmapping(&self->machines->connections,
			self->wire, &self->pins);
		psy_audio_connections_set_wire_volume(&self->machines->connections,
			self->wire, self->volume);
		psy_audio_exclusivelock_leave();
	}
	self->machines->preventundoredo = FALSE;
}

void connectmachinecommand_revert(ConnectMachineCommand* self)
{
	psy_audio_WireSocket* socket;
		
	self->machines->preventundoredo = TRUE;
	self->volume = psy_audio_connections_wire_volume(&self->machines->connections,
		self->wire);
	socket = psy_audio_connections_input(&self->machines->connections, self->wire);
	if (socket) {
		psy_audio_pinmapping_copy(&self->pins, &socket->mapping);
		self->restore = TRUE;
	} else {
		self->restore = FALSE;
	}
	psy_audio_machines_disconnect(self->machines, self->wire);
	self->machines->preventundoredo = FALSE;	
}

/* DisconnectMachineCommand */

/* vtable */
static psy_CommandVtable disconnectmachinecommand_vtable;
static bool disconnectmachinecommand_vtable_initialized = FALSE;

static void disconnectmachinecommand_vtable_init(DisconnectMachineCommand* self)
{
	if (!disconnectmachinecommand_vtable_initialized) {
		disconnectmachinecommand_vtable = *(self->command.vtable);
		disconnectmachinecommand_vtable.dispose =
			(psy_fp_command)
			disconnectmachinecommand_dispose;
		disconnectmachinecommand_vtable.execute =
			(psy_fp_command_param)
			disconnectmachinecommand_execute;
		disconnectmachinecommand_vtable.revert =
			(psy_fp_command)
			disconnectmachinecommand_revert;
		disconnectmachinecommand_vtable_initialized = TRUE;
	}
	self->command.vtable = &disconnectmachinecommand_vtable;
}

/* implementation */
DisconnectMachineCommand* disconnectmachinecommand_alloc_init(psy_audio_Machines* machines,
	psy_audio_Wire wire)
{
	DisconnectMachineCommand* rv;

	rv = malloc(sizeof(DisconnectMachineCommand));
	if (rv) {
		psy_command_init(&rv->command);
		disconnectmachinecommand_vtable_init(rv);		
		rv->command.vtable = &disconnectmachinecommand_vtable;
		rv->machines = machines;
		rv->wire = wire;
		rv->volume = 1.0;
		psy_audio_pinmapping_init(&rv->pins, 2);
	}
	return rv;
}

void disconnectmachinecommand_dispose(DisconnectMachineCommand* self)
{
	psy_audio_pinmapping_dispose(&self->pins);
}

void disconnectmachinecommand_execute(DisconnectMachineCommand* self,
	uintptr_t param)
{
	psy_audio_WireSocket* socket;
		
	self->machines->preventundoredo = TRUE;
	self->volume = psy_audio_connections_wire_volume(
		&self->machines->connections, self->wire);
	socket = psy_audio_connections_input(&self->machines->connections, self->wire);
	if (socket) {
		psy_audio_pinmapping_copy(&self->pins, &socket->mapping);
	}
	psy_audio_machines_disconnect(self->machines, self->wire);
	self->machines->preventundoredo = FALSE;
}

void disconnectmachinecommand_revert(DisconnectMachineCommand* self)
{
	self->machines->preventundoredo = TRUE;
	psy_audio_machines_connect(self->machines, self->wire);
	psy_audio_exclusivelock_enter();
	psy_audio_connections_setpinmapping(&self->machines->connections,
		self->wire, &self->pins);	
	psy_audio_connections_set_wire_volume(&self->machines->connections, self->wire,
		self->volume);
	psy_audio_exclusivelock_leave();
	self->machines->preventundoredo = FALSE;
}
