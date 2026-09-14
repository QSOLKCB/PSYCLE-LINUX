/*
** PSYCLE-LINUX Phase 5C Zephod SuperFM production persistence regression.
**
** Exercises the retained Zephod/Arguru generator through the production
** PluginCatcher / MachineFactory / preset / PSY3 paths.
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

#define PARAMETER_COUNT 20u
#define SLOT 0u

struct SeedValue {
	uintptr_t index;
	intptr_t value;
};

static const struct SeedValue SEEDS[] = {
	{0u, 128},
	{1u, 1024},
	{2u, 2000},
	{3u, 200},
	{4u, 3000},
	{5u, 64},
	{6u, 2048},
	{7u, 4096},
	{8u, 128},
	{9u, 5000},
	{10u, 350},
	{11u, 250},
	{12u, 450},
	{13u, 128},
	{14u, -64},
	{15u, 64},
	{16u, 2},
	{17u, 1},
	{18u, 3},
	{19u, 64}
};

static int fail(const char* message)
{
	fprintf(stderr, "phase5-zephod-superfm-state: FAIL: %s\n", message);
	return 1;
}

static int register_native_plugin(psy_audio_PluginCatcher* catcher,
	const char* module_path)
{
	psy_audio_MachineInfo info;
	char catcher_name[256];
	int rc = 0;

	machineinfo_init(&info);
	if (!psy_audio_plugin_psycle_test(module_path, catcher->native_root_, &info)) {
		rc = fail("production native-plugin probe failed");
	} else if (info.type != psy_audio_PLUGIN || !info.name ||
			strcmp(info.name, "Zephod SuperFM (Arguru Remix)") != 0 ||
			!info.shortname || strcmp(info.shortname, "SuperFM") != 0 ||
			!info.author || strcmp(info.author, "Zephod / Arguru") != 0) {
		rc = fail("production probe returned unexpected identity/type");
	} else {
		machineinfo_catchername(&info, catcher_name);
		if (strcmp(catcher_name, "zephod-superfm:0") != 0) {
			fprintf(stderr,
				"phase5-zephod-superfm-state: FAIL: catcher expected zephod-superfm:0 got %s\n",
				catcher_name);
			rc = 1;
		} else {
			psy_audio_plugins_add(&catcher->plugins_, &info);
			if (!psy_audio_plugins_at_id_const(&catcher->plugins_, catcher_name)) {
				rc = fail("PluginCatcher registration disappeared");
			}
		}
	}
	machineinfo_dispose(&info);
	return rc;
}

static psy_audio_Machine* make_machine(psy_audio_MachineFactory* factory)
{
	return psy_audio_machinefactory_make_machine(factory, psy_audio_PLUGIN,
		"zephod-superfm:0", psy_INDEX_INVALID);
}

static int verify_identity(psy_audio_Machine* machine)
{
	const psy_audio_MachineInfo* info;
	if (!machine || psy_audio_machine_type(machine) != psy_audio_PLUGIN) {
		return fail("machine missing or no longer a Psycle native plugin");
	}
	info = psy_audio_machine_info(machine);
	if (!info || !info->name || strcmp(info->name, "Zephod SuperFM (Arguru Remix)") != 0 ||
			!info->shortname || strcmp(info->shortname, "SuperFM") != 0 ||
			!info->author || strcmp(info->author, "Zephod / Arguru") != 0 ||
			!info->modulepath || !strstr(info->modulepath, "zephod-superfm")) {
		return fail("production identity/module path changed");
	}
	if (psy_audio_machine_num_parameters(machine) != PARAMETER_COUNT) {
		return fail("production parameter count changed");
	}
	if (psy_audio_machine_data_size(machine) != 0) {
		return fail("unexpected opaque plugin payload appeared");
	}
	return 0;
}

static int seed_parameters(psy_audio_Machine* machine)
{
	uintptr_t i;
	if (sizeof(SEEDS) / sizeof(SEEDS[0]) != PARAMETER_COUNT) {
		return fail("seed table no longer covers every public parameter");
	}
	for (i = 0; i < PARAMETER_COUNT; ++i) {
		psy_audio_MachineParam* param;
		intptr_t minval;
		intptr_t maxval;
		const uintptr_t index = SEEDS[i].index;
		const intptr_t value = SEEDS[i].value;
		if (index != i) return fail("seed table parameter ordering changed");
		param = psy_audio_machine_parameter(machine, index);
		if (!param) return fail("seed parameter disappeared");
		psy_audio_machine_parameter_range(machine, param, &minval, &maxval);
		if (value < minval || value > maxval) return fail("seed value outside public range");
		psy_audio_machine_parameter_tweak_scaled(machine, param, value);
		if (psy_audio_machine_parameter_scaled_value(machine, param) != value) {
			return fail("seed parameter did not retain requested public value");
		}
	}
	return 0;
}

static intptr_t* capture_values(psy_audio_Machine* machine)
{
	uintptr_t i;
	intptr_t* values = (intptr_t*)calloc(PARAMETER_COUNT, sizeof(intptr_t));
	if (!values) return NULL;
	for (i = 0; i < PARAMETER_COUNT; ++i) {
		psy_audio_MachineParam* param = psy_audio_machine_parameter(machine, i);
		if (!param) {
			free(values);
			return NULL;
		}
		values[i] = psy_audio_machine_parameter_scaled_value(machine, param);
	}
	return values;
}

static int verify_values(psy_audio_Machine* machine, const intptr_t* expected)
{
	uintptr_t i;
	if (verify_identity(machine) != 0) return 1;
	for (i = 0; i < PARAMETER_COUNT; ++i) {
		psy_audio_MachineParam* param = psy_audio_machine_parameter(machine, i);
		intptr_t actual;
		if (!param) return fail("parameter missing during state verification");
		actual = psy_audio_machine_parameter_scaled_value(machine, param);
		if (actual != expected[i]) {
			fprintf(stderr,
				"phase5-zephod-superfm-state: FAIL: parameter %lu expected %ld got %ld\n",
				(unsigned long)i, (long)expected[i], (long)actual);
			return 1;
		}
	}
	return 0;
}

static int preset_roundtrip(psy_audio_MachineFactory* factory,
	psy_audio_Machine* source, const intptr_t* expected, const char* preset_path)
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
		return fail("preset allocation failed");
	}
	psy_audio_machine_current_preset(source, captured);
	psy_audio_preset_set_name(captured, "Zephod SuperFM preservation");
	if (psy_audio_preset_num_parameters(captured) != PARAMETER_COUNT ||
			captured->datasize != 0 || captured->data != NULL) {
		psy_audio_preset_dispose(captured);
		free(captured);
		psy_audio_presets_dispose(&saved);
		psy_audio_presets_dispose(&loaded);
		return fail("captured preset geometry changed");
	}
	psy_audio_presets_insert(&saved, 0, captured);
	status = psy_audio_presetsio_save(preset_path, &saved);
	if (status != psy_audio_PRESETIO_OK) {
		rc = fail("preset save failed");
		goto cleanup;
	}
	status = psy_audio_presetsio_load(preset_path, &loaded, PARAMETER_COUNT, 0, "");
	if (status != psy_audio_PRESETIO_OK || psy_audio_presets_size(&loaded) != 1) {
		rc = fail("preset reload failed");
		goto cleanup;
	}
	reloaded = psy_audio_presets_at(&loaded, 0);
	if (!reloaded || psy_audio_preset_num_parameters(reloaded) != PARAMETER_COUNT ||
			reloaded->datasize != 0) {
		rc = fail("reloaded preset geometry changed");
		goto cleanup;
	}
	for (i = 0; i < PARAMETER_COUNT; ++i) {
		if (psy_audio_preset_value(reloaded, i) != expected[i]) {
			rc = fail("reloaded preset parameter changed");
			goto cleanup;
		}
	}
	fresh = make_machine(factory);
	if (!fresh) {
		rc = fail("fresh preset-restore machine creation failed");
		goto cleanup;
	}
	psy_audio_machine_tweak_preset(fresh, reloaded);
	rc = verify_values(fresh, expected);
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
	char preset_path[4096];
	psy_audio_MachineCallback callback;
	psy_audio_PluginCatcher catcher;
	psy_audio_MachineFactory factory;
	psy_audio_Song* song = NULL;
	psy_audio_Machine* machine = NULL;
	intptr_t* expected = NULL;
	psy_audio_MachineCallback loaded_callback;
	psy_audio_PluginCatcher loaded_catcher;
	psy_audio_MachineFactory loaded_factory;
	psy_audio_Song* loaded = NULL;
	int rc = 0;

	if (argc != 3) {
		fprintf(stderr, "usage: %s OUTPUT_DIRECTORY ZEPHOD_SUPERFM_SO\n", argv[0]);
		return 2;
	}
	if (snprintf(song_path, sizeof(song_path), "%s/phase5-zephod-superfm.psy", argv[1]) >=
			(int)sizeof(song_path) ||
			snprintf(preset_path, sizeof(preset_path), "%s/phase5-zephod-superfm.prs", argv[1]) >=
			(int)sizeof(preset_path)) {
		return 2;
	}

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
		rc = 1;
		goto initial_cleanup;
	}
	psy_audio_machinecallback_set_song(&callback, song);
	machine = make_machine(&factory);
	if (!machine || verify_identity(machine) != 0 || seed_parameters(machine) != 0) {
		rc = 1;
		if (machine) psy_audio_machine_deallocate(machine);
		machine = NULL;
		goto song_cleanup;
	}
	expected = capture_values(machine);
	if (!expected) {
		rc = fail("could not capture complete parameter state");
		psy_audio_machine_deallocate(machine);
		machine = NULL;
		goto song_cleanup;
	}
	if (preset_roundtrip(&factory, machine, expected, preset_path) != 0) {
		rc = 1;
		psy_audio_machine_deallocate(machine);
		machine = NULL;
		goto song_cleanup;
	}
	psy_audio_machines_insert(psy_audio_song_machines(song), SLOT, machine);
	machine = NULL;
	psy_audio_machines_connect(psy_audio_song_machines(song),
		psy_audio_wire_make(SLOT, psy_audio_MASTER_INDEX));
	if (save_song(song, song_path) != 0) {
		rc = fail("PSY3 save failed");
		goto song_cleanup;
	}

	psy_audio_machinecallback_init(&loaded_callback);
	psy_audio_plugincatcher_init(&loaded_catcher, NULL);
	if (register_native_plugin(&loaded_catcher, argv[2]) != 0) {
		rc = 1;
		psy_audio_plugincatcher_dispose(&loaded_catcher);
		goto song_cleanup;
	}
	psy_audio_machinefactory_init(&loaded_factory, &loaded_callback, &loaded_catcher, NULL);
	psy_audio_machinefactory_createwithoutproxy(&loaded_factory);
	loaded = load_song(&loaded_factory, song_path);
	if (!loaded) {
		rc = fail("fresh PSY3 reopen failed");
	} else {
		psy_audio_Machine* reopened;
		psy_audio_machinecallback_set_song(&loaded_callback, loaded);
		reopened = psy_audio_machines_at(psy_audio_song_machines(loaded), SLOT);
		if (verify_values(reopened, expected) != 0) {
			rc = 1;
		} else if (!psy_audio_machines_connected(psy_audio_song_machines(loaded),
				psy_audio_wire_make(SLOT, psy_audio_MASTER_INDEX))) {
			rc = fail("wire to Master did not survive PSY3 reload");
		}
		psy_audio_song_deallocate(loaded);
		loaded = NULL;
	}
	psy_audio_machinefactory_dispose(&loaded_factory);
	psy_audio_plugincatcher_dispose(&loaded_catcher);

song_cleanup:
	if (song) psy_audio_song_deallocate(song);
initial_cleanup:
	free(expected);
	psy_audio_machinefactory_dispose(&factory);
	psy_audio_plugincatcher_dispose(&catcher);
	psy_audio_dispose();
	if (rc != 0) return rc;

	printf("phase5-zephod-superfm-state: PASS\n");
	printf("catcher: zephod-superfm:0\n");
	printf("state: 20/20 public parameters seeded non-default, 0 opaque bytes\n");
	printf("topology: Zephod SuperFM -> Master\n");
	printf("song: %s\n", song_path);
	return 0;
}
