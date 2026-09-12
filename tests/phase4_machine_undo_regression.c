/*
** Regression for undo/redo ownership of built-in Psycle machines.
**
** Internal machines inherit the base Machine clone implementation, which
** returns NULL. Undo/redo must therefore preserve the actual machine object
** rather than depending on clone support.
*/

#include <stdio.h>

#include <machinefactory.h>
#include <machineproxy.h>
#include <machines.h>
#include <mixer.h>
#include <player.h>
#include <plugincatcher.h>
#include <song.h>
#include <undoredo.h>
#include <wire.h>

#define SAMPLER_SLOT 0
#define MIXER_SLOT 1
#define DOWNSTREAM_MIXER_SLOT 2
#define ROUTE_MIXER_SLOT 3
#define RAW_MIXER_SLOT 4
#define INPUT_MIXER_SLOT 5
#define INPUT_LOW_SLOT 6
#define INPUT_TARGET_SLOT 7
#define FX_INPUT_SOURCE_SLOT 8
#define FX_INPUT_MIXER_SLOT 9
#define MIXER_INPUT_VOLUME 0.37
#define MIXER_INPUT_PANNING 0.23
#define MIXER_INPUT_GAIN 0.61
#define MIXER_INPUT_DRYMIX 0.42
#define DOWNSTREAM_RETURN_VOLUME 0.44
#define DOWNSTREAM_RETURN_PANNING 0.71
#define DOWNSTREAM_RETURN_INPUTCONVOL 0.58
#define NEWER_ROUTE_RETURN_VOLUME 0.29
#define NEWER_ROUTE_RETURN_PANNING 0.18
#define TARGET_INPUT_VOLUME 0.36
#define TARGET_INPUT_GAIN 0.63
#define FX_NORMAL_INPUT_VOLUME 0.52

static int fail(const char* message)
{
	fprintf(stderr, "phase4-machine-undo: FAIL: %s\n", message);
	return 1;
}

static int topology_is_routed(psy_audio_Machines* machines)
{
	return psy_audio_machines_at(machines, MIXER_SLOT) &&
		psy_audio_machines_at(machines, DOWNSTREAM_MIXER_SLOT) &&
		psy_audio_machines_at(machines, ROUTE_MIXER_SLOT) &&
		psy_audio_machines_connected(machines,
			psy_audio_wire_make(SAMPLER_SLOT, MIXER_SLOT)) &&
		psy_audio_machines_connected(machines,
			psy_audio_wire_make(MIXER_SLOT, DOWNSTREAM_MIXER_SLOT)) &&
		psy_audio_machines_connected(machines,
			psy_audio_wire_make(ROUTE_MIXER_SLOT, DOWNSTREAM_MIXER_SLOT)) &&
		psy_audio_machines_connected(machines,
			psy_audio_wire_make(DOWNSTREAM_MIXER_SLOT,
				psy_audio_MASTER_INDEX)) &&
		!psy_audio_machines_connected(machines,
			psy_audio_wire_make(SAMPLER_SLOT, DOWNSTREAM_MIXER_SLOT)) &&
		!psy_audio_machines_connected(machines,
			psy_audio_wire_make(SAMPLER_SLOT, psy_audio_MASTER_INDEX));
}

static int topology_is_rewired(psy_audio_Machines* machines)
{
	return !psy_audio_machines_at(machines, MIXER_SLOT) &&
		psy_audio_machines_at(machines, DOWNSTREAM_MIXER_SLOT) &&
		psy_audio_machines_at(machines, ROUTE_MIXER_SLOT) &&
		psy_audio_machines_connected(machines,
			psy_audio_wire_make(SAMPLER_SLOT, DOWNSTREAM_MIXER_SLOT)) &&
		psy_audio_machines_connected(machines,
			psy_audio_wire_make(ROUTE_MIXER_SLOT, DOWNSTREAM_MIXER_SLOT)) &&
		psy_audio_machines_connected(machines,
			psy_audio_wire_make(DOWNSTREAM_MIXER_SLOT,
				psy_audio_MASTER_INDEX));
}

static psy_audio_Mixer* mixer_client(psy_audio_Machine* machine)
{
	psy_audio_MachineProxy* proxy;

	if (!machine || psy_audio_machine_type(machine) != psy_audio_MIXER) {
		return NULL;
	}
	/* Normal factory-created built-ins are proxied in this regression build. */
	proxy = (psy_audio_MachineProxy*)machine;
	if (!proxy->client || psy_audio_machine_type(proxy->client) != psy_audio_MIXER) {
		return NULL;
	}
	return (psy_audio_Mixer*)proxy->client;
}

static psy_audio_InputChannel* mixer_input_channel(psy_audio_Machine* machine)
{
	psy_audio_Mixer* mixer;

	mixer = mixer_client(machine);
	return mixer ? psy_audio_mixer_channel(mixer, 0) : NULL;
}

static uintptr_t mixer_input_id_for_slot(psy_audio_Mixer* mixer,
	uintptr_t inputslot)
{
	psy_TableIterator it;

	for (it = psy_table_begin(&mixer->inputs);
			!psy_tableiterator_equal(&it, psy_table_end());
			psy_tableiterator_inc(&it)) {
		psy_audio_InputChannel* input;

		input = (psy_audio_InputChannel*)psy_tableiterator_value(&it);
		if (input && input->inputslot == inputslot) {
			return psy_tableiterator_key(&it);
		}
	}
	return psy_INDEX_INVALID;
}

static uintptr_t mixer_input_count_for_slot(psy_audio_Mixer* mixer,
	uintptr_t inputslot)
{
	uintptr_t rv;
	psy_TableIterator it;

	rv = 0;
	for (it = psy_table_begin(&mixer->inputs);
			!psy_tableiterator_equal(&it, psy_table_end());
			psy_tableiterator_inc(&it)) {
		psy_audio_InputChannel* input;

		input = (psy_audio_InputChannel*)psy_tableiterator_value(&it);
		if (input && input->inputslot == inputslot) {
			++rv;
		}
	}
	return rv;
}

static uintptr_t mixer_return_id_for_slot(psy_audio_Mixer* mixer,
	uintptr_t fxslot)
{
	psy_TableIterator it;

	for (it = psy_table_begin(&mixer->returns);
			!psy_tableiterator_equal(&it, psy_table_end());
			psy_tableiterator_inc(&it)) {
		psy_audio_ReturnChannel* ret;

		ret = (psy_audio_ReturnChannel*)psy_tableiterator_value(&it);
		if (ret && ret->fxslot == fxslot) {
			return psy_tableiterator_key(&it);
		}
	}
	return psy_INDEX_INVALID;
}

static int mixer_input_state_is_preserved(psy_audio_Machine* machine)
{
	psy_audio_InputChannel* input;

	input = mixer_input_channel(machine);
	return input && input->inputslot == SAMPLER_SLOT &&
		input->volume == MIXER_INPUT_VOLUME &&
		input->panning == MIXER_INPUT_PANNING &&
		input->gain == MIXER_INPUT_GAIN &&
		input->drymix == MIXER_INPUT_DRYMIX;
}

static int downstream_mixer_is_routed(psy_audio_Machines* machines)
{
	psy_audio_Mixer* mixer;
	uintptr_t deleted_return_id;
	uintptr_t route_return_id;

	mixer = mixer_client(psy_audio_machines_at(machines,
		DOWNSTREAM_MIXER_SLOT));
	if (!mixer) {
		return FALSE;
	}
	deleted_return_id = mixer_return_id_for_slot(mixer, MIXER_SLOT);
	route_return_id = mixer_return_id_for_slot(mixer, ROUTE_MIXER_SLOT);
	return deleted_return_id != psy_INDEX_INVALID &&
		route_return_id != psy_INDEX_INVALID &&
		!psy_audio_mixer_channel(mixer, 0);
}

static int downstream_mixer_is_rewired(psy_audio_Machines* machines)
{
	psy_audio_Mixer* mixer;
	psy_audio_InputChannel* input;

	mixer = mixer_client(psy_audio_machines_at(machines,
		DOWNSTREAM_MIXER_SLOT));
	if (!mixer) {
		return FALSE;
	}
	input = psy_audio_mixer_channel(mixer, 0);
	return input && input->inputslot == SAMPLER_SLOT &&
		mixer_return_id_for_slot(mixer, MIXER_SLOT) == psy_INDEX_INVALID &&
		mixer_return_id_for_slot(mixer, ROUTE_MIXER_SLOT) != psy_INDEX_INVALID;
}

static int downstream_return_state_is_preserved(psy_audio_Machines* machines,
	uintptr_t expected_return_id)
{
	psy_audio_Mixer* mixer;
	uintptr_t deleted_return_id;
	uintptr_t route_return_id;
	psy_audio_ReturnChannel* ret;
	psy_audio_ReturnChannel* route_ret;
	psy_audio_MixerSend* send;

	mixer = mixer_client(psy_audio_machines_at(machines,
		DOWNSTREAM_MIXER_SLOT));
	if (!mixer) {
		return FALSE;
	}
	deleted_return_id = mixer_return_id_for_slot(mixer, MIXER_SLOT);
	route_return_id = mixer_return_id_for_slot(mixer, ROUTE_MIXER_SLOT);
	if (deleted_return_id == psy_INDEX_INVALID ||
			route_return_id == psy_INDEX_INVALID ||
			deleted_return_id != expected_return_id) {
		return FALSE;
	}
	ret = psy_audio_mixer_return(mixer, deleted_return_id);
	route_ret = psy_audio_mixer_return(mixer, route_return_id);
	send = psy_audio_mixer_Send(mixer, deleted_return_id);
	return ret && route_ret && send &&
		ret->id == expected_return_id &&
		ret->volume == DOWNSTREAM_RETURN_VOLUME &&
		ret->panning == DOWNSTREAM_RETURN_PANNING &&
		ret->mute == 1 &&
		ret->mastersend == 0 &&
		send->inputconvol == DOWNSTREAM_RETURN_INPUTCONVOL &&
		psy_table_exists(&ret->sendsto, route_return_id) &&
		psy_table_exists(&route_ret->sendsto, deleted_return_id) &&
		mixer->returnsolo == expected_return_id;
}

static int newer_unaffected_return_edit_is_preserved(
	psy_audio_Machines* machines)
{
	psy_audio_Mixer* mixer;
	uintptr_t route_return_id;
	psy_audio_ReturnChannel* ret;

	mixer = mixer_client(psy_audio_machines_at(machines,
		DOWNSTREAM_MIXER_SLOT));
	if (!mixer) {
		return FALSE;
	}
	route_return_id = mixer_return_id_for_slot(mixer, ROUTE_MIXER_SLOT);
	if (route_return_id == psy_INDEX_INVALID) {
		return FALSE;
	}
	ret = psy_audio_mixer_return(mixer, route_return_id);
	return ret && ret->volume == NEWER_ROUTE_RETURN_VOLUME &&
		ret->panning == NEWER_ROUTE_RETURN_PANNING && ret->mute == 1;
}

int main(void)
{
	psy_audio_MachineCallback callback;
	psy_audio_PluginCatcher catcher;
	psy_audio_MachineFactory factory;
	psy_audio_Song* song;
	psy_audio_Machines* machines;
	psy_audio_Machine* sampler;
	psy_audio_Machine* mixer;
	psy_audio_Machine* downstream_mixer;
	psy_audio_Machine* route_mixer;
	psy_audio_Machine* raw_mixer;
	psy_audio_Machine* input_mixer_machine;
	psy_audio_Machine* input_low_sampler;
	psy_audio_Machine* input_target_sampler;
	psy_audio_Machine* fx_input_source;
	psy_audio_Machine* fx_input_mixer_machine;
	psy_audio_InputChannel* input;
	psy_audio_InputChannel* target_input;
	psy_audio_InputChannel* fx_input;
	psy_audio_Mixer* downstream;
	psy_audio_Mixer* input_mixer;
	psy_audio_Mixer* fx_input_mixer;
	uintptr_t deleted_return_id;
	uintptr_t route_return_id;
	uintptr_t target_input_id;
	uintptr_t restored_input_id;
	uintptr_t fx_input_id;
	psy_audio_ReturnChannel* downstream_return;
	psy_audio_ReturnChannel* route_return;
	psy_audio_MixerSend* downstream_send;

	psy_audio_init();
	psy_audio_machinecallback_init(&callback);
	psy_audio_plugincatcher_init(&catcher, NULL);
	psy_audio_machinefactory_init(&factory, &callback, &catcher, NULL);
	song = psy_audio_song_alloc_init(&factory);
	if (!song) {
		return fail("could not allocate song");
	}
	machines = psy_audio_song_machines(song);

	sampler = psy_audio_machinefactory_make_machine_from_path(&factory,
		psy_audio_SAMPLER, NULL, 0, psy_INDEX_INVALID);
	mixer = psy_audio_machinefactory_make_machine_from_path(&factory,
		psy_audio_MIXER, NULL, 0, psy_INDEX_INVALID);
	downstream_mixer = psy_audio_machinefactory_make_machine_from_path(&factory,
		psy_audio_MIXER, NULL, 0, psy_INDEX_INVALID);
	route_mixer = psy_audio_machinefactory_make_machine_from_path(&factory,
		psy_audio_MIXER, NULL, 0, psy_INDEX_INVALID);
	if (!sampler || !mixer || !downstream_mixer || !route_mixer) {
		psy_audio_song_deallocate(song);
		return fail("could not create built-in machines");
	}

	psy_audio_machines_insert(machines, SAMPLER_SLOT, sampler);
	psy_audio_machines_insert(machines, MIXER_SLOT, mixer);
	if (!psy_audio_machines_at(machines, MIXER_SLOT)) {
		psy_audio_song_deallocate(song);
		return fail("mixer insert failed");
	}

	/* Undo/redo creation must work even though the Mixer has no clone method. */
	psy_undoredo_undo(&machines->undoredo);
	if (psy_audio_machines_at(machines, MIXER_SLOT)) {
		psy_audio_song_deallocate(song);
		return fail("insert undo did not detach mixer");
	}
	psy_undoredo_redo(&machines->undoredo);
	if (!psy_audio_machines_at(machines, MIXER_SLOT)) {
		psy_audio_song_deallocate(song);
		return fail("insert redo did not restore mixer");
	}

	psy_audio_machines_insert(machines, DOWNSTREAM_MIXER_SLOT,
		downstream_mixer);
	psy_audio_machines_insert(machines, ROUTE_MIXER_SLOT, route_mixer);
	psy_audio_machines_connect(machines,
		psy_audio_wire_make(SAMPLER_SLOT, MIXER_SLOT));
	psy_audio_machines_connect(machines,
		psy_audio_wire_make(MIXER_SLOT, DOWNSTREAM_MIXER_SLOT));
	psy_audio_machines_connect(machines,
		psy_audio_wire_make(ROUTE_MIXER_SLOT, DOWNSTREAM_MIXER_SLOT));
	psy_audio_machines_connect(machines,
		psy_audio_wire_make(DOWNSTREAM_MIXER_SLOT, psy_audio_MASTER_INDEX));
	if (!topology_is_routed(machines)) {
		psy_audio_song_deallocate(song);
		return fail("initial routed topology is invalid");
	}
	if (!downstream_mixer_is_routed(machines)) {
		psy_audio_song_deallocate(song);
		return fail("downstream mixer did not create routed return channels");
	}

	/* Give the deleted Mixer's input channel non-default state. A reversible
	** machine deletion must not let connection teardown destroy this object. */
	input = mixer_input_channel(psy_audio_machines_at(machines, MIXER_SLOT));
	if (!input || input->inputslot != SAMPLER_SLOT) {
		psy_audio_song_deallocate(song);
		return fail("mixer input channel was not created");
	}
	input->volume = MIXER_INPUT_VOLUME;
	input->panning = MIXER_INPUT_PANNING;
	input->gain = MIXER_INPUT_GAIN;
	input->drymix = MIXER_INPUT_DRYMIX;

	/* Mixer A and the route Mixer occupy two return columns in downstream B.
	** A is deliberately the non-highest return, so reconnecting it would append
	** at a new id unless undo explicitly restores the original column identity. */
	downstream = mixer_client(psy_audio_machines_at(machines,
		DOWNSTREAM_MIXER_SLOT));
	deleted_return_id = mixer_return_id_for_slot(downstream, MIXER_SLOT);
	route_return_id = mixer_return_id_for_slot(downstream, ROUTE_MIXER_SLOT);
	if (deleted_return_id == psy_INDEX_INVALID ||
			route_return_id == psy_INDEX_INVALID ||
			deleted_return_id >= route_return_id) {
		psy_audio_song_deallocate(song);
		return fail("downstream return ids do not exercise the non-highest case");
	}
	downstream_return = psy_audio_mixer_return(downstream, deleted_return_id);
	route_return = psy_audio_mixer_return(downstream, route_return_id);
	downstream_send = psy_audio_mixer_Send(downstream, deleted_return_id);
	if (!downstream_return || !route_return || !downstream_send) {
		psy_audio_song_deallocate(song);
		return fail("downstream return/send pair is missing");
	}
	downstream_return->volume = DOWNSTREAM_RETURN_VOLUME;
	downstream_return->panning = DOWNSTREAM_RETURN_PANNING;
	downstream_return->mute = 1;
	downstream_return->mastersend = 0;
	downstream_send->inputconvol = DOWNSTREAM_RETURN_INPUTCONVOL;
	/* Exercise both directions. A->C belongs to A's destroyed object, while
	** C->A is stored on a surviving return and is explicitly removed by
	** mixer.c::ondisconnected() when A disappears. */
	psy_table_insert(&downstream_return->sendsto, route_return_id,
		(void*)(uintptr_t)TRUE);
	psy_table_insert(&route_return->sendsto, deleted_return_id,
		(void*)(uintptr_t)TRUE);
	/* A solo index survives deletion as an integer. Restoring A at the same id
	** is what makes that solo state meaningful again without rolling back any
	** newer solo choice the user might make while A is absent. */
	downstream->returnsolo = deleted_return_id;

	psy_audio_machines_remove(machines, MIXER_SLOT, TRUE);
	if (!topology_is_rewired(machines)) {
		psy_audio_song_deallocate(song);
		return fail("delete did not rewire sampler through downstream mixer");
	}
	if (!downstream_mixer_is_rewired(machines)) {
		psy_audio_song_deallocate(song);
		return fail("delete did not rebuild downstream mixer for temporary wire");
	}
	downstream = mixer_client(psy_audio_machines_at(machines,
		DOWNSTREAM_MIXER_SLOT));
	route_return_id = mixer_return_id_for_slot(downstream, ROUTE_MIXER_SLOT);
	route_return = psy_audio_mixer_return(downstream, route_return_id);
	if (!route_return || psy_table_exists(&route_return->sendsto,
			deleted_return_id)) {
		psy_audio_song_deallocate(song);
		return fail("delete did not remove incoming route to deleted return");
	}

	/* Mixer parameter tweaks are independent of Machines undo. Change the
	** persistent route return after deletion; undo must not roll this newer,
	** unrelated edit back to the deletion-time snapshot. */
	route_return->volume = NEWER_ROUTE_RETURN_VOLUME;
	route_return->panning = NEWER_ROUTE_RETURN_PANNING;
	route_return->mute = 1;

	/* The saved A->B wire is a Mixer return. Deliberately switch the current
	** toolbar mode to normal Mixer input before undo; restoration must replay
	** the saved return classification without changing this current preference. */
	psy_audio_machines_connect_as_mixerinput(machines);
	psy_undoredo_undo(&machines->undoredo);
	if (psy_audio_machines_is_connect_as_mixersend(machines)) {
		psy_audio_song_deallocate(song);
		return fail("delete undo changed the current mixer-input toolbar mode");
	}
	if (!topology_is_routed(machines)) {
		psy_audio_song_deallocate(song);
		return fail("delete undo did not restore mixer and connections");
	}
	if (!mixer_input_state_is_preserved(
			psy_audio_machines_at(machines, MIXER_SLOT))) {
		psy_audio_song_deallocate(song);
		return fail("delete undo did not preserve mixer input-channel state");
	}
	if (!downstream_mixer_is_routed(machines)) {
		psy_audio_song_deallocate(song);
		return fail("delete undo did not restore saved Mixer-return classification");
	}
	if (!downstream_return_state_is_preserved(machines, deleted_return_id)) {
		psy_audio_song_deallocate(song);
		return fail("delete undo did not preserve return id/routes/settings");
	}
	if (!newer_unaffected_return_edit_is_preserved(machines)) {
		psy_audio_song_deallocate(song);
		return fail("delete undo reverted a newer unrelated Mixer edit");
	}

	psy_undoredo_redo(&machines->undoredo);
	if (!topology_is_rewired(machines)) {
		psy_audio_song_deallocate(song);
		return fail("delete redo did not reapply rewired topology");
	}
	if (!downstream_mixer_is_rewired(machines)) {
		psy_audio_song_deallocate(song);
		return fail("delete redo did not rebuild downstream temporary channel");
	}
	if (!newer_unaffected_return_edit_is_preserved(machines)) {
		psy_audio_song_deallocate(song);
		return fail("delete redo reverted the newer unrelated Mixer edit");
	}
	psy_undoredo_undo(&machines->undoredo);
	if (psy_audio_machines_is_connect_as_mixersend(machines)) {
		psy_audio_song_deallocate(song);
		return fail("second delete undo changed the current mixer-input mode");
	}
	if (!topology_is_routed(machines)) {
		psy_audio_song_deallocate(song);
		return fail("second delete undo did not restore topology");
	}
	if (!mixer_input_state_is_preserved(
			psy_audio_machines_at(machines, MIXER_SLOT))) {
		psy_audio_song_deallocate(song);
		return fail("second delete undo did not preserve mixer input-channel state");
	}
	if (!downstream_mixer_is_routed(machines)) {
		psy_audio_song_deallocate(song);
		return fail("second delete undo did not reconcile downstream mixer state");
	}
	if (!downstream_return_state_is_preserved(machines, deleted_return_id)) {
		psy_audio_song_deallocate(song);
		return fail("second undo did not preserve return id/routes/settings");
	}
	if (!newer_unaffected_return_edit_is_preserved(machines)) {
		psy_audio_song_deallocate(song);
		return fail("second delete undo reverted the newer unrelated Mixer edit");
	}

	/* createwithoutproxy() is supported even in proxy-enabled builds. Exercise
	** both insertion undo/redo and deletion undo with a raw Mixer so command
	** snapshot code cannot safely assume every Mixer instance is a proxy. */
	psy_audio_machinefactory_createwithoutproxy(&factory);
	raw_mixer = psy_audio_machinefactory_make_machine_from_path(&factory,
		psy_audio_MIXER, NULL, 0, psy_INDEX_INVALID);
	psy_audio_machinefactory_createasproxy(&factory);
	if (!raw_mixer) {
		psy_audio_song_deallocate(song);
		return fail("could not create raw Mixer");
	}
	psy_audio_machines_insert(machines, RAW_MIXER_SLOT, raw_mixer);
	if (psy_audio_machines_at(machines, RAW_MIXER_SLOT) != raw_mixer) {
		psy_audio_song_deallocate(song);
		return fail("raw Mixer insert failed");
	}
	psy_undoredo_undo(&machines->undoredo);
	if (psy_audio_machines_at(machines, RAW_MIXER_SLOT)) {
		psy_audio_song_deallocate(song);
		return fail("raw Mixer insert undo failed");
	}
	psy_undoredo_redo(&machines->undoredo);
	if (psy_audio_machines_at(machines, RAW_MIXER_SLOT) != raw_mixer) {
		psy_audio_song_deallocate(song);
		return fail("raw Mixer insert redo failed");
	}
	psy_audio_machines_remove(machines, RAW_MIXER_SLOT, FALSE);
	if (psy_audio_machines_at(machines, RAW_MIXER_SLOT)) {
		psy_audio_song_deallocate(song);
		return fail("raw Mixer delete failed");
	}
	psy_undoredo_undo(&machines->undoredo);
	if (psy_audio_machines_at(machines, RAW_MIXER_SLOT) != raw_mixer) {
		psy_audio_song_deallocate(song);
		return fail("raw Mixer delete undo failed");
	}

	/* A generator occupying input ID 1 must return to that exact column after
	** deletion even when input ID 0 is vacant. Otherwise inputsolo and the
	** gain parameter's ID-dependent metadata point at the wrong column. */
	input_mixer_machine = psy_audio_machinefactory_make_machine_from_path(&factory,
		psy_audio_MIXER, NULL, 0, psy_INDEX_INVALID);
	input_low_sampler = psy_audio_machinefactory_make_machine_from_path(&factory,
		psy_audio_SAMPLER, NULL, 0, psy_INDEX_INVALID);
	input_target_sampler = psy_audio_machinefactory_make_machine_from_path(&factory,
		psy_audio_SAMPLER, NULL, 0, psy_INDEX_INVALID);
	if (!input_mixer_machine || !input_low_sampler || !input_target_sampler) {
		psy_audio_song_deallocate(song);
		return fail("could not create Mixer input-id regression machines");
	}
	psy_audio_machines_insert(machines, INPUT_MIXER_SLOT, input_mixer_machine);
	psy_audio_machines_insert(machines, INPUT_LOW_SLOT, input_low_sampler);
	psy_audio_machines_insert(machines, INPUT_TARGET_SLOT, input_target_sampler);
	psy_audio_machines_connect(machines,
		psy_audio_wire_make(INPUT_LOW_SLOT, INPUT_MIXER_SLOT));
	psy_audio_machines_connect(machines,
		psy_audio_wire_make(INPUT_TARGET_SLOT, INPUT_MIXER_SLOT));
	input_mixer = mixer_client(psy_audio_machines_at(machines, INPUT_MIXER_SLOT));
	if (!input_mixer) {
		psy_audio_song_deallocate(song);
		return fail("input-id regression Mixer is unavailable");
	}
	target_input_id = mixer_input_id_for_slot(input_mixer, INPUT_TARGET_SLOT);
	if (mixer_input_id_for_slot(input_mixer, INPUT_LOW_SLOT) != 0 ||
			target_input_id == psy_INDEX_INVALID || target_input_id == 0) {
		psy_audio_song_deallocate(song);
		return fail("input-id regression did not allocate target above ID 0");
	}
	target_input = psy_audio_mixer_channel(input_mixer, target_input_id);
	if (!target_input) {
		psy_audio_song_deallocate(song);
		return fail("target input channel is missing");
	}
	target_input->volume = TARGET_INPUT_VOLUME;
	target_input->gain = TARGET_INPUT_GAIN;
	input_mixer->inputsolo = target_input_id;
	psy_audio_machines_disconnect(machines,
		psy_audio_wire_make(INPUT_LOW_SLOT, INPUT_MIXER_SLOT));
	if (mixer_input_id_for_slot(input_mixer, INPUT_LOW_SLOT) != psy_INDEX_INVALID ||
			mixer_input_id_for_slot(input_mixer, INPUT_TARGET_SLOT) != target_input_id) {
		psy_audio_song_deallocate(song);
		return fail("input-id regression did not leave the lower ID vacant");
	}
	psy_audio_machines_remove(machines, INPUT_TARGET_SLOT, FALSE);
	if (mixer_input_id_for_slot(input_mixer, INPUT_TARGET_SLOT) != psy_INDEX_INVALID) {
		psy_audio_song_deallocate(song);
		return fail("target generator deletion did not remove its input channel");
	}
	psy_undoredo_undo(&machines->undoredo);
	restored_input_id = mixer_input_id_for_slot(input_mixer, INPUT_TARGET_SLOT);
	target_input = (restored_input_id == psy_INDEX_INVALID)
		? NULL : psy_audio_mixer_channel(input_mixer, restored_input_id);
	if (!target_input || restored_input_id != target_input_id ||
			target_input->id != target_input_id ||
			target_input->gain_param.index != target_input_id ||
			input_mixer->inputsolo != target_input_id ||
			target_input->volume != TARGET_INPUT_VOLUME ||
			target_input->gain != TARGET_INPUT_GAIN ||
			psy_audio_mixer_channel(input_mixer, 0)) {
		psy_audio_song_deallocate(song);
		return fail("generator delete undo did not restore the original input ID");
	}

	/* Non-generator FX normal inputs survive mixer.c::ondisconnected(). Undo
	** must reuse that surviving channel while restoring the wire, not allocate a
	** second InputChannel for the same source. */
	fx_input_source = psy_audio_machinefactory_make_machine_from_path(&factory,
		psy_audio_MIXER, NULL, 0, psy_INDEX_INVALID);
	fx_input_mixer_machine = psy_audio_machinefactory_make_machine_from_path(&factory,
		psy_audio_MIXER, NULL, 0, psy_INDEX_INVALID);
	if (!fx_input_source || !fx_input_mixer_machine) {
		psy_audio_song_deallocate(song);
		return fail("could not create normal-FX-input regression machines");
	}
	psy_audio_machines_insert(machines, FX_INPUT_SOURCE_SLOT, fx_input_source);
	psy_audio_machines_insert(machines, FX_INPUT_MIXER_SLOT,
		fx_input_mixer_machine);
	psy_audio_machines_connect_as_mixerinput(machines);
	psy_audio_machines_connect(machines,
		psy_audio_wire_make(FX_INPUT_SOURCE_SLOT, FX_INPUT_MIXER_SLOT));
	fx_input_mixer = mixer_client(psy_audio_machines_at(machines,
		FX_INPUT_MIXER_SLOT));
	if (!fx_input_mixer ||
			mixer_input_count_for_slot(fx_input_mixer, FX_INPUT_SOURCE_SLOT) != 1) {
		psy_audio_song_deallocate(song);
		return fail("normal FX input did not create exactly one Mixer channel");
	}
	fx_input_id = mixer_input_id_for_slot(fx_input_mixer, FX_INPUT_SOURCE_SLOT);
	fx_input = psy_audio_mixer_channel(fx_input_mixer, fx_input_id);
	if (!fx_input) {
		psy_audio_song_deallocate(song);
		return fail("normal FX input channel is missing");
	}
	fx_input->volume = FX_NORMAL_INPUT_VOLUME;
	psy_audio_machines_remove(machines, FX_INPUT_SOURCE_SLOT, FALSE);
	if (mixer_input_count_for_slot(fx_input_mixer, FX_INPUT_SOURCE_SLOT) != 1 ||
			psy_audio_machines_connected(machines,
				psy_audio_wire_make(FX_INPUT_SOURCE_SLOT, FX_INPUT_MIXER_SLOT))) {
		psy_audio_song_deallocate(song);
		return fail("normal FX delete did not leave the expected surviving input");
	}
	psy_undoredo_undo(&machines->undoredo);
	fx_input_id = mixer_input_id_for_slot(fx_input_mixer, FX_INPUT_SOURCE_SLOT);
	fx_input = (fx_input_id == psy_INDEX_INVALID)
		? NULL : psy_audio_mixer_channel(fx_input_mixer, fx_input_id);
	if (!fx_input ||
			mixer_input_count_for_slot(fx_input_mixer, FX_INPUT_SOURCE_SLOT) != 1 ||
			!psy_audio_machines_connected(machines,
				psy_audio_wire_make(FX_INPUT_SOURCE_SLOT, FX_INPUT_MIXER_SLOT)) ||
			fx_input->volume != FX_NORMAL_INPUT_VOLUME) {
		psy_audio_song_deallocate(song);
		return fail("normal FX delete undo duplicated or reset its Mixer input");
	}

	psy_audio_song_deallocate(song);
	psy_audio_machinefactory_dispose(&factory);
	psy_audio_plugincatcher_dispose(&catcher);
	psy_audio_dispose();
	puts("phase4-machine-undo: PASS");
	return 0;
}
