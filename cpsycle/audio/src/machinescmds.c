/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "machinescmds.h"
/* local */
#include "exclusivelock.h"
#include "machine.h"
#include "machinefactory.h"
#include "machineproxy.h"
#include "mixer.h"
/* std */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
/* platform */
#include "../../detail/trace.h"


typedef struct MachineCommandRouteState {
	uintptr_t fxslot;
	uintptr_t value;
} MachineCommandRouteState;

typedef struct MachineCommandInputState {
	uintptr_t inputslot;
	double volume;
	double panning;
	double drymix;
	double gain;
	int mute;
	int dryonly;
	int wetonly;
	psy_List* sendvols;
} MachineCommandInputState;

typedef struct MachineCommandReturnState {
	uintptr_t fxslot;
	double volume;
	double panning;
	int mute;
	unsigned char mastersend;
	double inputconvol;
	psy_List* sendsto;
} MachineCommandReturnState;

typedef struct MachineCommandMixerState {
	uintptr_t slot;
	uintptr_t inputsolo_slot;
	uintptr_t returnsolo_fxslot;
	psy_List* inputs;
	psy_List* returns;
} MachineCommandMixerState;

static void machinecommand_inputstate_dispose(MachineCommandInputState* self)
{
	psy_list_deallocate(&self->sendvols, NULL);
}

static void machinecommand_returnstate_dispose(MachineCommandReturnState* self)
{
	psy_list_deallocate(&self->sendsto, NULL);
}

static void machinecommand_mixerstate_dispose(MachineCommandMixerState* self)
{
	psy_list_deallocate(&self->inputs,
		(psy_fp_disposefunc)machinecommand_inputstate_dispose);
	psy_list_deallocate(&self->returns,
		(psy_fp_disposefunc)machinecommand_returnstate_dispose);
}

static void machinecommand_dispose_mixer_snapshots(psy_List** snapshots)
{
	psy_list_deallocate(snapshots,
		(psy_fp_disposefunc)machinecommand_mixerstate_dispose);
}

static psy_audio_Mixer* machinecommand_mixer_client(psy_audio_Machine* machine)
{
	psy_audio_MachineFactory* factory;

	if (!machine || psy_audio_machine_type(machine) != psy_audio_MIXER) {
		return NULL;
	}
	factory = psy_audio_machine_machinefactory(machine);
	if (factory && factory->createasproxy) {
		psy_audio_MachineProxy* proxy;

		proxy = (psy_audio_MachineProxy*)machine;
		if (!proxy->client ||
				psy_audio_machine_type(proxy->client) != psy_audio_MIXER) {
			return NULL;
		}
		return (psy_audio_Mixer*)proxy->client;
	}
	return (psy_audio_Mixer*)machine;
}

static uintptr_t machinecommand_input_id(psy_audio_Mixer* mixer,
	uintptr_t inputslot)
{
	psy_TableIterator it;

	for (it = psy_table_begin(&mixer->inputs);
			!psy_tableiterator_equal(&it, psy_table_end());
			psy_tableiterator_inc(&it)) {
		psy_audio_InputChannel* channel;

		channel = (psy_audio_InputChannel*)psy_tableiterator_value(&it);
		if (channel && channel->inputslot == inputslot) {
			return psy_tableiterator_key(&it);
		}
	}
	return psy_INDEX_INVALID;
}

static uintptr_t machinecommand_return_id(psy_audio_Mixer* mixer,
	uintptr_t fxslot)
{
	psy_TableIterator it;

	for (it = psy_table_begin(&mixer->returns);
			!psy_tableiterator_equal(&it, psy_table_end());
			psy_tableiterator_inc(&it)) {
		psy_audio_ReturnChannel* channel;

		channel = (psy_audio_ReturnChannel*)psy_tableiterator_value(&it);
		if (channel && channel->fxslot == fxslot) {
			return psy_tableiterator_key(&it);
		}
	}
	return psy_INDEX_INVALID;
}

static void machinecommand_capture_mixer_snapshots(psy_audio_Machines* machines,
	psy_List** snapshots)
{
	psy_TableIterator machine_it;

	machinecommand_dispose_mixer_snapshots(snapshots);
	for (machine_it = psy_audio_machines_begin(machines);
			!psy_tableiterator_equal(&machine_it, psy_table_end());
			psy_tableiterator_inc(&machine_it)) {
		psy_audio_Machine* machine;
		psy_audio_Mixer* mixer;
		MachineCommandMixerState* state;
		psy_TableIterator it;

		machine = (psy_audio_Machine*)psy_tableiterator_value(&machine_it);
		mixer = machinecommand_mixer_client(machine);
		if (!mixer) {
			continue;
		}
		state = (MachineCommandMixerState*)malloc(
			sizeof(MachineCommandMixerState));
		if (!state) {
			continue;
		}
		state->slot = psy_tableiterator_key(&machine_it);
		state->inputsolo_slot = psy_INDEX_INVALID;
		state->returnsolo_fxslot = psy_INDEX_INVALID;
		state->inputs = NULL;
		state->returns = NULL;
		if (mixer->inputsolo != psy_INDEX_INVALID) {
			psy_audio_InputChannel* solo;

			solo = psy_audio_mixer_channel(mixer, mixer->inputsolo);
			if (solo) {
				state->inputsolo_slot = solo->inputslot;
			}
		}
		if (mixer->returnsolo != psy_INDEX_INVALID) {
			psy_audio_ReturnChannel* solo;

			solo = psy_audio_mixer_return(mixer, mixer->returnsolo);
			if (solo) {
				state->returnsolo_fxslot = solo->fxslot;
			}
		}
		for (it = psy_table_begin(&mixer->inputs);
				!psy_tableiterator_equal(&it, psy_table_end());
				psy_tableiterator_inc(&it)) {
			psy_audio_InputChannel* channel;
			MachineCommandInputState* input;
			psy_TableIterator route_it;

			channel = (psy_audio_InputChannel*)psy_tableiterator_value(&it);
			if (!channel) {
				continue;
			}
			input = (MachineCommandInputState*)malloc(
				sizeof(MachineCommandInputState));
			if (!input) {
				continue;
			}
			input->inputslot = channel->inputslot;
			input->volume = channel->volume;
			input->panning = channel->panning;
			input->drymix = channel->drymix;
			input->gain = channel->gain;
			input->mute = channel->mute;
			input->dryonly = channel->dryonly;
			input->wetonly = channel->wetonly;
			input->sendvols = NULL;
			for (route_it = psy_table_begin(&channel->sendvols);
					!psy_tableiterator_equal(&route_it, psy_table_end());
					psy_tableiterator_inc(&route_it)) {
				psy_audio_ReturnChannel* target;
				MachineCommandRouteState* route;

				target = psy_audio_mixer_return(mixer,
					psy_tableiterator_key(&route_it));
				if (!target) {
					continue;
				}
				route = (MachineCommandRouteState*)malloc(
					sizeof(MachineCommandRouteState));
				if (!route) {
					continue;
				}
				route->fxslot = target->fxslot;
				route->value = (uintptr_t)psy_tableiterator_value(&route_it);
				psy_list_append(&input->sendvols, route);
			}
			psy_list_append(&state->inputs, input);
		}
		for (it = psy_table_begin(&mixer->returns);
				!psy_tableiterator_equal(&it, psy_table_end());
				psy_tableiterator_inc(&it)) {
			uintptr_t id;
			psy_audio_ReturnChannel* channel;
			psy_audio_MixerSend* send;
			MachineCommandReturnState* ret;
			psy_TableIterator route_it;

			id = psy_tableiterator_key(&it);
			channel = (psy_audio_ReturnChannel*)psy_tableiterator_value(&it);
			if (!channel) {
				continue;
			}
			ret = (MachineCommandReturnState*)malloc(
				sizeof(MachineCommandReturnState));
			if (!ret) {
				continue;
			}
			ret->fxslot = channel->fxslot;
			ret->volume = channel->volume;
			ret->panning = channel->panning;
			ret->mute = channel->mute;
			ret->mastersend = channel->mastersend;
			send = psy_audio_mixer_Send(mixer, id);
			ret->inputconvol = send ? send->inputconvol : 1.0;
			ret->sendsto = NULL;
			for (route_it = psy_table_begin(&channel->sendsto);
					!psy_tableiterator_equal(&route_it, psy_table_end());
					psy_tableiterator_inc(&route_it)) {
				psy_audio_ReturnChannel* target;
				MachineCommandRouteState* route;

				target = psy_audio_mixer_return(mixer,
					psy_tableiterator_key(&route_it));
				if (!target) {
					continue;
				}
				route = (MachineCommandRouteState*)malloc(
					sizeof(MachineCommandRouteState));
				if (!route) {
					continue;
				}
				route->fxslot = target->fxslot;
				route->value = TRUE;
				psy_list_append(&ret->sendsto, route);
			}
			psy_list_append(&state->returns, ret);
		}
		psy_list_append(snapshots, state);
	}
}

static void machinecommand_restore_mixer_snapshots(psy_audio_Machines* machines,
	psy_List* snapshots)
{
	psy_List* p;

	for (p = snapshots; p != NULL; p = p->next) {
		MachineCommandMixerState* state;
		psy_audio_Machine* machine;
		psy_audio_Mixer* mixer;
		psy_List* q;

		state = (MachineCommandMixerState*)p->entry;
		machine = psy_audio_machines_at(machines, state->slot);
		mixer = machinecommand_mixer_client(machine);
		if (!mixer) {
			continue;
		}
		for (q = state->inputs; q != NULL; q = q->next) {
			MachineCommandInputState* input;
			uintptr_t id;
			psy_audio_InputChannel* channel;
			psy_List* r;

			input = (MachineCommandInputState*)q->entry;
			id = machinecommand_input_id(mixer, input->inputslot);
			if (id == psy_INDEX_INVALID) {
				continue;
			}
			channel = psy_audio_mixer_channel(mixer, id);
			if (!channel) {
				continue;
			}
			channel->volume = input->volume;
			channel->panning = input->panning;
			channel->drymix = input->drymix;
			channel->gain = input->gain;
			channel->mute = input->mute;
			channel->dryonly = input->dryonly;
			channel->wetonly = input->wetonly;
			psy_table_dispose(&channel->sendvols);
			psy_table_init(&channel->sendvols);
			for (r = input->sendvols; r != NULL; r = r->next) {
				MachineCommandRouteState* route;
				uintptr_t return_id;

				route = (MachineCommandRouteState*)r->entry;
				return_id = machinecommand_return_id(mixer, route->fxslot);
				if (return_id != psy_INDEX_INVALID) {
					psy_table_insert(&channel->sendvols, return_id,
						(void*)route->value);
				}
			}
		}
		for (q = state->returns; q != NULL; q = q->next) {
			MachineCommandReturnState* ret;
			uintptr_t id;
			psy_audio_ReturnChannel* channel;
			psy_audio_MixerSend* send;
			psy_List* r;

			ret = (MachineCommandReturnState*)q->entry;
			id = machinecommand_return_id(mixer, ret->fxslot);
			if (id == psy_INDEX_INVALID) {
				continue;
			}
			channel = psy_audio_mixer_return(mixer, id);
			if (!channel) {
				continue;
			}
			channel->volume = ret->volume;
			channel->panning = ret->panning;
			channel->mute = ret->mute;
			channel->mastersend = ret->mastersend;
			send = psy_audio_mixer_Send(mixer, id);
			if (send) {
				send->inputconvol = ret->inputconvol;
			}
			psy_table_dispose(&channel->sendsto);
			psy_table_init(&channel->sendsto);
			for (r = ret->sendsto; r != NULL; r = r->next) {
				MachineCommandRouteState* route;
				uintptr_t return_id;

				route = (MachineCommandRouteState*)r->entry;
				return_id = machinecommand_return_id(mixer, route->fxslot);
				if (return_id != psy_INDEX_INVALID) {
					psy_table_insert(&channel->sendsto, return_id,
						(void*)(uintptr_t)TRUE);
				}
			}
		}
		mixer->inputsolo = (state->inputsolo_slot == psy_INDEX_INVALID)
			? psy_INDEX_INVALID
			: machinecommand_input_id(mixer, state->inputsolo_slot);
		mixer->returnsolo = (state->returnsolo_fxslot == psy_INDEX_INVALID)
			? psy_INDEX_INVALID
			: machinecommand_return_id(mixer, state->returnsolo_fxslot);
		++mixer->strobe;
	}
}

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
		rv->mixer_snapshots = NULL;
		psy_audio_connections_init(&rv->connections);
	}
	return rv;
}

void insertmachinecommand_dispose(InsertMachineCommand* self)
{
	machinecommand_dispose_detached(&self->machine, &self->machine_detached);
	machinecommand_dispose_mixer_snapshots(&self->mixer_snapshots);
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
		machinecommand_restore_mixer_snapshots(self->machines,
			self->mixer_snapshots);
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
		machinecommand_capture_mixer_snapshots(self->machines,
			&self->mixer_snapshots);
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
		rv->mixer_snapshots = NULL;
		psy_audio_connections_init(&rv->connections);
	}
	return rv;
}

void deletemachinecommand_dispose(DeleteMachineCommand* self)
{
	machinecommand_dispose_detached(&self->machine, &self->machine_detached);
	machinecommand_dispose_mixer_snapshots(&self->mixer_snapshots);
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
		machinecommand_capture_mixer_snapshots(self->machines,
			&self->mixer_snapshots);
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
		machinecommand_restore_mixer_snapshots(self->machines,
			self->mixer_snapshots);
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
