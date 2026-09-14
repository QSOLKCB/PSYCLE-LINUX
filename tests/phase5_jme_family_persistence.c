/*
** PSYCLE-LINUX Phase 5C JME-family production persistence regression.
**
** Exercises all four retained JME native generators through the production
** PluginCatcher / MachineFactory / preset / PSY3 paths.  The older and newer
** Blitz/GameFX identities remain distinct modules and catcher names.
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

#define JME_COUNT 4u
#define MAX_SEEDS 8u

struct SeedValue {
	uintptr_t index;
	intptr_t value;
};

typedef struct JmeSpec {
	const char* label;
	const char* catcher_name;
	const char* expected_name;
	const char* expected_short_name;
	const char* module_fragment;
	uintptr_t slot;
	uintptr_t parameter_count;
	const char* preset_file;
	struct SeedValue seeds[MAX_SEEDS];
	uintptr_t seed_count;
} JmeSpec;

static const JmeSpec SPECS[JME_COUNT] = {
	{
		"Blitz 1.2.1", "blitz12:0", "Blitz 1.2.1", "Blitz", "blitz12",
		0u, 112u, "phase5-jme-blitz12.prs",
		{{1u, 200}, {2u, 12}, {17u, 192}, {20u, 1}, {90u, 200}, {98u, 192}}, 6u
	},
	{
		"Blitz 1.6", "blitzn:0", "Blitz 1.6", "Blitz", "blitzn",
		1u, 112u, "phase5-jme-blitzn.prs",
		{{1u, 200}, {2u, 12}, {17u, 192}, {20u, 1}, {90u, 200}, {98u, 192}}, 6u
	},
	{
		"GameFX 1.3.1", "gamefx13:0", "GameFX ver. 1.3.1", "GameFX", "gamefx13",
		2u, 128u, "phase5-jme-gamefx13.prs",
		{{0u, 200}, {16u, 1}, {32u, 48}, {48u, 1}, {80u, 128}, {96u, 64}}, 6u
	},
	{
		"GameFX 1.6", "gamefxn:0", "GameFX ver. 1.6", "GameFX", "gamefxn",
		3u, 128u, "phase5-jme-gamefxn.prs",
		{{0u, 200}, {16u, 1}, {32u, 48}, {48u, 1}, {80u, 128}, {96u, 64}}, 6u
	}
};

static int fail_spec(const JmeSpec* spec, const char* message)
{
	fprintf(stderr, "phase5-jme-family-state: FAIL [%s]: %s\n", spec->label, message);
	return 1;
}

static int register_native_plugin(psy_audio_PluginCatcher* catcher,
	const JmeSpec* spec, const char* module_path)
{
	psy_audio_MachineInfo info;
	char catcher_name[256];
	int rc = 0;

	machineinfo_init(&info);
	if (!psy_audio_plugin_psycle_test(module_path, catcher->native_root_, &info)) {
		rc = fail_spec(spec, "production native-plugin probe failed");
	} else if (info.type != psy_audio_PLUGIN || !info.name ||
			strcmp(info.name, spec->expected_name) != 0) {
		rc = fail_spec(spec, "production probe returned unexpected identity/type");
	} else {
		machineinfo_catchername(&info, catcher_name);
		if (strcmp(catcher_name, spec->catcher_name) != 0) {
			fprintf(stderr,
				"phase5-jme-family-state: FAIL [%s]: catcher expected %s got %s\n",
				spec->label, spec->catcher_name, catcher_name);
			rc = 1;
		} else {
			psy_audio_plugins_add(&catcher->plugins_, &info);
			if (!psy_audio_plugins_at_id_const(&catcher->plugins_, catcher_name)) {
				rc = fail_spec(spec, "PluginCatcher registration disappeared");
			}
		}
	}
	machineinfo_dispose(&info);
	return rc;
}

static psy_audio_Machine* make_machine(psy_audio_MachineFactory* factory,
	const JmeSpec* spec)
{
	return psy_audio_machinefactory_make_machine(factory, psy_audio_PLUGIN,
		spec->catcher_name, psy_INDEX_INVALID);
}

static int verify_identity(const JmeSpec* spec, psy_audio_Machine* machine)
{
	const psy_audio_MachineInfo* info;

	if (!machine || psy_audio_machine_type(machine) != psy_audio_PLUGIN) {
		return fail_spec(spec, "machine missing or no longer a Psycle native plugin");
	}
	info = psy_audio_machine_info(machine);
	if (!info || !info->name || strcmp(info->name, spec->expected_name) != 0 ||
			!info->shortname || strcmp(info->shortname, spec->expected_short_name) != 0 ||
			!info->author || strcmp(info->author, "jme") != 0 ||
			!info->modulepath || !strstr(info->modulepath, spec->module_fragment)) {
		return fail_spec(spec, "production identity/module path changed");
	}
	if (psy_audio_machine_num_parameters(machine) != spec->parameter_count) {
		return fail_spec(spec, "production parameter count changed");
	}
	if (psy_audio_machine_data_size(machine) != 0) {
		return fail_spec(spec, "unexpected opaque plugin payload appeared");
	}
	return 0;
}

static int seed_parameters(const JmeSpec* spec, psy_audio_Machine* machine)
{
	uintptr_t i;
	for (i = 0; i < spec->seed_count; ++i) {
		psy_audio_MachineParam* param;
		intptr_t minval;
		intptr_t maxval;
		const uintptr_t index = spec->seeds[i].index;
		const intptr_t value = spec->seeds[i].value;
		if (index >= spec->parameter_count) return fail_spec(spec, "seed index out of range");
		param = psy_audio_machine_parameter(machine, index);
		if (!param) return fail_spec(spec, "seed parameter disappeared");
		psy_audio_machine_parameter_range(machine, param, &minval, &maxval);
		if (value < minval || value > maxval) return fail_spec(spec, "seed value outside public range");
		psy_audio_machine_parameter_tweak_scaled(machine, param, value);
		if (psy_audio_machine_parameter_scaled_value(machine, param) != value) {
			return fail_spec(spec, "seed parameter did not retain requested public value");
		}
	}
	return 0;
}

static intptr_t* capture_values(const JmeSpec* spec, psy_audio_Machine* machine)
{
	uintptr_t i;
	intptr_t* values = (intptr_t*)calloc(spec->parameter_count, sizeof(intptr_t));
	if (!values) return NULL;
	for (i = 0; i < spec->parameter_count; ++i) {
		psy_audio_MachineParam* param = psy_audio_machine_parameter(machine, i);
		if (!param) {
			free(values);
			return NULL;
		}
		values[i] = psy_audio_machine_parameter_scaled_value(machine, param);
	}
	return values;
}

static int verify_values(const JmeSpec* spec, psy_audio_Machine* machine,
	const intptr_t* expected)
{
	uintptr_t i;
	if (verify_identity(spec, machine) != 0) return 1;
	for (i = 0; i < spec->parameter_count; ++i) {
		psy_audio_MachineParam* param = psy_audio_machine_parameter(machine, i);
		intptr_t actual;
		if (!param) return fail_spec(spec, "parameter missing during state verification");
		actual = psy_audio_machine_parameter_scaled_value(machine, param);
		if (actual != expected[i]) {
			fprintf(stderr,
				"phase5-jme-family-state: FAIL [%s]: parameter %lu expected %ld got %ld\n",
				spec->label, (unsigned long)i, (long)expected[i], (long)actual);
			return 1;
		}
	}
	return 0;
}

static int preset_roundtrip(psy_audio_MachineFactory* factory,
	const JmeSpec* spec, psy_audio_Machine* source, const intptr_t* expected,
	const char* preset_path)
{
	psy_audio_Presets saved;
	psy_audio_Presets loaded;
	psy_audio_Preset* captured;
	psy_audio_Preset* reloaded;
	psy_audio_Machine* fresh;
	uintptr_t i;
	int status;
	int rc = 0;

	psy_audio_presets_init(&saved);
	psy_audio_presets_init(&loaded);
	captured = psy_audio_preset_alloc_init();
	if (!captured) {
		psy_audio_presets_dispose(&saved);
		psy_audio_presets_dispose(&loaded);
		return fail_spec(spec, "preset allocation failed");
	}
	psy_audio_machine_current_preset(source, captured);
	psy_audio_preset_set_name(captured, spec->label);
	if (psy_audio_preset_num_parameters(captured) != spec->parameter_count ||
			captured->datasize != 0 || captured->data != NULL) {
		psy_audio_preset_dispose(captured);
		free(captured);
		psy_audio_presets_dispose(&saved);
		psy_audio_presets_dispose(&loaded);
		return fail_spec(spec, "captured preset geometry changed");
	}
	psy_audio_presets_insert(&saved, 0, captured);
	status = psy_audio_presetsio_save(preset_path, &saved);
	if (status != psy_audio_PRESETIO_OK) {
		rc = fail_spec(spec, "preset save failed");
		goto cleanup;
	}
	status = psy_audio_presetsio_load(preset_path, &loaded,
		spec->parameter_count, 0, "");
	if (status != psy_audio_PRESETIO_OK || psy_audio_presets_size(&loaded) != 1) {
		rc = fail_spec(spec, "preset reload failed");
		goto cleanup;
	}
	reloaded = psy_audio_presets_at(&loaded, 0);
	if (!reloaded || psy_audio_preset_num_parameters(reloaded) != spec->parameter_count ||
			reloaded->datasize != 0) {
		rc = fail_spec(spec, "reloaded preset geometry changed");
		goto cleanup;
	}
	for (i = 0; i < spec->parameter_count; ++i) {
		if (psy_audio_preset_value(reloaded, i) != expected[i]) {
			rc = fail_spec(spec, "reloaded preset parameter changed");
			goto cleanup;
		}
	}
	fresh = make_machine(factory, spec);
	if (!fresh) {
		rc = fail_spec(spec, "fresh preset-restore machine creation failed");
		goto cleanup;
	}
	psy_audio_machine_tweak_preset(fresh, reloaded);
	rc = verify_values(spec, fresh, expected);
	psy_audio_machine_deallocate(fresh);

cleanup:
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
	return status == PSY_OK ? 0 : 1;
}

static psy_audio_Song* load_song(psy_audio_MachineFactory* factory, const char* path)
{
	psy_audio_Song* song = psy_audio_song_alloc_init(factory);
	psy_audio_SongReader reader;
	int status;
	if (!song) return NULL;
	psy_audio_songreader_init(&reader, song, NULL, FALSE);
	status = psy_audio_songreader_load(&reader, path);
	psy_audio_songreader_dispose(&reader);
	if (status != PSY_OK) {
		psy_audio_song_deallocate(song);
		return NULL;
	}
	return song;
}

int main(int argc, char** argv)
{
	char song_path[4096];
	char preset_paths[JME_COUNT][4096];
	psy_audio_MachineCallback callback;
	psy_audio_PluginCatcher catcher;
	psy_audio_MachineFactory factory;
	psy_audio_Song* song = NULL;
	intptr_t* expected[JME_COUNT] = {NULL, NULL, NULL, NULL};
	psy_audio_MachineCallback loaded_callback;
	psy_audio_PluginCatcher loaded_catcher;
	psy_audio_MachineFactory loaded_factory;
	psy_audio_Song* loaded = NULL;
	uintptr_t i;
	int rc = 0;

	if (argc != 6) {
		fprintf(stderr,
			"usage: %s OUTPUT_DIRECTORY BLITZ12_SO BLITZN_SO GAMEFX13_SO GAMEFXN_SO\n",
			argv[0]);
		return 2;
	}
	if (snprintf(song_path, sizeof(song_path), "%s/phase5-jme-family.psy", argv[1]) >=
			(int)sizeof(song_path)) {
		return 2;
	}
	for (i = 0; i < JME_COUNT; ++i) {
		if (snprintf(preset_paths[i], sizeof(preset_paths[i]), "%s/%s",
				argv[1], SPECS[i].preset_file) >= (int)sizeof(preset_paths[i])) return 2;
	}

	psy_audio_init();
	psy_audio_machinecallback_init(&callback);
	psy_audio_plugincatcher_init(&catcher, NULL);
	psy_audio_machinefactory_init(&factory, &callback, &catcher, NULL);
	psy_audio_machinefactory_createwithoutproxy(&factory);
	for (i = 0; i < JME_COUNT; ++i) {
		if (register_native_plugin(&catcher, &SPECS[i], argv[i + 2]) != 0) {
			rc = 1;
			goto initial_cleanup;
		}
	}

	song = psy_audio_song_alloc_init(&factory);
	if (!song) {
		rc = 1;
		goto initial_cleanup;
	}
	psy_audio_machinecallback_set_song(&callback, song);
	for (i = 0; i < JME_COUNT; ++i) {
		psy_audio_Machine* machine = make_machine(&factory, &SPECS[i]);
		if (!machine || verify_identity(&SPECS[i], machine) != 0 ||
				seed_parameters(&SPECS[i], machine) != 0) {
			rc = 1;
			if (machine) psy_audio_machine_deallocate(machine);
			goto song_cleanup;
		}
		expected[i] = capture_values(&SPECS[i], machine);
		if (!expected[i]) {
			rc = fail_spec(&SPECS[i], "could not capture complete parameter state");
			psy_audio_machine_deallocate(machine);
			goto song_cleanup;
		}
		if (preset_roundtrip(&factory, &SPECS[i], machine, expected[i],
				preset_paths[i]) != 0) {
			rc = 1;
			psy_audio_machine_deallocate(machine);
			goto song_cleanup;
		}
		psy_audio_machines_insert(psy_audio_song_machines(song), SPECS[i].slot, machine);
		psy_audio_machines_connect(psy_audio_song_machines(song),
			psy_audio_wire_make(SPECS[i].slot, psy_audio_MASTER_INDEX));
	}
	if (save_song(song, song_path) != 0) {
		rc = 1;
		goto song_cleanup;
	}

	psy_audio_machinecallback_init(&loaded_callback);
	psy_audio_plugincatcher_init(&loaded_catcher, NULL);
	for (i = 0; i < JME_COUNT; ++i) {
		if (register_native_plugin(&loaded_catcher, &SPECS[i], argv[i + 2]) != 0) {
			rc = 1;
			psy_audio_plugincatcher_dispose(&loaded_catcher);
			goto song_cleanup;
		}
	}
	psy_audio_machinefactory_init(&loaded_factory, &loaded_callback,
		&loaded_catcher, NULL);
	psy_audio_machinefactory_createwithoutproxy(&loaded_factory);
	loaded = load_song(&loaded_factory, song_path);
	if (!loaded) {
		rc = 1;
	} else {
		psy_audio_machinecallback_set_song(&loaded_callback, loaded);
		for (i = 0; i < JME_COUNT && rc == 0; ++i) {
			psy_audio_Machine* machine = psy_audio_machines_at(
				psy_audio_song_machines(loaded), SPECS[i].slot);
			if (verify_values(&SPECS[i], machine, expected[i]) != 0) {
				rc = 1;
				break;
			}
			if (!psy_audio_machines_connected(psy_audio_song_machines(loaded),
					psy_audio_wire_make(SPECS[i].slot, psy_audio_MASTER_INDEX))) {
				rc = fail_spec(&SPECS[i], "wire to Master did not survive PSY3 reload");
				break;
			}
		}
		psy_audio_song_deallocate(loaded);
		loaded = NULL;
	}
	psy_audio_machinefactory_dispose(&loaded_factory);
	psy_audio_plugincatcher_dispose(&loaded_catcher);

song_cleanup:
	if (song) psy_audio_song_deallocate(song);
initial_cleanup:
	for (i = 0; i < JME_COUNT; ++i) free(expected[i]);
	psy_audio_machinefactory_dispose(&factory);
	psy_audio_plugincatcher_dispose(&catcher);
	psy_audio_dispose();
	if (rc != 0) return rc;

	printf("phase5-jme-family-state: PASS\n");
	printf("song: %s\n", song_path);
	printf("machines: 4 distinct JME identities\n");
	printf("state: 112 + 112 + 128 + 128 public parameters, 0 opaque bytes\n");
	printf("topology: all four JME generators -> Master\n");
	return 0;
}
