/*
** PSYCLE-LINUX Phase 5C D. W. Aley family production persistence regression.
**
** Exercises all four retained DW native effects through PluginCatcher,
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
#include <plugincatcher.h>
#include <preset.h>
#include <presetio.h>
#include <presets.h>
#include <song.h>
#include <songio.h>
#include <wire.h>

#define SPEC_COUNT 4u
#define SKIP_SEED INTPTR_MIN

typedef enum SeedKind {
	SEED_EQ,
	SEED_GRANULIZER,
	SEED_IOPAN,
	SEED_TREMOLO
} SeedKind;

typedef struct DwSpec {
	const char* label;
	const char* catcher_name;
	const char* expected_name;
	const char* expected_shortname;
	const char* module_token;
	uintptr_t parameter_count;
	uintptr_t writable_seed_count;
	SeedKind seed_kind;
} DwSpec;

static const DwSpec SPECS[SPEC_COUNT] = {
	{"dw eq", "dw-eq:0", "dw eq", "eq", "dw-eq", 12u, 12u, SEED_EQ},
	{"dw granulizer", "dw-granulizer:0", "dw granulizer", "granulizer", "dw-granulizer", 50u, 39u, SEED_GRANULIZER},
	{"dw IoPan", "dw-iopan:0", "dw IoPan", "IoPan", "dw-iopan", 4u, 4u, SEED_IOPAN},
	{"dw Tremolo", "dw-tremolo:0", "dw Tremolo", "Tremolo", "dw-tremolo", 8u, 8u, SEED_TREMOLO},
};

typedef struct Snapshot {
	uintptr_t parameter_count;
	intptr_t* parameters;
	uintptr_t data_size;
	unsigned char* data;
} Snapshot;

static int fail_spec(const DwSpec* spec, const char* message)
{
	fprintf(stderr, "phase5-dw-family-state: FAIL [%s]: %s\n",
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

static int snapshot_from_machine(const DwSpec* spec,
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
	for (i = 0; i < out->parameter_count; ++i)
		out->parameters[i] = psy_audio_preset_value(&preset, i);
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

static int snapshot_equal(const DwSpec* spec, const Snapshot* expected,
	const Snapshot* actual, const char* context)
{
	uintptr_t i;
	if (expected->parameter_count != actual->parameter_count)
		return fail_spec(spec, "persistence parameter count changed");
	for (i = 0; i < expected->parameter_count; ++i) {
		if (expected->parameters[i] != actual->parameters[i]) {
			fprintf(stderr,
				"phase5-dw-family-state: FAIL [%s]: %s parameter %lu expected %ld got %ld\n",
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

static int register_native(const DwSpec* spec,
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
			!info.author || strcmp(info.author, "dw") != 0) {
		rc = fail_spec(spec, "production native-plugin probe returned unexpected identity");
	} else {
		machineinfo_catchername(&info, catcher_name);
		if (strcmp(catcher_name, spec->catcher_name) != 0) {
			fprintf(stderr,
				"phase5-dw-family-state: FAIL [%s]: catcher expected %s got %s\n",
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

static psy_audio_Machine* make_machine(const DwSpec* spec,
	psy_audio_MachineFactory* factory)
{
	return psy_audio_machinefactory_make_machine(factory, psy_audio_PLUGIN,
		spec->catcher_name, psy_INDEX_INVALID);
}

static int verify_identity(const DwSpec* spec, psy_audio_Machine* machine)
{
	const psy_audio_MachineInfo* info;
	if (!machine || psy_audio_machine_type(machine) != psy_audio_PLUGIN)
		return fail_spec(spec, "machine is missing or no longer a native plugin");
	info = psy_audio_machine_info(machine);
	if (!info || !info->name || strcmp(info->name, spec->expected_name) != 0 ||
			!info->shortname || strcmp(info->shortname, spec->expected_shortname) != 0 ||
			!info->author || strcmp(info->author, "dw") != 0 ||
			!info->modulepath || !strstr(info->modulepath, spec->module_token))
		return fail_spec(spec, "identity/module path changed");
	if (psy_audio_machine_num_parameters(machine) != spec->parameter_count)
		return fail_spec(spec, "production parameter count changed");
	if (psy_audio_machine_data_size(machine) != 0)
		return fail_spec(spec, "DW machine unexpectedly acquired opaque state");
	return 0;
}

static int read_value(const DwSpec* spec, psy_audio_Machine* machine,
	uintptr_t index, intptr_t* out)
{
	psy_audio_MachineParam* param = psy_audio_machine_parameter(machine, index);
	if (!param) return fail_spec(spec, "parameter surface is incomplete");
	*out = psy_audio_machine_parameter_scaled_value(machine, param);
	return 0;
}

static int tweak_value(const DwSpec* spec, psy_audio_Machine* machine,
	uintptr_t index, intptr_t requested)
{
	psy_audio_MachineParam* param = psy_audio_machine_parameter(machine, index);
	intptr_t minval, maxval, actual;
	if (!param) return fail_spec(spec, "parameter surface is incomplete");
	psy_audio_machine_parameter_range(machine, param, &minval, &maxval);
	if (requested < minval || requested > maxval) {
		fprintf(stderr,
			"phase5-dw-family-state: FAIL [%s]: seed %lu=%ld outside %ld..%ld\n",
			spec->label, (unsigned long)index, (long)requested,
			(long)minval, (long)maxval);
		return 1;
	}
	psy_audio_machine_parameter_tweak_scaled(machine, param, requested);
	actual = psy_audio_machine_parameter_scaled_value(machine, param);
	if (actual != requested) {
		fprintf(stderr,
			"phase5-dw-family-state: FAIL [%s]: seed %lu requested %ld got %ld\n",
			spec->label, (unsigned long)index, (long)requested, (long)actual);
		return 1;
	}
	return 0;
}

static int seed_eq(const DwSpec* spec, psy_audio_Machine* machine,
	const intptr_t* defaults)
{
	uintptr_t i;
	for (i = 0; i < spec->parameter_count; ++i) {
		psy_audio_MachineParam* param = psy_audio_machine_parameter(machine, i);
		intptr_t minval, maxval, requested;
		if (!param) return fail_spec(spec, "EQ parameter missing");
		psy_audio_machine_parameter_range(machine, param, &minval, &maxval);
		requested = maxval;
		if (requested == defaults[i]) requested = minval;
		if (requested == defaults[i]) return fail_spec(spec, "EQ parameter has no non-default seed");
		if (tweak_value(spec, machine, i, requested) != 0) return 1;
	}
	return 0;
}

static int seed_granulizer(const DwSpec* spec, psy_audio_Machine* machine)
{
	static const intptr_t seeds[50] = {
		SKIP_SEED, 2000, 1200, 300, 400, 900, 800, 1, 4, SKIP_SEED,
		SKIP_SEED, 10, 20, 30, 40, 50, 60, SKIP_SEED, 1, 2,
		SKIP_SEED, 110, 120, 130, 140, 150, 160, SKIP_SEED, 20000, 22000,
		SKIP_SEED, 90, 80, 70, 60, 50, 40, SKIP_SEED, 45, 90,
		SKIP_SEED, 20000, 200, SKIP_SEED, 1, 2, SKIP_SEED, SKIP_SEED, SKIP_SEED, SKIP_SEED
	};
	uintptr_t i;
	uintptr_t seeded = 0;
	for (i = 0; i < spec->parameter_count; ++i) {
		if (seeds[i] == SKIP_SEED) continue;
		if (tweak_value(spec, machine, i, seeds[i]) != 0) return 1;
		++seeded;
	}
	if (seeded != spec->writable_seed_count)
		return fail_spec(spec, "Granulizer writable seed table count changed");
	return 0;
}

static int seed_iopan(const DwSpec* spec, psy_audio_Machine* machine)
{
	/* Establish extents while Maintain=None/Allow Flip=Yes, then freeze the
	** centered/no-flip final state. The resulting four values are all non-default. */
	if (tweak_value(spec, machine, 0, 24) != 0 ||
			tweak_value(spec, machine, 2, 104) != 0 ||
			tweak_value(spec, machine, 3, 0) != 0 ||
			tweak_value(spec, machine, 1, 2) != 0)
		return 1;
	return 0;
}

static int seed_tremolo(const DwSpec* spec, psy_audio_Machine* machine)
{
	static const intptr_t values[8] = {500, 1, 80, 300, 2500, 180, 1, 1};
	uintptr_t i;
	for (i = 0; i < 8; ++i)
		if (tweak_value(spec, machine, i, values[i]) != 0) return 1;
	return 0;
}

static int seed_machine(const DwSpec* spec, psy_audio_Machine* machine)
{
	intptr_t* defaults;
	uintptr_t i;
	uintptr_t changed = 0;
	int rc = 0;
	defaults = (intptr_t*)malloc(spec->parameter_count * sizeof(intptr_t));
	if (!defaults) return fail_spec(spec, "default snapshot allocation failed");
	for (i = 0; i < spec->parameter_count; ++i) {
		if (read_value(spec, machine, i, &defaults[i]) != 0) {
			free(defaults);
			return 1;
		}
	}

	switch (spec->seed_kind) {
	case SEED_EQ: rc = seed_eq(spec, machine, defaults); break;
	case SEED_GRANULIZER: rc = seed_granulizer(spec, machine); break;
	case SEED_IOPAN: rc = seed_iopan(spec, machine); break;
	case SEED_TREMOLO: rc = seed_tremolo(spec, machine); break;
	}
	if (rc != 0) {
		free(defaults);
		return rc;
	}

	for (i = 0; i < spec->parameter_count; ++i) {
		intptr_t actual;
		if (read_value(spec, machine, i, &actual) != 0) {
			free(defaults);
			return 1;
		}
		if (actual != defaults[i]) ++changed;
	}
	free(defaults);
	if (changed < spec->writable_seed_count) {
		fprintf(stderr,
			"phase5-dw-family-state: FAIL [%s]: only %lu parameters differ from fresh defaults; expected at least %lu\n",
			spec->label, (unsigned long)changed,
			(unsigned long)spec->writable_seed_count);
		return 1;
	}
	printf("phase5-dw-family-state: seed PASS [%s] changed=%lu\n",
		spec->label, (unsigned long)changed);
	return 0;
}

static int preset_roundtrip(const DwSpec* spec,
	psy_audio_MachineFactory* factory, psy_audio_Machine* source,
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
	fresh = make_machine(spec, factory);
	if (!fresh || verify_identity(spec, fresh) != 0) {
		rc = fail_spec(spec, "fresh preset-restore machine creation failed");
		goto cleanup;
	}
	psy_audio_machine_tweak_preset(fresh, reloaded);
	if (snapshot_from_machine(spec, fresh, &actual) != 0 ||
			snapshot_equal(spec, expected, &actual, "preset restore") != 0) {
		rc = 1;
		goto cleanup;
	}
	printf("phase5-dw-family-state: preset PASS [%s]\n", spec->label);

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
	psy_audio_Song* song = NULL;
	psy_audio_Machine* machines[SPEC_COUNT] = {NULL, NULL, NULL, NULL};
	Snapshot expected[SPEC_COUNT];
	char preset_paths[SPEC_COUNT][4096];
	char song_path[4096];
	uintptr_t i;
	int rc = 0;

	psy_audio_MachineCallback loaded_callback;
	psy_audio_PluginCatcher loaded_catcher;
	psy_audio_MachineFactory loaded_factory;
	psy_audio_Song* loaded = NULL;

	if (argc != 6) {
		fprintf(stderr,
			"usage: %s OUTPUT_DIR DW_EQ_SO DW_GRANULIZER_SO DW_IOPAN_SO DW_TREMOLO_SO\n",
			argv[0]);
		return 2;
	}
	for (i = 0; i < SPEC_COUNT; ++i) snapshot_init(&expected[i]);
	if (snprintf(song_path, sizeof(song_path), "%s/phase5-dw-family.psy", argv[1]) >=
			(int)sizeof(song_path))
		return fail_spec(NULL, "song output path too long");
	for (i = 0; i < SPEC_COUNT; ++i) {
		if (snprintf(preset_paths[i], sizeof(preset_paths[i]), "%s/phase5-dw-%lu.prs",
				argv[1], (unsigned long)i) >= (int)sizeof(preset_paths[i]))
			return fail_spec(&SPECS[i], "preset output path too long");
	}

	psy_audio_init();
	psy_audio_machinecallback_init(&callback);
	psy_audio_plugincatcher_init(&catcher, NULL);
	psy_audio_machinefactory_init(&factory, &callback, &catcher, NULL);
	psy_audio_machinefactory_createwithoutproxy(&factory);
	for (i = 0; i < SPEC_COUNT; ++i) {
		if (register_native(&SPECS[i], &catcher, argv[i + 2]) != 0) {
			rc = 1;
			goto initial_cleanup;
		}
	}

	song = psy_audio_song_alloc_init(&factory);
	if (!song) {
		rc = fail_spec(NULL, "could not allocate DW family song");
		goto initial_cleanup;
	}
	psy_audio_machinecallback_set_song(&callback, song);
	for (i = 0; i < SPEC_COUNT; ++i) {
		const DwSpec* spec = &SPECS[i];
		machines[i] = make_machine(spec, &factory);
		if (!machines[i] || verify_identity(spec, machines[i]) != 0 ||
				seed_machine(spec, machines[i]) != 0 ||
				snapshot_from_machine(spec, machines[i], &expected[i]) != 0 ||
				expected[i].data_size != 0 ||
				preset_roundtrip(spec, &factory, machines[i], &expected[i], preset_paths[i]) != 0) {
			rc = 1;
			goto song_cleanup;
		}
		psy_audio_machines_insert(psy_audio_song_machines(song), i, machines[i]);
		machines[i] = NULL;
		psy_audio_machines_connect(psy_audio_song_machines(song),
			psy_audio_wire_make(i, psy_audio_MASTER_INDEX));
	}
	if (save_song(song, song_path) != 0) {
		rc = fail_spec(NULL, "DW family PSY3 save failed");
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
		rc = fail_spec(NULL, "fresh DW family PSY3 reopen failed");
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
	psy_audio_machinefactory_dispose(&factory);
	psy_audio_plugincatcher_dispose(&catcher);
	psy_audio_dispose();
	for (i = 0; i < SPEC_COUNT; ++i) snapshot_dispose(&expected[i]);
	if (rc != 0) return rc;

	printf("phase5-dw-family-state: PASS machines=4\n");
	printf("catchers: dw-eq:0 dw-granulizer:0 dw-iopan:0 dw-tremolo:0\n");
	printf("state: EQ 12/12, Granulizer 39 writable non-default + runtime display state, IoPan 4/4, Tremolo 8/8; 0 opaque bytes\n");
	printf("topology: 4/4 DW effects -> Master\n");
	printf("song: %s\n", song_path);
	return 0;
}
