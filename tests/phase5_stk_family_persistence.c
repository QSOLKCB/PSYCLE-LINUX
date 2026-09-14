/*
** PSYCLE-LINUX Phase 5C STK-derived production persistence regression.
**
** Exercises stk Plucked, stk Reverbs and stk Shakers through PluginCatcher,
** MachineFactory, version-1 preset I/O and one-song fresh PSY3 reopen.
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <machine.h>
#include <machinefactory.h>
#include <machineinfo.h>
#include <machines.h>
#include <player.h>
#include <plugin.h>
#include <plugin_interface.h>
#include <plugincatcher.h>
#include <preset.h>
#include <presetio.h>
#include <presets.h>
#include <song.h>
#include <songio.h>
#include <wire.h>

#define SPEC_COUNT 3u
#define MAX_PARAMS 6u

typedef struct StkSpec {
	const char* label;
	const char* catcher_name;
	const char* expected_name;
	const char* expected_shortname;
	const char* expected_author;
	const char* module_token;
	uintptr_t parameter_count;
	intptr_t seeds[MAX_PARAMS];
} StkSpec;

static const StkSpec SPECS[SPEC_COUNT] = {
	{"stk Plucked", "stk-plucked:0", "stk Plucked", "stk Plucked",
		"Sartorius, Bohan and STK 4.2.0 developers", "stk-plucked", 5u,
		{20000, 1000, 2000, 12000, 4000, 0}},
	{"stk Reverbs", "stk-reverbs:0", "stk Reverbs", "stk Reverbs",
		"Sartorius and STK developers", "stk-reverbs", 4u,
		{2, 120, 40, 0, 0, 0}},
	{"stk Shakers", "stk-shakers:0", "stk Shakers", "Shakers",
		"Sartorius, bohan and STK 4.5.0 developers", "stk-shakers", 6u,
		{80, 90, 20, 70, 75, 20000}},
};

typedef struct Snapshot {
	uintptr_t parameter_count;
	intptr_t* parameters;
	uintptr_t data_size;
	unsigned char* data;
} Snapshot;

static int fail_spec(const StkSpec* spec, const char* message)
{
	fprintf(stderr, "phase5-stk-family-state: FAIL [%s]: %s\n",
		spec ? spec->label : "family", message);
	return 1;
}

static void snapshot_init(Snapshot* self)
{
	self->parameter_count = 0;
	self->parameters = NULL;
	self->data_size = 0;
	self->data = NULL;
}

static void snapshot_dispose(Snapshot* self)
{
	free(self->parameters);
	free(self->data);
	snapshot_init(self);
}

static int snapshot_from_machine(const StkSpec* spec,
	psy_audio_Machine* machine, Snapshot* out)
{
	psy_audio_Preset preset;
	uintptr_t i;
	snapshot_dispose(out);
	psy_audio_preset_init(&preset);
	psy_audio_machine_current_preset(machine, &preset);
	out->parameter_count = psy_audio_preset_num_parameters(&preset);
	if (out->parameter_count != spec->parameter_count) {
		psy_audio_preset_dispose(&preset);
		return fail_spec(spec, "captured preset parameter count changed");
	}
	out->parameters = (intptr_t*)malloc(out->parameter_count * sizeof(intptr_t));
	if (!out->parameters) {
		psy_audio_preset_dispose(&preset);
		return fail_spec(spec, "snapshot parameter allocation failed");
	}
	for (i = 0; i < out->parameter_count; ++i) {
		psy_audio_MachineParam* param = psy_audio_machine_parameter(machine, i);
		if (!param) {
			psy_audio_preset_dispose(&preset);
			return fail_spec(spec, "snapshot parameter surface is incomplete");
		}
		if ((psy_audio_machine_parameter_type(machine, param) & 0x1FF) != MPF_STATE) {
			psy_audio_preset_dispose(&preset);
			return fail_spec(spec, "public parameter is no longer MPF_STATE");
		}
		out->parameters[i] = psy_audio_preset_value(&preset, i);
	}
	out->data_size = preset.datasize;
	if (out->data_size > 0) {
		if (!preset.data) {
			psy_audio_preset_dispose(&preset);
			return fail_spec(spec, "opaque size is nonzero but data pointer is null");
		}
		out->data = (unsigned char*)malloc(out->data_size);
		if (!out->data) {
			psy_audio_preset_dispose(&preset);
			return fail_spec(spec, "snapshot opaque allocation failed");
		}
		memcpy(out->data, preset.data, out->data_size);
	}
	psy_audio_preset_dispose(&preset);
	return 0;
}

static int snapshot_equal(const StkSpec* spec, const Snapshot* expected,
	const Snapshot* actual, const char* context)
{
	uintptr_t i;
	if (expected->parameter_count != actual->parameter_count)
		return fail_spec(spec, "persistence parameter count changed");
	for (i = 0; i < expected->parameter_count; ++i) {
		if (expected->parameters[i] != actual->parameters[i]) {
			fprintf(stderr,
				"phase5-stk-family-state: FAIL [%s]: %s slot %lu expected %ld got %ld\n",
				spec->label, context, (unsigned long)i,
				(long)expected->parameters[i], (long)actual->parameters[i]);
			return 1;
		}
	}
	if (expected->data_size != actual->data_size)
		return fail_spec(spec, "opaque state size changed");
	if (expected->data_size > 0 && (!expected->data || !actual->data ||
			memcmp(expected->data, actual->data, expected->data_size) != 0))
		return fail_spec(spec, "opaque state bytes changed");
	return 0;
}

static int register_native(const StkSpec* spec,
	psy_audio_PluginCatcher* catcher, const char* path)
{
	psy_audio_MachineInfo info;
	char catcher_name[256];
	int rc = 0;
	machineinfo_init(&info);
	if (!psy_audio_plugin_psycle_test(path, catcher->native_root_, &info)) {
		rc = fail_spec(spec, "production native-plugin probe failed");
	} else if (info.type != psy_audio_PLUGIN || !info.name ||
			strcmp(info.name, spec->expected_name) != 0 ||
			!info.shortname || strcmp(info.shortname, spec->expected_shortname) != 0 ||
			!info.author || strcmp(info.author, spec->expected_author) != 0) {
		rc = fail_spec(spec, "production native-plugin probe returned unexpected identity");
	} else {
		machineinfo_catchername(&info, catcher_name);
		if (strcmp(catcher_name, spec->catcher_name) != 0) {
			fprintf(stderr,
				"phase5-stk-family-state: FAIL [%s]: catcher expected %s got %s\n",
				spec->label, spec->catcher_name, catcher_name);
			rc = 1;
		} else {
			psy_audio_plugins_add(&catcher->plugins_, &info);
			if (!psy_audio_plugins_at_id_const(&catcher->plugins_, catcher_name))
				rc = fail_spec(spec, "PluginCatcher registration disappeared");
		}
	}
	machineinfo_dispose(&info);
	return rc;
}

static psy_audio_Machine* make_machine(const StkSpec* spec,
	psy_audio_MachineFactory* factory)
{
	return psy_audio_machinefactory_make_machine(factory, psy_audio_PLUGIN,
		spec->catcher_name, psy_INDEX_INVALID);
}

static int verify_identity(const StkSpec* spec, psy_audio_Machine* machine)
{
	const psy_audio_MachineInfo* info;
	if (!machine || psy_audio_machine_type(machine) != psy_audio_PLUGIN)
		return fail_spec(spec, "machine is missing or no longer a native plugin");
	info = psy_audio_machine_info(machine);
	if (!info || !info->name || strcmp(info->name, spec->expected_name) != 0 ||
			!info->shortname || strcmp(info->shortname, spec->expected_shortname) != 0 ||
			!info->author || strcmp(info->author, spec->expected_author) != 0 ||
			!info->modulepath || !strstr(info->modulepath, spec->module_token))
		return fail_spec(spec, "identity/module path changed");
	if (psy_audio_machine_num_parameters(machine) != spec->parameter_count)
		return fail_spec(spec, "production parameter count changed");
	if (psy_audio_machine_data_size(machine) != 0)
		return fail_spec(spec, "STK wrapper unexpectedly acquired opaque state");
	return 0;
}

static int tweak_value(const StkSpec* spec, psy_audio_Machine* machine,
	uintptr_t index, intptr_t requested)
{
	psy_audio_MachineParam* param = psy_audio_machine_parameter(machine, index);
	intptr_t minval, maxval, actual;
	if (!param) return fail_spec(spec, "parameter surface is incomplete");
	psy_audio_machine_parameter_range(machine, param, &minval, &maxval);
	if (requested < minval || requested > maxval) {
		fprintf(stderr,
			"phase5-stk-family-state: FAIL [%s]: seed %lu=%ld outside %ld..%ld\n",
			spec->label, (unsigned long)index, (long)requested,
			(long)minval, (long)maxval);
		return 1;
	}
	psy_audio_machine_parameter_tweak_scaled(machine, param, requested);
	actual = psy_audio_machine_parameter_scaled_value(machine, param);
	if (actual != requested) {
		fprintf(stderr,
			"phase5-stk-family-state: FAIL [%s]: seed %lu requested %ld got %ld\n",
			spec->label, (unsigned long)index, (long)requested, (long)actual);
		return 1;
	}
	return 0;
}

static int seed_machine(const StkSpec* spec, psy_audio_Machine* machine)
{
	uintptr_t i;
	for (i = 0; i < spec->parameter_count; ++i) {
		psy_audio_MachineParam* param = psy_audio_machine_parameter(machine, i);
		intptr_t before;
		if (!param) return fail_spec(spec, "parameter missing during seed");
		before = psy_audio_machine_parameter_scaled_value(machine, param);
		if (before == spec->seeds[i])
			return fail_spec(spec, "seed unexpectedly equals fresh default");
		if (tweak_value(spec, machine, i, spec->seeds[i]) != 0) return 1;
	}
	printf("phase5-stk-family-state: seed PASS [%s] changed=%lu\n",
		spec->label, (unsigned long)spec->parameter_count);
	return 0;
}

static int preset_roundtrip(const StkSpec* spec,
	psy_audio_MachineFactory* restore_factory, psy_audio_Machine* source,
	const Snapshot* expected, const char* path)
{
	psy_audio_Presets saved;
	psy_audio_Presets loaded;
	psy_audio_Preset* captured;
	psy_audio_Preset* reloaded;
	psy_audio_Machine* fresh = NULL;
	Snapshot actual;
	int status;
	int rc = 0;
	char preset_name[32];

	psy_audio_presets_init(&saved);
	psy_audio_presets_init(&loaded);
	snapshot_init(&actual);
	captured = psy_audio_preset_alloc_init();
	if (!captured) {
		rc = fail_spec(spec, "preset allocation failed");
		goto cleanup;
	}
	psy_audio_machine_current_preset(source, captured);
	snprintf(preset_name, sizeof(preset_name), "P5 %s", spec->expected_shortname);
	psy_audio_preset_set_name(captured, preset_name);
	if (psy_audio_preset_num_parameters(captured) != spec->parameter_count ||
			captured->datasize != 0 || captured->data != NULL) {
		psy_audio_preset_dispose(captured);
		free(captured);
		rc = fail_spec(spec, "captured preset geometry changed");
		goto cleanup;
	}
	psy_audio_presets_insert(&saved, 0, captured);
	captured = NULL;

	status = psy_audio_presetsio_save(path, &saved);
	if (status != psy_audio_PRESETIO_OK) {
		rc = fail_spec(spec, "preset save failed");
		goto cleanup;
	}
	status = psy_audio_presetsio_load(path, &loaded, spec->parameter_count, 0, "");
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
	fresh = make_machine(spec, restore_factory);
	if (!fresh || verify_identity(spec, fresh) != 0) {
		rc = fail_spec(spec, "independent preset-restore machine creation failed");
		goto cleanup;
	}
	psy_audio_machine_tweak_preset(fresh, reloaded);
	if (snapshot_from_machine(spec, fresh, &actual) != 0 ||
			snapshot_equal(spec, expected, &actual, "preset restore") != 0) {
		rc = 1;
		goto cleanup;
	}
	printf("phase5-stk-family-state: preset PASS [%s]\n", spec->label);

cleanup:
	if (fresh) psy_audio_machine_deallocate(fresh);
	snapshot_dispose(&actual);
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
	psy_audio_MachineCallback callback;
	psy_audio_PluginCatcher catcher;
	psy_audio_MachineFactory factory;
	psy_audio_MachineCallback preset_callback;
	psy_audio_PluginCatcher preset_catcher;
	psy_audio_MachineFactory preset_factory;
	psy_audio_Song* song = NULL;
	psy_audio_Machine* machines[SPEC_COUNT] = {NULL, NULL, NULL};
	Snapshot expected[SPEC_COUNT];
	char preset_paths[SPEC_COUNT][4096];
	char song_path[4096];
	uintptr_t i;
	int rc = 0;

	psy_audio_MachineCallback loaded_callback;
	psy_audio_PluginCatcher loaded_catcher;
	psy_audio_MachineFactory loaded_factory;
	psy_audio_Song* loaded = NULL;

	if (argc != 5) {
		fprintf(stderr,
			"usage: %s OUTPUT_DIR STK_PLUCKED_SO STK_REVERBS_SO STK_SHAKERS_SO\n",
			argv[0]);
		return 2;
	}
	for (i = 0; i < SPEC_COUNT; ++i) snapshot_init(&expected[i]);
	if (snprintf(song_path, sizeof(song_path), "%s/phase5-stk-family.psy", argv[1]) >=
			(int)sizeof(song_path))
		return fail_spec(NULL, "song output path too long");
	for (i = 0; i < SPEC_COUNT; ++i) {
		if (snprintf(preset_paths[i], sizeof(preset_paths[i]), "%s/phase5-stk-%lu.prs",
				argv[1], (unsigned long)i) >= (int)sizeof(preset_paths[i]))
			return fail_spec(&SPECS[i], "preset output path too long");
	}

	psy_audio_init();
	psy_audio_machinecallback_init(&callback);
	psy_audio_plugincatcher_init(&catcher, NULL);
	psy_audio_machinefactory_init(&factory, &callback, &catcher, NULL);
	psy_audio_machinefactory_createwithoutproxy(&factory);

	psy_audio_machinecallback_init(&preset_callback);
	psy_audio_plugincatcher_init(&preset_catcher, NULL);
	psy_audio_machinefactory_init(&preset_factory, &preset_callback, &preset_catcher, NULL);
	psy_audio_machinefactory_createwithoutproxy(&preset_factory);

	for (i = 0; i < SPEC_COUNT; ++i) {
		if (register_native(&SPECS[i], &catcher, argv[i + 2]) != 0 ||
				register_native(&SPECS[i], &preset_catcher, argv[i + 2]) != 0) {
			rc = 1;
			goto initial_cleanup;
		}
	}

	song = psy_audio_song_alloc_init(&factory);
	if (!song) {
		rc = fail_spec(NULL, "could not allocate STK family song");
		goto initial_cleanup;
	}
	psy_audio_machinecallback_set_song(&callback, song);
	for (i = 0; i < SPEC_COUNT; ++i) {
		const StkSpec* spec = &SPECS[i];
		machines[i] = make_machine(spec, &factory);
		if (!machines[i] || verify_identity(spec, machines[i]) != 0 ||
				seed_machine(spec, machines[i]) != 0 ||
				snapshot_from_machine(spec, machines[i], &expected[i]) != 0 ||
				expected[i].data_size != 0 ||
				preset_roundtrip(spec, &preset_factory, machines[i], &expected[i], preset_paths[i]) != 0) {
			rc = 1;
			goto song_cleanup;
		}
		psy_audio_machines_insert(psy_audio_song_machines(song), i, machines[i]);
		machines[i] = NULL;
		psy_audio_machines_connect(psy_audio_song_machines(song),
			psy_audio_wire_make(i, psy_audio_MASTER_INDEX));
	}
	if (save_song(song, song_path) != 0) {
		rc = fail_spec(NULL, "STK family PSY3 save failed");
		goto song_cleanup;
	}

	psy_audio_machinecallback_init(&loaded_callback);
	psy_audio_plugincatcher_init(&loaded_catcher, NULL);
	for (i = 0; i < SPEC_COUNT; ++i) {
		if (register_native(&SPECS[i], &loaded_catcher, argv[i + 2]) != 0) {
			rc = 1;
			psy_audio_plugincatcher_dispose(&loaded_catcher);
			goto song_cleanup;
		}
	}
	psy_audio_machinefactory_init(&loaded_factory, &loaded_callback, &loaded_catcher, NULL);
	psy_audio_machinefactory_createwithoutproxy(&loaded_factory);
	loaded = load_song(&loaded_factory, song_path);
	if (!loaded) {
		rc = fail_spec(NULL, "fresh STK family PSY3 reopen failed");
	} else {
		psy_audio_machinecallback_set_song(&loaded_callback, loaded);
		for (i = 0; i < SPEC_COUNT; ++i) {
			psy_audio_Machine* reopened = psy_audio_machines_at(
				psy_audio_song_machines(loaded), i);
			Snapshot actual;
			snapshot_init(&actual);
			if (verify_identity(&SPECS[i], reopened) != 0 ||
					snapshot_from_machine(&SPECS[i], reopened, &actual) != 0 ||
					snapshot_equal(&SPECS[i], &expected[i], &actual, "PSY3 reopen") != 0) {
				rc = 1;
				snapshot_dispose(&actual);
				break;
			}
			if (!psy_audio_machines_connected(psy_audio_song_machines(loaded),
					psy_audio_wire_make(i, psy_audio_MASTER_INDEX))) {
				rc = fail_spec(&SPECS[i], "wire to Master did not survive PSY3 reopen");
				snapshot_dispose(&actual);
				break;
			}
			snapshot_dispose(&actual);
		}
		psy_audio_song_deallocate(loaded);
		loaded = NULL;
	}
	psy_audio_machinefactory_dispose(&loaded_factory);
	psy_audio_plugincatcher_dispose(&loaded_catcher);

song_cleanup:
	for (i = 0; i < SPEC_COUNT; ++i) {
		if (machines[i]) psy_audio_machine_deallocate(machines[i]);
	}
	if (song) psy_audio_song_deallocate(song);
initial_cleanup:
	psy_audio_machinefactory_dispose(&preset_factory);
	psy_audio_plugincatcher_dispose(&preset_catcher);
	psy_audio_machinefactory_dispose(&factory);
	psy_audio_plugincatcher_dispose(&catcher);
	psy_audio_dispose();
	for (i = 0; i < SPEC_COUNT; ++i) snapshot_dispose(&expected[i]);
	if (rc != 0) return rc;

	printf("phase5-stk-family-state: PASS machines=3\n");
	printf("catchers: stk-plucked:0 stk-reverbs:0 stk-shakers:0\n");
	printf("state: Plucked 5/5; Reverbs 4/4; Shakers 6/6; 0 opaque bytes\n");
	printf("preset-factory: independent PluginCatcher/MachineFactory\n");
	printf("topology: 3/3 STK wrappers -> Master\n");
	printf("song: %s\n", song_path);
	return 0;
}
