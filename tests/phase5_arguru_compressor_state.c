/*
** PSYCLE-LINUX Phase 5 Arguru Compressor state-preservation regression.
**
** Exercises the retained native machine through Psycle's production plugin
** wrapper and song I/O rather than treating the shared object as a standalone
** library. The fixture is project-authored at test time.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <machine.h>
#include <machinefactory.h>
#include <machineinfo.h>
#include <player.h>
#include <plugin.h>
#include <plugincatcher.h>
#include <preset.h>
#include <presetio.h>
#include <presets.h>
#include <song.h>
#include <songio.h>
#include <wire.h>

#define COMPRESSOR_SLOT 0u
#define PARAM_COUNT 6u
#define PRESET_NAME "Phase 5 Arguru Compressor state"

static const intptr_t EXPECTED_VALUES[PARAM_COUNT] = {
	37, 22, 5, 17, 81, 1
};

static int fail(const char* message)
{
	fprintf(stderr, "phase5-arguru-compressor-state: FAIL: %s\n", message);
	return 1;
}

static int register_native_plugin(psy_audio_PluginCatcher* catcher,
	const char* module_path)
{
	psy_audio_MachineInfo info;
	char catcher_name[256];
	int rc;

	machineinfo_init(&info);
	rc = 0;
	if (!psy_audio_plugin_psycle_test(module_path, catcher->native_root_, &info)) {
		rc = fail("production native-plugin probe could not read Arguru Compressor");
	} else if (info.type != psy_audio_PLUGIN || !info.name ||
			strcmp(info.name, "Arguru Compressor") != 0) {
		rc = fail("production native-plugin probe returned unexpected machine metadata");
	} else {
		machineinfo_catchername(&info, catcher_name);
		if (strcmp(catcher_name, "arguru-compressor:0") != 0) {
			rc = fail("Arguru Compressor catcher identity changed");
		} else {
			psy_audio_plugins_add(&catcher->plugins_, &info);
			if (!psy_audio_plugins_at_id_const(&catcher->plugins_, catcher_name)) {
				rc = fail("Arguru Compressor was not registered in PluginCatcher");
			}
		}
	}
	machineinfo_dispose(&info);
	return rc;
}

static psy_audio_Machine* make_compressor(psy_audio_MachineFactory* factory)
{
	return psy_audio_machinefactory_make_machine(factory, psy_audio_PLUGIN,
		"arguru-compressor:0", psy_INDEX_INVALID);
}

static int tweak_values(psy_audio_Machine* machine,
	const intptr_t values[PARAM_COUNT])
{
	uintptr_t i;

	if (!machine || psy_audio_machine_type(machine) != psy_audio_PLUGIN ||
			psy_audio_machine_num_parameters(machine) != PARAM_COUNT) {
		return fail("production wrapper did not expose the expected native machine");
	}
	for (i = 0; i < PARAM_COUNT; ++i) {
		psy_audio_MachineParam* param;

		param = psy_audio_machine_parameter(machine, i);
		if (!param) {
			return fail("Arguru Compressor parameter wrapper is missing");
		}
		psy_audio_machine_parameter_tweak_scaled(machine, param, values[i]);
	}
	return 0;
}

static int verify_values(psy_audio_Machine* machine,
	const intptr_t values[PARAM_COUNT])
{
	uintptr_t i;
	const psy_audio_MachineInfo* info;

	if (!machine || psy_audio_machine_type(machine) != psy_audio_PLUGIN) {
		return fail("Arguru Compressor is missing or no longer a native plugin");
	}
	info = psy_audio_machine_info(machine);
	if (!info || !info->name || strcmp(info->name, "Arguru Compressor") != 0 ||
			!info->modulepath || !strstr(info->modulepath, "arguru-compressor")) {
		return fail("Arguru Compressor identity/module path did not survive");
	}
	if (psy_audio_machine_num_parameters(machine) != PARAM_COUNT) {
		return fail("Arguru Compressor parameter count changed");
	}
	if (psy_audio_machine_data_size(machine) != 0) {
		return fail("Arguru Compressor unexpectedly gained opaque plugin state");
	}
	for (i = 0; i < PARAM_COUNT; ++i) {
		psy_audio_MachineParam* param;
		intptr_t actual;

		param = psy_audio_machine_parameter(machine, i);
		if (!param) {
			return fail("Arguru Compressor parameter disappeared");
		}
		actual = psy_audio_machine_parameter_scaled_value(machine, param);
		if (actual != values[i]) {
			fprintf(stderr,
				"phase5-arguru-compressor-state: FAIL: parameter %lu expected %ld got %ld\n",
				(unsigned long)i, (long)values[i], (long)actual);
			return 1;
		}
	}
	return 0;
}

static int exercise_preset_roundtrip(psy_audio_MachineFactory* factory,
	psy_audio_Machine* source, const char* preset_path)
{
	psy_audio_Presets saved;
	psy_audio_Presets loaded;
	psy_audio_Preset* captured;
	psy_audio_Preset* reloaded;
	psy_audio_Machine* fresh;
	int status;
	int rc;

	psy_audio_presets_init(&saved);
	psy_audio_presets_init(&loaded);
	captured = psy_audio_preset_alloc_init();
	if (!captured) {
		psy_audio_presets_dispose(&saved);
		psy_audio_presets_dispose(&loaded);
		return fail("could not allocate captured Arguru preset");
	}
	psy_audio_machine_current_preset(source, captured);
	psy_audio_preset_set_name(captured, PRESET_NAME);
	if (psy_audio_preset_num_parameters(captured) != PARAM_COUNT ||
			captured->datasize != 0 || captured->data != NULL) {
		psy_audio_preset_dispose(captured);
		free(captured);
		psy_audio_presets_dispose(&saved);
		psy_audio_presets_dispose(&loaded);
		return fail("captured Arguru preset has unexpected state geometry");
	}
	psy_audio_presets_insert(&saved, 0, captured);

	status = psy_audio_presetsio_save(preset_path, &saved);
	if (status != psy_audio_PRESETIO_OK) {
		fprintf(stderr,
			"phase5-arguru-compressor-state: preset save failed: %s (%d)\n",
			psy_audio_presetsio_statusstr(status), status);
		psy_audio_presets_dispose(&saved);
		psy_audio_presets_dispose(&loaded);
		return 1;
	}
	status = psy_audio_presetsio_load(preset_path, &loaded, PARAM_COUNT, 0, "");
	if (status != psy_audio_PRESETIO_OK || psy_audio_presets_size(&loaded) != 1) {
		fprintf(stderr,
			"phase5-arguru-compressor-state: preset load failed: %s (%d)\n",
			psy_audio_presetsio_statusstr(status), status);
		psy_audio_presets_dispose(&saved);
		psy_audio_presets_dispose(&loaded);
		return 1;
	}
	reloaded = psy_audio_presets_at(&loaded, 0);
	if (!reloaded || strcmp(psy_audio_preset_name(reloaded), PRESET_NAME) != 0 ||
			psy_audio_preset_num_parameters(reloaded) != PARAM_COUNT ||
			reloaded->datasize != 0) {
		psy_audio_presets_dispose(&saved);
		psy_audio_presets_dispose(&loaded);
		return fail("Arguru preset metadata changed across file save/load");
	}

	fresh = make_compressor(factory);
	if (!fresh) {
		psy_audio_presets_dispose(&saved);
		psy_audio_presets_dispose(&loaded);
		return fail("could not create fresh Arguru Compressor for preset restore");
	}
	psy_audio_machine_tweak_preset(fresh, reloaded);
	rc = verify_values(fresh, EXPECTED_VALUES);
	psy_audio_machine_deallocate(fresh);
	psy_audio_presets_dispose(&saved);
	psy_audio_presets_dispose(&loaded);
	return rc;
}

static int save_song(psy_audio_Song* song, const char* path)
{
	psy_audio_SongFile songfile;
	int status;

	psy_audio_songfile_init_song(&songfile, song);
	status = psy_audio_songfile_save(&songfile, path);
	psy_audio_songfile_dispose(&songfile);
	if (status != PSY_OK) {
		fprintf(stderr,
			"phase5-arguru-compressor-state: song save failed for %s (%d)\n",
			path, status);
		return 1;
	}
	return 0;
}

static psy_audio_Song* load_song(psy_audio_MachineFactory* factory,
	const char* path)
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
		fprintf(stderr,
			"phase5-arguru-compressor-state: song load failed for %s (%d)\n",
			path, status);
		psy_audio_song_deallocate(song);
		return NULL;
	}
	return song;
}

static int verify_song(psy_audio_Song* song)
{
	psy_audio_Machine* machine;

	machine = psy_audio_machines_at(psy_audio_song_machines(song), COMPRESSOR_SLOT);
	if (verify_values(machine, EXPECTED_VALUES) != 0) {
		return 1;
	}
	if (!psy_audio_machines_connected(psy_audio_song_machines(song),
			psy_audio_wire_make(COMPRESSOR_SLOT, psy_audio_MASTER_INDEX))) {
		return fail("Arguru Compressor-to-Master wire did not survive song roundtrip");
	}
	return 0;
}

int main(int argc, char** argv)
{
	char preset_path[4096];
	char first_song_path[4096];
	char second_song_path[4096];
	psy_audio_MachineCallback callback;
	psy_audio_PluginCatcher catcher;
	psy_audio_MachineFactory factory;
	psy_audio_Song* song;
	psy_audio_Song* loaded;
	psy_audio_Song* reloaded;
	psy_audio_Machine* compressor;
	int rc;

	if (argc != 3) {
		fprintf(stderr,
			"usage: %s OUTPUT_DIRECTORY PATH_TO_ARGURU_COMPRESSOR_SO\n", argv[0]);
		return 2;
	}
	if (snprintf(preset_path, sizeof(preset_path),
			"%s/arguru-compressor-state.prs", argv[1]) >= (int)sizeof(preset_path) ||
			snprintf(first_song_path, sizeof(first_song_path),
				"%s/arguru-compressor-first.psy", argv[1]) >= (int)sizeof(first_song_path) ||
			snprintf(second_song_path, sizeof(second_song_path),
				"%s/arguru-compressor-roundtrip.psy", argv[1]) >= (int)sizeof(second_song_path)) {
		return fail("output path is too long");
	}

	psy_audio_init();
	psy_audio_machinecallback_init(&callback);
	psy_audio_plugincatcher_init(&catcher, NULL);
	psy_audio_machinefactory_init(&factory, &callback, &catcher, NULL);
	psy_audio_machinefactory_createwithoutproxy(&factory);

	if (register_native_plugin(&catcher, argv[2]) != 0) {
		psy_audio_machinefactory_dispose(&factory);
		psy_audio_plugincatcher_dispose(&catcher);
		psy_audio_dispose();
		return 1;
	}

	song = psy_audio_song_alloc_init(&factory);
	if (!song) {
		psy_audio_machinefactory_dispose(&factory);
		psy_audio_plugincatcher_dispose(&catcher);
		psy_audio_dispose();
		return fail("could not allocate Arguru preservation song");
	}
	psy_audio_machinecallback_set_song(&callback, song);
	compressor = make_compressor(&factory);
	if (!compressor) {
		psy_audio_song_deallocate(song);
		psy_audio_machinefactory_dispose(&factory);
		psy_audio_plugincatcher_dispose(&catcher);
		psy_audio_dispose();
		return fail("production MachineFactory could not create Arguru Compressor");
	}
	psy_audio_machines_insert(psy_audio_song_machines(song), COMPRESSOR_SLOT,
		compressor);
	psy_audio_machines_connect(psy_audio_song_machines(song),
		psy_audio_wire_make(COMPRESSOR_SLOT, psy_audio_MASTER_INDEX));

	if (tweak_values(compressor, EXPECTED_VALUES) != 0 ||
			verify_values(compressor, EXPECTED_VALUES) != 0 ||
			exercise_preset_roundtrip(&factory, compressor, preset_path) != 0 ||
			save_song(song, first_song_path) != 0) {
		psy_audio_song_deallocate(song);
		psy_audio_machinefactory_dispose(&factory);
		psy_audio_plugincatcher_dispose(&catcher);
		psy_audio_dispose();
		return 1;
	}

	loaded = load_song(&factory, first_song_path);
	if (!loaded || verify_song(loaded) != 0 ||
			save_song(loaded, second_song_path) != 0) {
		if (loaded) {
			psy_audio_song_deallocate(loaded);
		}
		psy_audio_song_deallocate(song);
		psy_audio_machinefactory_dispose(&factory);
		psy_audio_plugincatcher_dispose(&catcher);
		psy_audio_dispose();
		return 1;
	}
	reloaded = load_song(&factory, second_song_path);
	if (!reloaded) {
		psy_audio_song_deallocate(loaded);
		psy_audio_song_deallocate(song);
		psy_audio_machinefactory_dispose(&factory);
		psy_audio_plugincatcher_dispose(&catcher);
		psy_audio_dispose();
		return 1;
	}
	rc = verify_song(reloaded);

	psy_audio_song_deallocate(reloaded);
	psy_audio_song_deallocate(loaded);
	psy_audio_song_deallocate(song);
	psy_audio_machinefactory_dispose(&factory);
	psy_audio_plugincatcher_dispose(&catcher);
	psy_audio_dispose();

	if (rc != 0) {
		return rc;
	}
	printf("phase5-arguru-compressor-state: PASS\n");
	printf("preset: %s\n", preset_path);
	printf("song: %s\n", first_song_path);
	printf("roundtrip: %s\n", second_song_path);
	printf("state: 6 parameters, 0 opaque bytes\n");
	return 0;
}
