/*
** Regression for undo/redo ownership of built-in Psycle machines.
**
** Internal machines inherit the base Machine clone implementation, which
** returns NULL. Undo/redo must therefore preserve the actual machine object
** rather than depending on clone support.
*/

#include <stdio.h>

#include <machinefactory.h>
#include <machines.h>
#include <player.h>
#include <plugincatcher.h>
#include <song.h>
#include <undoredo.h>
#include <wire.h>

#define SAMPLER_SLOT 0
#define MIXER_SLOT 1

static int fail(const char* message)
{
	fprintf(stderr, "phase4-machine-undo: FAIL: %s\n", message);
	return 1;
}

static int topology_is_routed(psy_audio_Machines* machines)
{
	return psy_audio_machines_at(machines, MIXER_SLOT) &&
		psy_audio_machines_connected(machines,
			psy_audio_wire_make(SAMPLER_SLOT, MIXER_SLOT)) &&
		psy_audio_machines_connected(machines,
			psy_audio_wire_make(MIXER_SLOT, psy_audio_MASTER_INDEX)) &&
		!psy_audio_machines_connected(machines,
			psy_audio_wire_make(SAMPLER_SLOT, psy_audio_MASTER_INDEX));
}

static int topology_is_rewired(psy_audio_Machines* machines)
{
	return !psy_audio_machines_at(machines, MIXER_SLOT) &&
		psy_audio_machines_connected(machines,
			psy_audio_wire_make(SAMPLER_SLOT, psy_audio_MASTER_INDEX));
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
	if (!sampler || !mixer) {
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

	psy_audio_machines_connect(machines,
		psy_audio_wire_make(SAMPLER_SLOT, MIXER_SLOT));
	psy_audio_machines_connect(machines,
		psy_audio_wire_make(MIXER_SLOT, psy_audio_MASTER_INDEX));
	if (!topology_is_routed(machines)) {
		psy_audio_song_deallocate(song);
		return fail("initial routed topology is invalid");
	}

	psy_audio_machines_remove(machines, MIXER_SLOT, TRUE);
	if (!topology_is_rewired(machines)) {
		psy_audio_song_deallocate(song);
		return fail("delete did not rewire sampler to master");
	}
	psy_undoredo_undo(&machines->undoredo);
	if (!topology_is_routed(machines)) {
		psy_audio_song_deallocate(song);
		return fail("delete undo did not restore mixer and connections");
	}
	psy_undoredo_redo(&machines->undoredo);
	if (!topology_is_rewired(machines)) {
		psy_audio_song_deallocate(song);
		return fail("delete redo did not reapply rewired topology");
	}
	psy_undoredo_undo(&machines->undoredo);
	if (!topology_is_routed(machines)) {
		psy_audio_song_deallocate(song);
		return fail("second delete undo did not restore topology");
	}

	psy_audio_song_deallocate(song);
	psy_audio_machinefactory_dispose(&factory);
	psy_audio_plugincatcher_dispose(&catcher);
	psy_audio_dispose();
	puts("phase4-machine-undo: PASS");
	return 0;
}
