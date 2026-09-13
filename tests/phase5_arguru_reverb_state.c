/*
** PSYCLE-LINUX Phase 5 Arguru Reverb state/persistence regression.
**
** Exercises the retained native machine through Psycle's production Plugin,
** PluginCatcher, MachineFactory, preset I/O, and PSY3 save/reload paths.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <machine.h>
#include <machinefactory.h>
#include <machineinfo.h>
#include <machines.h>
#include <player.h>
#include <plugin.h>
#include <plugincatcher.h>
#include <preset.h>
#include <presetio.h>
#include <presets.h>
#include <song.h>
#include <songio.h>
#include <wire.h>

#define REVERB_SLOT 0u
#define PARAM_COUNT 8u
#define CATCHER_NAME "arguru-reverb:0"
#define PRESET_NAME "Phase 5 Arguru Reverb state"

static const intptr_t EXPECTED_VALUES[PARAM_COUNT] = {
	1234, 96, 240, 700, 22050, 192, 96, 6
};

static int fail(const char* message)
{
	fprintf(stderr, "phase5-arguru-reverb-state: FAIL: %s\n", message);
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
		rc = fail("production native-plugin probe could not read Arguru Reverb");
	} else if (info.type != psy_audio_PLUGIN || !info.name ||
			strcmp(info.name, "Arguru Reverb") != 0) {
		rc = fail("production native-plugin probe returned unexpected machine metadata");
	} else {
		machineinfo_catchername(&info, catcher_name);
		if (strcmp(catcher_name, CATCHER_NAME) != 0) {
			rc = fail("Arguru Reverb catcher identity changed");
		} else {
			psy_audio_plugins_add(&catcher->plugins_, &info);
			if (!psy_audio_plugins_at_id_const(&catcher->plugins_, catcher_name)) {
				rc = fail("Arguru Reverb was not registered in PluginCatcher");
			}
		}
	}
	machineinfo_dispose(&info);
	return rc;
}

static psy_audio_Machine* make_reverb(psy_audio_MachineFactory* factory)
{
	return psy_audio_machinefactory_make_machine(factory, psy_audio_PLUGIN,
		CATCHER_NAME, psy_INDEX_INVALID);
}

static int verify_identity(psy_audio_Machine* machine)
{
	const psy_audio_MachineInfo* info;

	if (!machine || psy_audio_machine_type(machine) != psy_audio_PLUGIN) {
		return fail("Arguru Reverb is missing or no longer a Psycle native plugin");
	}
	info = psy_audio_machine_info(machine);
	if (!info || !info->name || strcmp(info->name, "Arguru Reverb") != 0 ||
			!info->shortname || strcmp(info->shortname, "Reverb") != 0 ||
			!info->author || strcmp(info->author, "J. Arguelles") != 0 ||
			!info->modulepath || !strstr(info->modulepath, "arguru-reverb")) {
		return fail("Arguru Reverb identity/module path changed");
	}
	if (psy_audio_machine_num_parameters(machine) != PARAM_COUNT) {
		return fail("Arguru Reverb parameter count changed");
	}
	if (psy_audio_machine_data_size(machine) != 0) {
		return fail("Arguru Reverb unexpectedly acquired opaque plugin state");
	}
	return 0;
}

static int set_expected_parameters(psy_audio_Machine* machine)
{
	uintptr_t i;

	for (i = 0; i < PARAM_COUNT; ++i) {
		psy_audio_MachineParam* param;

		param = psy_audio_machine_parameter(machine, i);
		if (!param) {
			return fail("Arguru Reverb parameter surface is incomplete");
		}
		psy_audio_machine_parameter_tweak_scaled(machine, param, EXPECTED_VALUES[i]);
	}
	return 0;
}

static int verify_expected_parameters(psy_audio_Machine* machine)
{
	uintptr_t i;

	if (verify_identity(machine) != 0) {
		return 1;
	}
	for (i = 0; i < PARAM_COUNT; ++i) {
		psy_audio_MachineParam* param;
		intptr_t actual;

		param = psy_audio_machine_parameter(machine, i);
		if (!param) {
			return fail("Arguru Reverb parameter missing during verification");
		}
		actual = psy_audio_machine_parameter_scaled_value(machine, param);
		if (actual != EXPECTED_VALUES[i]) {
			fprintf(stderr,
				"phase5-arguru-reverb-state: FAIL: parameter %lu expected %ld got %ld\n",
				(unsigned long)i, (long)EXPECTED_VALUES[i], (long)actual);
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
	uintptr_t i;
	int status;
	int rc;

	psy_audio_presets_init(&saved);
	psy_audio_presets_init(&loaded);
	captured = psy_audio_preset_alloc_init();
	if (!captured) {
		psy_audio_presets_dispose(&saved);
		psy_audio_presets_dispose(&loaded);
		return fail("could not allocate captured Arguru Reverb preset");
	}
	psy_audio_machine_current_preset(source, captured);
	psy_audio_preset_set_name(captured, PRESET_NAME);
	if (psy_audio_preset_num_parameters(captured) != PARAM_COUNT ||
			captured->datasize != 0 || captured->data != NULL) {
		psy_audio_preset_dispose(captured);
		free(captured);
		psy_audio_presets_dispose(&saved);
		psy_audio_presets_dispose(&loaded);
		return fail("captured Arguru Reverb preset has unexpected state geometry");
	}
	psy_audio_presets_insert(&saved, 0, captured);

	status = psy_audio_presetsio_save(preset_path, &saved);
	if (status != psy_audio_PRESETIO_OK) {
		psy_audio_presets_dispose(&saved);
		psy_audio_presets_dispose(&loaded);
		return fail("Arguru Reverb preset save failed");
	}
	status = psy_audio_presetsio_load(preset_path, &loaded, PARAM_COUNT, 0, "");
	if (status != psy_audio_PRESETIO_OK || psy_audio_presets_size(&loaded) != 1) {
		psy_audio_presets_dispose(&saved);
		psy_audio_presets_dispose(&loaded);
		return fail("Arguru Reverb preset reload failed");
	}
	reloaded = psy_audio_presets_at(&loaded, 0);
	if (!reloaded || strcmp(psy_audio_preset_name(reloaded), PRESET_NAME) != 0 ||
			psy_audio_preset_num_parameters(reloaded) != PARAM_COUNT ||
			reloaded->datasize != 0) {
		psy_audio_presets_dispose(&saved);
		psy_audio_presets_dispose(&loaded);
		return fail("Arguru Reverb preset metadata changed");
	}
	for (i = 0; i < PARAM_COUNT; ++i) {
		if (psy_audio_preset_value(reloaded, i) != EXPECTED_VALUES[i]) {
			psy_audio_presets_dispose(&saved);
			psy_audio_presets_dispose(&loaded);
			return fail("Arguru Reverb preset parameter changed");
		}
	}

	fresh = make_reverb(factory);
	if (!fresh) {
		psy_audio_presets_dispose(&saved);
		psy_audio_presets_dispose(&loaded);
		return fail("could not create fresh Arguru Reverb for preset restore");
	}
	psy_audio_machine_tweak_preset(fresh, reloaded);
	rc = verify_expected_parameters(fresh);
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
		fprintf(stderr, "phase5-arguru-reverb-state: song save failed (%d)\n", status);
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
		fprintf(stderr, "phase5-arguru-reverb-state: song load failed (%d)\n", status);
		psy_audio_song_deallocate(song);
		return NULL;
	}
	return song;
}

int main(int argc, char** argv)
{
	char song_path[4096];
	char preset_path[4096];
	psy_audio_MachineCallback callback;
	psy_audio_PluginCatcher catcher;
	psy_audio_MachineFactory factory;
	psy_audio_Song* song;
	psy_audio_Machine* reverb;
	psy_audio_MachineCallback loaded_callback;
	psy_audio_PluginCatcher loaded_catcher;
	psy_audio_MachineFactory loaded_factory;
	psy_audio_Song* loaded;
	psy_audio_Machine* loaded_reverb;
	int rc;

	if (argc != 3) {
		fprintf(stderr, "usage: %s OUTPUT_DIRECTORY PATH_TO_ARGURU_REVERB_SO\n", argv[0]);
		return 2;
	}
	if (snprintf(song_path, sizeof(song_path), "%s/phase5-arguru-reverb.psy", argv[1]) >=
			(int)sizeof(song_path) ||
			snprintf(preset_path, sizeof(preset_path), "%s/phase5-arguru-reverb.prs", argv[1]) >=
			(int)sizeof(preset_path)) {
		return fail("output path is too long");
	}

	rc = 0;
	psy_audio_init();
	psy_audio_machinecallback_init(&callback);
	psy_audio_plugincatcher_init(&catcher, NULL);
	psy_audio_machinefactory_init(&factory, &callback, &catcher, NULL);
	psy_audio_machinefactory_createwithoutproxy(&factory);
	if (register_native_plugin(&catcher, argv[2]) != 0) {
		rc = 1;
		goto initial_cleanup;
	}

	song = psy_audio_song_alloc_init(&factory);
	if (!song) {
		rc = fail("could not allocate Arguru Reverb song");
		goto initial_cleanup;
	}
	psy_audio_machinecallback_set_song(&callback, song);
	reverb = make_reverb(&factory);
	if (!reverb) {
		rc = fail("production MachineFactory could not create Arguru Reverb");
		goto song_cleanup;
	}
	psy_audio_machines_insert(psy_audio_song_machines(song), REVERB_SLOT, reverb);
	psy_audio_machines_connect(psy_audio_song_machines(song),
		psy_audio_wire_make(REVERB_SLOT, psy_audio_MASTER_INDEX));

	if ((rc = verify_identity(reverb)) != 0 ||
			(rc = set_expected_parameters(reverb)) != 0 ||
			(rc = verify_expected_parameters(reverb)) != 0 ||
			(rc = exercise_preset_roundtrip(&factory, reverb, preset_path)) != 0 ||
			(rc = save_song(song, song_path)) != 0) {
		goto song_cleanup;
	}

	psy_audio_machinecallback_init(&loaded_callback);
	psy_audio_plugincatcher_init(&loaded_catcher, NULL);
	if (register_native_plugin(&loaded_catcher, argv[2]) != 0) {
		rc = 1;
		psy_audio_plugincatcher_dispose(&loaded_catcher);
		goto song_cleanup;
	}
	psy_audio_machinefactory_init(&loaded_factory, &loaded_callback,
		&loaded_catcher, NULL);
	psy_audio_machinefactory_createwithoutproxy(&loaded_factory);
	loaded = load_song(&loaded_factory, song_path);
	if (!loaded) {
		rc = fail("PSY3 reload could not reconstruct Arguru Reverb");
	} else {
		psy_audio_machinecallback_set_song(&loaded_callback, loaded);
		loaded_reverb = psy_audio_machines_at(psy_audio_song_machines(loaded),
			REVERB_SLOT);
		if ((rc = verify_expected_parameters(loaded_reverb)) == 0 &&
				!psy_audio_machines_connected(psy_audio_song_machines(loaded),
					psy_audio_wire_make(REVERB_SLOT, psy_audio_MASTER_INDEX))) {
			rc = fail("Arguru Reverb wire to Master did not survive PSY3 reload");
		}
		psy_audio_song_deallocate(loaded);
	}
	psy_audio_machinefactory_dispose(&loaded_factory);
	psy_audio_plugincatcher_dispose(&loaded_catcher);

song_cleanup:
	psy_audio_song_deallocate(song);
initial_cleanup:
	psy_audio_machinefactory_dispose(&factory);
	psy_audio_plugincatcher_dispose(&catcher);
	psy_audio_dispose();

	if (rc != 0) {
		return rc;
	}
	printf("phase5-arguru-reverb-state: PASS\n");
	printf("preset: %s\n", preset_path);
	printf("song: %s\n", song_path);
	printf("state: 8 parameters, 0 opaque bytes\n");
	return 0;
}
