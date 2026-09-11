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
#define MIXER_INPUT_VOLUME 0.37
#define MIXER_INPUT_PANNING 0.23
#define MIXER_INPUT_GAIN 0.61
#define MIXER_INPUT_DRYMIX 0.42

static int fail(const char* message)
{
	fprintf(stderr, "phase4-machine-undo: FAIL: %s\n", message);
	return 1;
}

static int topology_is_routed(psy_audio_Machines* machines)
{
	return psy_audio_machines_at(machines, MIXER_SLOT) &&
		psy_audio_machines_at(machines, DOWNSTREAM_MIXER_SLOT) &&
		psy_audio_machines_connected(machines,
			psy_audio_wire_make(SAMPLER_SLOT, MIXER_SLOT)) &&
		psy_audio_machines_connected(machines,
			psy_audio_wire_make(MIXER_SLOT, DOWNSTREAM_MIXER_SLOT)) &&
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
		psy_audio_machines_connected(machines,
			psy_audio_wire_make(SAMPLER_SLOT, DOWNSTREAM_MIXER_SLOT)) &&
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
	/* MachineFactory wraps built-ins in MachineProxy; inspect the retained
	** Mixer client so this regression can verify its internal channel state. */
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
	psy_audio_InputChannel* input;
	psy_audio_ReturnChannel* ret;

	mixer = mixer_client(psy_audio_machines_at(machines,
		DOWNSTREAM_MIXER_SLOT));
	if (!mixer) {
		return FALSE;
	}
	input = psy_audio_mixer_channel(mixer, 0);
	ret = psy_audio_mixer_return(mixer, 0);
	return !input && ret && ret->fxslot == MIXER_SLOT;
}

static int downstream_mixer_is_rewired(psy_audio_Machines* machines)
{
	psy_audio_Mixer* mixer;
	psy_audio_InputChannel* input;
	psy_audio_ReturnChannel* ret;

	mixer = mixer_client(psy_audio_machines_at(machines,
		DOWNSTREAM_MIXER_SLOT));
	if (!mixer) {
		return FALSE;
	}
	input = psy_audio_mixer_channel(mixer, 0);
	ret = psy_audio_mixer_return(mixer, 0);
	return input && input->inputslot == SAMPLER_SLOT && !ret;
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
	psy_audio_InputChannel* input;

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
	if (!sampler || !mixer || !downstream_mixer) {
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
	psy_audio_machines_connect(machines,
		psy_audio_wire_make(SAMPLER_SLOT, MIXER_SLOT));
	psy_audio_machines_connect(machines,
		psy_audio_wire_make(MIXER_SLOT, DOWNSTREAM_MIXER_SLOT));
	psy_audio_machines_connect(machines,
		psy_audio_wire_make(DOWNSTREAM_MIXER_SLOT, psy_audio_MASTER_INDEX));
	if (!topology_is_routed(machines)) {
		psy_audio_song_deallocate(song);
		return fail("initial routed topology is invalid");
	}
	if (!downstream_mixer_is_routed(machines)) {
		psy_audio_song_deallocate(song);
		return fail("downstream mixer did not create routed return channel");
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

	psy_audio_machines_remove(machines, MIXER_SLOT, TRUE);
	if (!topology_is_rewired(machines)) {
		psy_audio_song_deallocate(song);
		return fail("delete did not rewire sampler through downstream mixer");
	}
	if (!downstream_mixer_is_rewired(machines)) {
		psy_audio_song_deallocate(song);
		return fail("delete did not rebuild downstream mixer for temporary wire");
	}
	psy_undoredo_undo(&machines->undoredo);
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
		return fail("delete undo did not reconcile downstream mixer state");
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
	psy_undoredo_undo(&machines->undoredo);
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

	psy_audio_song_deallocate(song);
	psy_audio_machinefactory_dispose(&factory);
	psy_audio_plugincatcher_dispose(&catcher);
	psy_audio_dispose();
	puts("phase4-machine-undo: PASS");
	return 0;
}
