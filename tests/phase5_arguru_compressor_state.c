/*
** PSYCLE-LINUX Phase 5 Arguru Compressor state/persistence regression.
**
** Exercises the retained native machine through Psycle's production Plugin,
** MachineFactory, preset I/O, and PSY3 save/reload paths.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <machine.h>
#include <machinefactory.h>
#include <machineinfo.h>
#include <machines.h>
#include <plugin.h>
#include <plugincatcher.h>
#include <preset.h>
#include <presetio.h>
#include <presets.h>
#include <song.h>
#include <songio.h>
#include <wire.h>

#define MACHINE_SLOT 0
#define PARAM_COUNT 6u

static const intptr_t EXPECTED_VALUES[PARAM_COUNT] = {
	32, 80, 4, 10, 50, 1
};

static int fail(const char* message)
{
	fprintf(stderr, "phase5-arguru-compressor-state: FAIL: %s\n", message);
	return 1;
}

static int register_native(psy_audio_PluginCatcher* catcher, const char* path)
{
	psy_audio_MachineInfo info;
	int ok;

	machineinfo_init(&info);
	ok = psy_audio_plugin_psycle_test(path, catcher->native_root_, &info);
	if (ok) {
		psy_audio_plugins_add(&catcher->plugins_, &info);
	}
	machineinfo_dispose(&info);
	return ok ? 0 : fail("PluginCatcher could not identify Arguru Compressor");
}

static int verify_identity(psy_audio_Machine* machine)
{
	const psy_audio_MachineInfo* info;

	if (!machine || psy_audio_machine_type(machine) != psy_audio_PLUGIN) {
		return fail("Arguru Compressor did not reload as a Psycle native plugin");
	}
	info = psy_audio_machine_info(machine);
	if (!info || !info->name || strcmp(info->name, "Arguru Compressor") != 0 ||
			!info->shortname || strcmp(info->shortname, "Compressor") != 0 ||
			!info->author || strcmp(info->author, "J. Arguelles & psycledelics") != 0) {
		return fail("Arguru Compressor identity changed across production wrapper");
	}
	if (psy_audio_machine_num_parameters(machine) != PARAM_COUNT) {
		return fail("Arguru Compressor parameter count changed");
	}
	if (psy_audio_machine_data_size(machine) != 0) {
		return fail("Arguru Compressor unexpectedly acquired opaque plugin state");
	}
	return 0;
}

static int set_expected_parameters(psy_audio_Machine* machine)
{
	uintptr_t i;

	for (i = 0; i < PARAM_COUNT; ++i) {
		psy_audio_MachineParam* param = psy_audio_machine_parameter(machine, i);
		if (!param) {
			return fail("Arguru Compressor parameter surface is incomplete");
		}
		psy_audio_machine_parameter_tweak_scaled(machine, param, EXPECTED_VALUES[i]);
	}
	return 0;
}

static int verify_expected_parameters(psy_audio_Machine* machine)
{
	uintptr_t i;

	for (i = 0; i < PARAM_COUNT; ++i) {
		psy_audio_MachineParam* param = psy_audio_machine_parameter(machine, i);
		intptr_t actual;
		if (!param) {
			return fail("Arguru Compressor parameter missing during verification");
		}
		actual = psy_audio_machine_parameter_scaled_value(machine, param);
		if (actual != EXPECTED_VALUES[i]) {
			fprintf(stderr,
				"phase5-arguru-compressor-state: FAIL: parameter %lu expected %ld got %ld\n",
				(unsigned long)i, (long)EXPECTED_VALUES[i], (long)actual);
			return 1;
		}
	}
	return 0;
}

static int save_song(psy_audio_Song* song, const char* path)
{
	psy_audio_SongFile songfile;
	int status;

	psy_audio_songfile_init_song(&songfile, song);
	status = psy_audio_songfile_save(&songfile, path);
	psy_audio_songfile_dispose(&songfile);
	if (status != PSY_OK) {
		fprintf(stderr, "phase5-arguru-compressor-state: song save failed (%d)\n", status);
		return 1;
	}
	return 0;
}

static psy_audio_Song* load_song(psy_audio_MachineFactory* factory, const char* path)
{
	psy_audio_Song* song;
	psy_audio_SongReader reader;
	int status;

	song = psy_audio_song_alloc_init(factory);
	if (!song) {
		return NULL;
	}
	psy_audio_songreader_init(&reader, song, NULL, FALSE);
	status = psy_audio_songreader_load(&reader, path);
	psy_audio_songreader_dispose(&reader);
	if (status != PSY_OK) {
		fprintf(stderr, "phase5-arguru-compressor-state: song load failed (%d)\n", status);
		psy_audio_song_deallocate(song);
		return NULL;
	}
	return song;
}

static int exercise_preset_roundtrip(psy_audio_Machine* machine, const char* path)
{
	psy_audio_Presets saved;
	psy_audio_Presets loaded;
	psy_audio_Preset* preset;
	psy_audio_Preset* reloaded;
	uintptr_t i;
	int status;
	int rc = 0;

	psy_audio_presets_init(&saved);
	psy_audio_presets_init(&loaded);
	preset = psy_audio_preset_alloc_init();
	if (!preset) {
		psy_audio_presets_dispose(&saved);
		psy_audio_presets_dispose(&loaded);
		return fail("could not allocate Arguru Compressor preset");
	}
	psy_audio_machine_current_preset(machine, preset);
	psy_audio_preset_set_name(preset, "Phase 5 Arguru state");
	psy_audio_presets_insert(&saved, 0, preset);

	status = psy_audio_presetsio_save(path, &saved);
	if (status != psy_audio_PRESETIO_OK) {
		rc = fail("Arguru Compressor preset save failed");
		goto done;
	}
	status = psy_audio_presetsio_load(path, &loaded, PARAM_COUNT, 0, "");
	if (status != psy_audio_PRESETIO_OK || psy_audio_presets_size(&loaded) != 1) {
		rc = fail("Arguru Compressor preset reload failed");
		goto done;
	}
	reloaded = psy_audio_presets_at(&loaded, 0);
	if (!reloaded || strcmp(psy_audio_preset_name(reloaded), "Phase 5 Arguru state") != 0 ||
			psy_audio_preset_num_parameters(reloaded) != PARAM_COUNT) {
		rc = fail("Arguru Compressor preset metadata changed");
		goto done;
	}
	for (i = 0; i < PARAM_COUNT; ++i) {
		if (psy_audio_preset_value(reloaded, i) != EXPECTED_VALUES[i]) {
			rc = fail("Arguru Compressor preset parameter changed");
			goto done;
		}
	}

	/* Reset away from the expected state, then prove the reloaded preset is
	** sufficient to restore the machine through the production preset path. */
	for (i = 0; i < PARAM_COUNT; ++i) {
		psy_audio_machine_parameter_reset(machine,
			psy_audio_machine_parameter(machine, i));
	}
	psy_audio_machine_tweak_preset(machine, reloaded);
	rc = verify_expected_parameters(machine);

done:
	psy_audio_presets_dispose(&loaded);
	psy_audio_presets_dispose(&saved);
	return rc;
}

int main(int argc, char** argv)
{
	char song_path[4096];
	char preset_path[4096];
	psy_audio_MachineCallback callback;
	psy_audio_PluginCatcher catcher;
	psy_audio_MachineFactory factory;
	psy_audio_Song* song;
	psy_audio_Machine* machine;
	psy_audio_MachineCallback loaded_callback;
	psy_audio_PluginCatcher loaded_catcher;
	psy_audio_MachineFactory loaded_factory;
	psy_audio_Song* loaded;
	psy_audio_Machine* loaded_machine;
	int rc = 0;

	if (argc != 3) {
		fprintf(stderr, "usage: %s OUTPUT_DIRECTORY PATH_TO_ARGURU_COMPRESSOR_SO\n", argv[0]);
		return 2;
	}
	if (snprintf(song_path, sizeof(song_path), "%s/phase5-arguru-compressor.psy", argv[1]) >=
			(int)sizeof(song_path) ||
			snprintf(preset_path, sizeof(preset_path), "%s/phase5-arguru-compressor.prs", argv[1]) >=
			(int)sizeof(preset_path)) {
		return fail("output path is too long");
	}

	psy_audio_init();
	psy_audio_machinecallback_init(&callback);
	psy_audio_plugincatcher_init(&catcher, NULL);
	if (register_native(&catcher, argv[2]) != 0) {
		rc = 1;
		goto initial_cleanup_no_factory;
	}
	psy_audio_machinefactory_init(&factory, &callback, &catcher, NULL);
	song = psy_audio_song_alloc_init(&factory);
	if (!song) {
		rc = fail("could not allocate Arguru Compressor song");
		goto initial_cleanup;
	}
	psy_audio_machinecallback_set_song(&callback, song);
	machine = psy_audio_machinefactory_make_machine_from_path(&factory,
		psy_audio_PLUGIN, argv[2], 0, psy_INDEX_INVALID);
	if (!machine) {
		rc = fail("MachineFactory could not instantiate Arguru Compressor");
		goto song_cleanup;
	}
	psy_audio_machines_insert(psy_audio_song_machines(song), MACHINE_SLOT, machine);
	psy_audio_machines_connect(psy_audio_song_machines(song),
		psy_audio_wire_make(MACHINE_SLOT, psy_audio_MASTER_INDEX));
	if ((rc = verify_identity(machine)) != 0 ||
			(rc = set_expected_parameters(machine)) != 0 ||
			(rc = verify_expected_parameters(machine)) != 0 ||
			(rc = exercise_preset_roundtrip(machine, preset_path)) != 0 ||
			(rc = save_song(song, song_path)) != 0) {
		goto song_cleanup;
	}

	psy_audio_machinecallback_init(&loaded_callback);
	psy_audio_plugincatcher_init(&loaded_catcher, NULL);
	if (register_native(&loaded_catcher, argv[2]) != 0) {
		rc = 1;
		psy_audio_plugincatcher_dispose(&loaded_catcher);
		goto song_cleanup;
	}
	psy_audio_machinefactory_init(&loaded_factory, &loaded_callback, &loaded_catcher, NULL);
	loaded = load_song(&loaded_factory, song_path);
	if (!loaded) {
		rc = fail("PSY3 reload could not reconstruct Arguru Compressor");
	} else {
		psy_audio_machinecallback_set_song(&loaded_callback, loaded);
		loaded_machine = psy_audio_machines_at(psy_audio_song_machines(loaded), MACHINE_SLOT);
		if ((rc = verify_identity(loaded_machine)) == 0) {
			rc = verify_expected_parameters(loaded_machine);
		}
		if (rc == 0 && !psy_audio_machines_connected(psy_audio_song_machines(loaded),
				psy_audio_wire_make(MACHINE_SLOT, psy_audio_MASTER_INDEX))) {
			rc = fail("Arguru Compressor wire to Master did not survive PSY3 reload");
		}
		psy_audio_song_deallocate(loaded);
	}
	psy_audio_machinefactory_dispose(&loaded_factory);
	psy_audio_plugincatcher_dispose(&loaded_catcher);

song_cleanup:
	psy_audio_song_deallocate(song);
initial_cleanup:
	psy_audio_machinefactory_dispose(&factory);
initial_cleanup_no_factory:
	psy_audio_plugincatcher_dispose(&catcher);
	psy_audio_dispose();

	if (rc != 0) {
		return rc;
	}
	printf("phase5-arguru-compressor-state: PASS\n");
	printf("preset: %s\n", preset_path);
	printf("song: %s\n", song_path);
	printf("parameters: preset + PSY3 roundtrip preserved all six values\n");
	return 0;
}
