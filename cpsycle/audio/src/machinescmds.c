/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "machinescmds.h"
/* local */
#include "exclusivelock.h"
#include "machine.h"
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
	uintptr_t id;
	uintptr_t inputslot;
	double volume;
	double panning;
	double drymix;
	double gain;
	int mute;
	int dryonly;
	int wetonly;
	bool survived;
	psy_List* sendvols;
} MachineCommandInputState;

typedef struct MachineCommandReturnState {
	uintptr_t id;
	uintptr_t fxslot;
	double volume;
	double panning;
	int mute;
	unsigned char mastersend;
	double inputconvol;
	psy_List* sendsto;
	psy_List* incoming_from;
} MachineCommandReturnState;

typedef struct MachineCommandMixerState {
	uintptr_t slot;
	psy_List* inputs;
	psy_List* returns;
} MachineCommandMixerState;

typedef enum MachineCommandMixerWireKind {
	MACHINECOMMAND_MIXER_WIRE_UNKNOWN = 0,
	MACHINECOMMAND_MIXER_WIRE_INPUT,
	MACHINECOMMAND_MIXER_WIRE_RETURN
} MachineCommandMixerWireKind;

static void machinecommand_inputstate_dispose(MachineCommandInputState* self)
{
	psy_list_deallocate(&self->sendvols, NULL);
}

static void machinecommand_returnstate_dispose(MachineCommandReturnState* self)
{
	psy_list_deallocate(&self->sendsto, NULL);
	psy_list_deallocate(&self->incoming_from, NULL);
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

static bool machinecommand_is_proxy(psy_audio_Machine* machine)
{
#ifdef PSYCLE_USE_MACHINEPROXY
	psy_audio_MachineProxy probe;
	bool rv;

	if (!machine) {
		return FALSE;
	}
	/*
	** Proxy support is a build capability, not a per-machine guarantee:
	** MachineFactory can explicitly create raw machines. Build a temporary
	** proxy only to obtain the proxy vtable identity, compare the candidate's
	** vtable, then dispose just the probe's base Machine state. Never dispose
	** the probe through its proxy vtable, because that would dispose `machine`.
	*/
	psy_audio_machineproxy_init(&probe, machine);
	rv = (machine->vtable == probe.machine.vtable);
	if (probe.machinedispose) {
		probe.machinedispose(&probe.machine);
	}
	return rv;
#else
	(void)machine;
	return FALSE;
#endif
}

static psy_audio_Mixer* machinecommand_mixer_client(psy_audio_Machine* machine)
{
	if (!machine || psy_audio_machine_type(machine) != psy_audio_MIXER) {
		return NULL;
	}
#ifdef PSYCLE_USE_MACHINEPROXY
	if (machinecommand_is_proxy(machine)) {
		psy_audio_MachineProxy* proxy;

		proxy = (psy_audio_MachineProxy*)machine;
		if (!proxy->client ||
				psy_audio_machine_type(proxy->client) != psy_audio_MIXER) {
			return NULL;
		}
		return (psy_audio_Mixer*)proxy->client;
	}
#endif
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

static MachineCommandInputState* machinecommand_capture_input(
	psy_audio_Mixer* mixer, psy_audio_InputChannel* channel)
{
	MachineCommandInputState* input;
	psy_TableIterator route_it;

	input = (MachineCommandInputState*)malloc(sizeof(MachineCommandInputState));
	if (!input) {
		return NULL;
	}
	input->id = channel->id;
	input->inputslot = channel->inputslot;
	input->volume = channel->volume;
	input->panning = channel->panning;
	input->drymix = channel->drymix;
	input->gain = channel->gain;
	input->mute = channel->mute;
	input->dryonly = channel->dryonly;
	input->wetonly = channel->wetonly;
	input->survived = FALSE;
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
	return input;
}

static MachineCommandReturnState* machinecommand_capture_return(
	psy_audio_Mixer* mixer, uintptr_t id, psy_audio_ReturnChannel* channel)
{
	MachineCommandReturnState* ret;
	psy_audio_MixerSend* send;
	psy_TableIterator route_it;

	ret = (MachineCommandReturnState*)malloc(sizeof(MachineCommandReturnState));
	if (!ret) {
		return NULL;
	}
	ret->id = id;
	ret->fxslot = channel->fxslot;
	ret->volume = channel->volume;
	ret->panning = channel->panning;
	ret->mute = channel->mute;
	ret->mastersend = channel->mastersend;
	send = psy_audio_mixer_Send(mixer, id);
	ret->inputconvol = send ? send->inputconvol : 1.0;
	ret->sendsto = NULL;
	ret->incoming_from = NULL;
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
	/*
	** ondisconnected() also removes routes from surviving returns that target
	** this return. Capture those edges separately so undo can add back only the
	** entries teardown destroyed without rolling back any newer state on the
	** surviving source return.
	*/
	for (route_it = psy_table_begin(&mixer->returns);
			!psy_tableiterator_equal(&route_it, psy_table_end());
			psy_tableiterator_inc(&route_it)) {
		uintptr_t source_id;
		psy_audio_ReturnChannel* source;
		MachineCommandRouteState* route;

		source_id = psy_tableiterator_key(&route_it);
		source = (psy_audio_ReturnChannel*)psy_tableiterator_value(&route_it);
		if (!source || source_id == id ||
				!psy_table_exists(&source->sendsto, id)) {
			continue;
		}
		route = (MachineCommandRouteState*)malloc(
			sizeof(MachineCommandRouteState));
		if (!route) {
			continue;
		}
		route->fxslot = source->fxslot;
		route->value = TRUE;
		psy_list_append(&ret->incoming_from, route);
	}
	return ret;
}

static void machinecommand_capture_mixer_snapshots(psy_audio_Machines* machines,
	uintptr_t affected_slot, psy_List** snapshots)
{
	psy_TableIterator machine_it;

	/*
	** Snapshot only Mixer channel objects that this machine command can destroy:
	** channels whose source/FX endpoint is the machine being detached. Mixer
	** tweaks are not part of the Machines undo stack, so recording every channel
	** here would incorrectly roll back newer edits made after the command.
	*/
	machinecommand_dispose_mixer_snapshots(snapshots);
	for (machine_it = psy_audio_machines_begin(machines);
			!psy_tableiterator_equal(&machine_it, psy_table_end());
			psy_tableiterator_inc(&machine_it)) {
		psy_audio_Machine* machine;
		psy_audio_Mixer* mixer;
		uintptr_t input_id;
		uintptr_t return_id;
		MachineCommandMixerState* state;

		machine = (psy_audio_Machine*)psy_tableiterator_value(&machine_it);
		mixer = machinecommand_mixer_client(machine);
		if (!mixer) {
			continue;
		}
		input_id = machinecommand_input_id(mixer, affected_slot);
		return_id = machinecommand_return_id(mixer, affected_slot);
		if (input_id == psy_INDEX_INVALID && return_id == psy_INDEX_INVALID) {
			continue;
		}
		state = (MachineCommandMixerState*)malloc(
			sizeof(MachineCommandMixerState));
		if (!state) {
			continue;
		}
		state->slot = psy_tableiterator_key(&machine_it);
		state->inputs = NULL;
		state->returns = NULL;
		if (input_id != psy_INDEX_INVALID) {
			psy_audio_InputChannel* channel;
			MachineCommandInputState* input;

			channel = psy_audio_mixer_channel(mixer, input_id);
			if (channel) {
				input = machinecommand_capture_input(mixer, channel);
				if (input) {
					psy_list_append(&state->inputs, input);
				}
			}
		}
		if (return_id != psy_INDEX_INVALID) {
			psy_audio_ReturnChannel* channel;
			MachineCommandReturnState* ret;

			channel = psy_audio_mixer_return(mixer, return_id);
			if (channel) {
				ret = machinecommand_capture_return(mixer, return_id, channel);
				if (ret) {
					psy_list_append(&state->returns, ret);
				}
			}
		}
		if (state->inputs || state->returns) {
			psy_list_append(snapshots, state);
		} else {
			machinecommand_mixerstate_dispose(state);
			free(state);
		}
	}
}

static MachineCommandMixerState* machinecommand_mixer_snapshot(
	psy_List* snapshots, uintptr_t slot)
{
	psy_List* p;

	for (p = snapshots; p != NULL; p = p->next) {
		MachineCommandMixerState* state;

		state = (MachineCommandMixerState*)p->entry;
		if (state && state->slot == slot) {
			return state;
		}
	}
	return NULL;
}

static MachineCommandMixerWireKind machinecommand_saved_mixer_wire_kind(
	psy_audio_Machines* machines, psy_List* snapshots, psy_audio_Wire wire)
{
	psy_audio_Machine* dst;
	psy_audio_Mixer* mixer;
	MachineCommandMixerState* state;
	psy_List* p;

	dst = psy_audio_machines_at(machines, wire.dst);
	mixer = machinecommand_mixer_client(dst);
	if (!mixer) {
		return MACHINECOMMAND_MIXER_WIRE_UNKNOWN;
	}
	/* A retained destination Mixer already knows the historical classification
	** for its preserved incident channel objects. */
	if (machinecommand_input_id(mixer, wire.src) != psy_INDEX_INVALID) {
		return MACHINECOMMAND_MIXER_WIRE_INPUT;
	}
	if (machinecommand_return_id(mixer, wire.src) != psy_INDEX_INVALID) {
		return MACHINECOMMAND_MIXER_WIRE_RETURN;
	}
	/* Otherwise use the destination-specific snapshot. Connections.sends is
	** global per source and cannot distinguish one source feeding two Mixers in
	** different modes. */
	state = machinecommand_mixer_snapshot(snapshots, wire.dst);
	if (!state) {
		return MACHINECOMMAND_MIXER_WIRE_UNKNOWN;
	}
	for (p = state->inputs; p != NULL; p = p->next) {
		MachineCommandInputState* input;

		input = (MachineCommandInputState*)p->entry;
		if (input && input->inputslot == wire.src) {
			return MACHINECOMMAND_MIXER_WIRE_INPUT;
		}
	}
	for (p = state->returns; p != NULL; p = p->next) {
		MachineCommandReturnState* ret;

		ret = (MachineCommandReturnState*)p->entry;
		if (ret && ret->fxslot == wire.src) {
			return MACHINECOMMAND_MIXER_WIRE_RETURN;
		}
	}
	return MACHINECOMMAND_MIXER_WIRE_UNKNOWN;
}

static void machinecommand_prepare_mixer_restore(psy_audio_Machines* machines,
	psy_List* snapshots)
{
	psy_List* p;

	for (p = snapshots; p != NULL; p = p->next) {
		MachineCommandMixerState* state;
		psy_audio_Machine* machine;
		psy_audio_Mixer* mixer;
		psy_List* q;

		state = (MachineCommandMixerState*)p->entry;
		machine = state ? psy_audio_machines_at(machines, state->slot) : NULL;
		mixer = machinecommand_mixer_client(machine);
		for (q = state ? state->inputs : NULL; q != NULL; q = q->next) {
			MachineCommandInputState* input;

			input = (MachineCommandInputState*)q->entry;
			input->survived = mixer && machinecommand_input_id(mixer,
				input->inputslot) != psy_INDEX_INVALID;
		}
	}
}

static uintptr_t machinecommand_restore_input_id(psy_audio_Mixer* mixer,
	const MachineCommandInputState* input)
{
	uintptr_t current_id;
	psy_audio_InputChannel* channel;

	current_id = machinecommand_input_id(mixer, input->inputslot);
	if (current_id == psy_INDEX_INVALID || current_id == input->id) {
		return current_id;
	}
	/* Keep a newer channel that legitimately occupied the old column. */
	if (psy_audio_mixer_channel(mixer, input->id)) {
		return current_id;
	}
	channel = psy_audio_mixer_channel(mixer, current_id);
	if (!channel) {
		return psy_INDEX_INVALID;
	}
	psy_table_remove(&mixer->inputs, current_id);
	channel->id = input->id;
	channel->gain_param.index = input->id;
	psy_audio_mixer_insertchannel(mixer, input->id, channel);
	return input->id;
}

static uintptr_t machinecommand_restore_return_id(psy_audio_Mixer* mixer,
	const MachineCommandReturnState* ret)
{
	uintptr_t current_id;
	psy_audio_ReturnChannel* channel;
	psy_audio_MixerSend* send;

	current_id = machinecommand_return_id(mixer, ret->fxslot);
	if (current_id == psy_INDEX_INVALID || current_id == ret->id) {
		return current_id;
	}
	/* The original return id is a hole left by teardown. Do not replace an
	** unrelated return if a future editing path has legitimately occupied it. */
	if (psy_audio_mixer_return(mixer, ret->id) ||
			psy_audio_mixer_Send(mixer, ret->id)) {
		return current_id;
	}
	channel = psy_audio_mixer_return(mixer, current_id);
	send = psy_audio_mixer_Send(mixer, current_id);
	if (!channel) {
		return psy_INDEX_INVALID;
	}
	psy_table_remove(&mixer->returns, current_id);
	if (send) {
		psy_table_remove(&mixer->sends, current_id);
	}
	channel->id = ret->id;
	psy_audio_mixer_insertreturn(mixer, ret->id, channel);
	if (send) {
		psy_audio_mixer_insertsend(mixer, ret->id, send);
	}
	return ret->id;
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
			/* A normal FX input can survive teardown. Its controls remain live and
			** may have been edited after the machine command, so never roll that
			** surviving object back to the deletion-time snapshot. */
			if (input->survived) {
				continue;
			}
			id = machinecommand_restore_input_id(mixer, input);
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
						(void*)(uintptr_t)route->value);
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
			id = machinecommand_restore_return_id(mixer, ret);
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
			for (r = ret->incoming_from; r != NULL; r = r->next) {
				MachineCommandRouteState* route;
				uintptr_t source_id;
				psy_audio_ReturnChannel* source;

				route = (MachineCommandRouteState*)r->entry;
				source_id = machinecommand_return_id(mixer, route->fxslot);
				if (source_id == psy_INDEX_INVALID || source_id == id) {
					continue;
				}
				source = psy_audio_mixer_return(mixer, source_id);
				if (source) {
					psy_table_insert(&source->sendsto, id,
						(void*)(uintptr_t)TRUE);
				}
			}
		}
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

static void machinecommand_rebuild_mixer_send_markers(psy_audio_Machines* machines)
{
	psy_TableIterator machine_it;

	/* Retained Mixers suppress their own teardown/reconnect callbacks to keep
	** channel objects alive. Rebuild the global send-marker set from the actual
	** live Mixer return channels so suppression cannot leave stale markers, and
	** multiple destination Mixers remain correctly represented. */
	psy_table_clear(&machines->connections.sends);
	for (machine_it = psy_audio_machines_begin(machines);
			!psy_tableiterator_equal(&machine_it, psy_table_end());
			psy_tableiterator_inc(&machine_it)) {
		psy_audio_Machine* machine;
		psy_audio_Mixer* mixer;
		psy_TableIterator return_it;

		machine = (psy_audio_Machine*)psy_tableiterator_value(&machine_it);
		mixer = machinecommand_mixer_client(machine);
		if (!mixer) {
			continue;
		}
		for (return_it = psy_table_begin(&mixer->returns);
				!psy_tableiterator_equal(&return_it, psy_table_end());
				psy_tableiterator_inc(&return_it)) {
			psy_audio_ReturnChannel* channel;
			MachineList* path;
			MachineList* p;

			channel = (psy_audio_ReturnChannel*)psy_tableiterator_value(&return_it);
			if (!channel) {
				continue;
			}
			path = psy_audio_compute_path(machines, channel->fxslot, FALSE);
			for (p = path; p != NULL; p = p->next) {
				uintptr_t slot;

				slot = (uintptr_t)p->entry;
				if (slot != psy_INDEX_INVALID) {
					psy_audio_machines_addmixersend(machines, slot);
				}
			}
			psy_list_free(path);
		}
	}
}

static void machinecommand_restore_wire_metadata(psy_audio_Connections* live,
	psy_audio_Connections* saved, psy_audio_Wire wire)
{
	const psy_audio_WireSocket* saved_input;
	psy_audio_WireSocket* live_input;

	saved_input = psy_audio_connections_input_const(saved, wire);
	live_input = psy_audio_connections_input(live, wire);
	if (!saved_input || !live_input) {
		return;
	}
	psy_audio_connections_set_wire_volume(live, wire, saved_input->volume);
	psy_audio_connections_setpinmapping(live, wire, &saved_input->mapping);
	live_input = psy_audio_connections_input(live, wire);
	if (live_input) {
		live_input->mute = saved_input->mute;
	}
}

static void machinecommand_connect_saved(psy_audio_Machines* machines,
	psy_audio_Connections* saved, psy_List* snapshots, psy_audio_Wire wire)
{
	bool previous_mode;
	bool suppress_existing_input;
	uintptr_t dst_slot;
	psy_audio_Machine* src;
	psy_audio_Machine* dst;
	MachineCommandMixerWireKind kind;

	previous_mode = psy_audio_machines_is_connect_as_mixersend(machines);
	suppress_existing_input = FALSE;
	dst_slot = psy_INDEX_INVALID;
	src = psy_audio_machines_at(machines, wire.src);
	dst = psy_audio_machines_at(machines, wire.dst);
	kind = MACHINECOMMAND_MIXER_WIRE_UNKNOWN;
	if (src && dst && psy_audio_machine_type(dst) == psy_audio_MIXER &&
			psy_audio_machine_mode(src) != psy_audio_MACHMODE_GENERATOR) {
		psy_audio_Mixer* mixer;

		kind = machinecommand_saved_mixer_wire_kind(machines, snapshots, wire);
		if (kind == MACHINECOMMAND_MIXER_WIRE_RETURN) {
			psy_audio_machines_connect_as_mixersend(machines);
		} else if (kind == MACHINECOMMAND_MIXER_WIRE_INPUT) {
			psy_audio_machines_connect_as_mixerinput(machines);
			/* mixer.c::ondisconnected() classifies every non-generator as a
			** return, so a normal FX input can leave its InputChannel alive while
			** the wire is absent. Suppress only this destination Mixer's synthetic
			** reconnect callback and reuse that live object. */
			mixer = machinecommand_mixer_client(dst);
			if (mixer && machinecommand_input_id(mixer, wire.src) !=
					psy_INDEX_INVALID && psy_audio_machine_slot(dst) == wire.dst) {
				dst_slot = wire.dst;
				psy_audio_machine_set_slot(dst, psy_INDEX_INVALID);
				suppress_existing_input = TRUE;
			}
		}
	}
	psy_audio_connections_connect(&machines->connections, wire);
	if (suppress_existing_input) {
		psy_audio_machine_set_slot(dst, dst_slot);
	}
	if (previous_mode) {
		psy_audio_machines_connect_as_mixersend(machines);
	} else {
		psy_audio_machines_connect_as_mixerinput(machines);
	}
	machinecommand_restore_wire_metadata(&machines->connections, saved, wire);
}

static void machinecommand_restore_connections(psy_audio_Machines* machines,
	psy_audio_Connections* saved, psy_List* snapshots)
{
	psy_audio_Connections* live;
	psy_audio_Connections before;
	psy_TableIterator slot_it;

	live = &machines->connections;
	/* Reconcile the topology delta through signal-emitting operations so
	** connection-aware machines rebuild internal state. Metadata is restored
	** only on wires that are actually recreated; unaffected live wires keep any
	** newer volume/pin/mute edits made after the machine command. */
	psy_audio_connections_init(&before);
	psy_audio_connections_copy(&before, live);

	for (slot_it = psy_table_begin(&before.container);
			!psy_tableiterator_equal(&slot_it, psy_table_end());
			psy_tableiterator_inc(&slot_it)) {
		psy_audio_MachineSockets* sockets;
		psy_TableIterator wire_it;
		uintptr_t srcslot;

		srcslot = psy_tableiterator_key(&slot_it);
		sockets = (psy_audio_MachineSockets*)psy_tableiterator_value(&slot_it);
		for (wire_it = psy_audio_wiresockets_begin(&sockets->outputs);
				!psy_tableiterator_equal(&wire_it, psy_table_end());
				psy_tableiterator_inc(&wire_it)) {
			psy_audio_WireSocket* socket;
			psy_audio_Wire wire;

			socket = (psy_audio_WireSocket*)psy_tableiterator_value(&wire_it);
			wire = psy_audio_wire_make(srcslot, socket->slot);
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
		uintptr_t srcslot;

		srcslot = psy_tableiterator_key(&slot_it);
		sockets = (psy_audio_MachineSockets*)psy_tableiterator_value(&slot_it);
		for (wire_it = psy_audio_wiresockets_begin(&sockets->outputs);
				!psy_tableiterator_equal(&wire_it, psy_table_end());
				psy_tableiterator_inc(&wire_it)) {
			psy_audio_WireSocket* socket;
			psy_audio_MachineSockets* dst_sockets;
			uintptr_t src_id;
			uintptr_t dst_id;
			psy_audio_Wire wire;

			socket = (psy_audio_WireSocket*)psy_tableiterator_value(&wire_it);
			src_id = psy_tableiterator_key(&wire_it);
			dst_sockets = psy_audio_connections_at(saved, socket->slot);
			dst_id = dst_sockets
				? psy_audio_connection_id(&dst_sockets->inputs, srcslot)
				: psy_INDEX_INVALID;
			wire = psy_audio_wire_makeall(srcslot, src_id, socket->slot, dst_id);
			if (!psy_audio_connections_connected(live, wire)) {
				machinecommand_connect_saved(machines, saved, snapshots, wire);
			}
		}
	}

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
		machinecommand_prepare_mixer_restore(self->machines,
			self->mixer_snapshots);
		/* Keep the retained machine from consuming its own synthetic restore
		** notifications; its preserved internal state already represents those
		** incident wires. Other machines still receive the topology delta. */
		machinecommand_mark_detached(self->machine);
		machinecommand_restore_connections(self->machines, &self->connections,
			self->mixer_snapshots);
		machinecommand_restore_mixer_snapshots(self->machines,
			self->mixer_snapshots);
		psy_audio_machine_set_slot(self->machine, self->slot);
		machinecommand_rebuild_mixer_send_markers(self->machines);
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
		machinecommand_capture_mixer_snapshots(self->machines, self->slot,
			&self->mixer_snapshots);
		self->restoreconnection = TRUE;
		self->machine = machine;
		machinecommand_mark_detached(machine);
		psy_audio_machines_erase(self->machines, self->slot);
		self->machine_detached = TRUE;
		machinecommand_rebuild_mixer_send_markers(self->machines);
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
		machinecommand_capture_mixer_snapshots(self->machines, self->slot,
			&self->mixer_snapshots);
		self->machine = machine;
		psy_audio_connections_rewire(&self->machines->connections,
			psy_audio_connections_at(&self->machines->connections, self->slot));
		machinecommand_mark_detached(machine);
		psy_audio_machines_erase(self->machines, self->slot);
		self->machine_detached = TRUE;
		machinecommand_rebuild_mixer_send_markers(self->machines);
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
		machinecommand_prepare_mixer_restore(self->machines,
			self->mixer_snapshots);
		/* The retained machine already owns the channel state for its original
		** incident wires. Suppress only its own restore callbacks while the
		** signal-emitting delta repairs downstream connection-aware machines. */
		machinecommand_mark_detached(self->machine);
		machinecommand_restore_connections(self->machines, &self->connections,
			self->mixer_snapshots);
		machinecommand_restore_mixer_snapshots(self->machines,
			self->mixer_snapshots);
		psy_audio_machine_set_slot(self->machine, self->slot);
		machinecommand_rebuild_mixer_send_markers(self->machines);
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
