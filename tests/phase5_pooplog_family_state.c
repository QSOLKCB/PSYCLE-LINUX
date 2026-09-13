/*
** PSYCLE-LINUX Phase 5B Pooplog family production state regression.
**
** Registers every retained source-built Pooplog native machine in a production
** PluginCatcher, creates them through MachineFactory, exercises every exposed
** parameter through the public scaled API, round-trips per-machine presets, and
** saves/reopens one PSY3 song containing the complete nine-binary family.
**
** The FM Laboratory / Light / UltraLight machines have real historical opaque
** GetData state; those bytes are compared exactly across preset and PSY3 paths.
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

#define SPEC_COUNT 9u
#define FIRST_SLOT 0u

/* Keep this table in the same order as phase5_pooplog_family.cpp and the smoke
** script argument list. The catcher IDs correspond to actual retained/source-
** built binaries; the stale host category alias pooplog-scratch-master-2:0 has
** no retained source/build target and is intentionally not represented here. */
typedef struct PooplogSpec {
	const char* label;
	const char* catcher_name;
	const char* expected_name;
	const char* expected_shortname;
	const char* module_token;
	uintptr_t parameter_count;
	int expects_opaque_state;
} PooplogSpec;

static const PooplogSpec SPECS[SPEC_COUNT] = {
	{"FM Laboratory", "pooplog-fm-laboratory:0", "Pooplog FM Laboratory0.68b",
		"Pooplog", "pooplog-fm-laboratory", 101u, 1},
	{"FM Light", "pooplog-fm-light:0", "Pooplog FM Light0.68b",
		"Pooplog Light", "pooplog-fm-light", 57u, 1},
	{"FM UltraLight", "pooplog-fm-ultralight:0", "Pooplog FM UltraLight0.68b",
		"Pooplog UltraL", "pooplog-fm-ultralight", 45u, 1},
	{"Delay", "pooplog-delay:0", "Pooplog Delay 0.04b",
		"Pooplog Delay", "pooplog-delay", 43u, 0},
	{"Delay Light", "pooplog-delay-light:0", "Pooplog Delay Light 0.04b",
		"Pooplog Delay L", "pooplog-delay-light", 28u, 0},
	{"Filter", "pooplog-filter:0", "Pooplog Filter 0.06b",
		"Pooplog Filter", "pooplog-filter", 16u, 0},
	{"Autopan", "pooplog-autopan:0", "Pooplog Autopan 0.06b",
		"Pooplog Autopan", "pooplog-autopan", 9u, 0},
	{"Lofi", "pooplog-lofi-processor:0", "Pooplog Lofi Processor 0.04b",
		"Pooplog Lofi", "pooplog-lofi-processor", 4u, 0},
	{"Scratch", "pooplog-scratch-master:0", "Pooplog Scratch Master 0.06b",
		"Pooplog Scratch", "pooplog-scratch-master", 8u, 0},
};

typedef struct Snapshot {
	uintptr_t parameter_count;
	intptr_t* parameters;
	uintptr_t data_size;
	unsigned char* data;
} Snapshot;

static int fail_spec(const PooplogSpec* spec, const char* message)
{
	fprintf(stderr, "phase5-pooplog-family-state: FAIL [%s]: %s\n",
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

static int snapshot_from_preset(const PooplogSpec* spec,
	const psy_audio_Preset* preset, Snapshot* out)
{
	uintptr_t i;

	snapshot_dispose(out);
	out->parameter_count = psy_audio_preset_num_parameters(preset);
	if (out->parameter_count != spec->parameter_count) {
		return fail_spec(spec, "captured preset parameter count changed");
	}
	if (out->parameter_count > 0) {
		out->parameters = (intptr_t*)malloc(out->parameter_count * sizeof(intptr_t));
		if (!out->parameters) return fail_spec(spec, "could not allocate parameter snapshot");
		for (i = 0; i < out->parameter_count; ++i) {
			out->parameters[i] = psy_audio_preset_value((psy_audio_Preset*)preset, i);
		}
	}
	out->data_size = preset->datasize;
	if (out->data_size > 0) {
		out->data = (unsigned char*)malloc(out->data_size);
		if (!out->data) {
			snapshot_dispose(out);
			return fail_spec(spec, "could not allocate opaque-state snapshot");
		}
		if (!preset->data) {
			snapshot_dispose(out);
			return fail_spec(spec, "opaque-state size is nonzero but data pointer is null");
		}
		memcpy(out->data, preset->data, out->data_size);
	}
	return 0;
}

static int snapshot_from_machine(const PooplogSpec* spec,
	psy_audio_Machine* machine, Snapshot* out)
{
	psy_audio_Preset preset;
	int rc;

	psy_audio_preset_init(&preset);
	psy_audio_machine_current_preset(machine, &preset);
	rc = snapshot_from_preset(spec, &preset, out);
	psy_audio_preset_dispose(&preset);
	return rc;
}

static int snapshot_equal(const PooplogSpec* spec, const Snapshot* expected,
	const Snapshot* actual, const char* context)
{
	uintptr_t i;

	if (expected->parameter_count != actual->parameter_count) {
		fprintf(stderr,
			"phase5-pooplog-family-state: FAIL [%s]: %s parameter count %lu != %lu\n",
			spec->label, context, (unsigned long)actual->parameter_count,
			(unsigned long)expected->parameter_count);
		return 1;
	}
	for (i = 0; i < expected->parameter_count; ++i) {
		if (expected->parameters[i] != actual->parameters[i]) {
			fprintf(stderr,
				"phase5-pooplog-family-state: FAIL [%s]: %s parameter %lu expected %ld got %ld\n",
				spec->label, context, (unsigned long)i,
				(long)expected->parameters[i], (long)actual->parameters[i]);
			return 1;
		}
	}
	if (expected->data_size != actual->data_size) {
		fprintf(stderr,
			"phase5-pooplog-family-state: FAIL [%s]: %s opaque size %lu != %lu\n",
			spec->label, context, (unsigned long)actual->data_size,
			(unsigned long)expected->data_size);
		return 1;
	}
	if (expected->data_size > 0 &&
			(!expected->data || !actual->data ||
			memcmp(expected->data, actual->data, expected->data_size) != 0)) {
		return fail_spec(spec, "opaque state bytes changed across persistence path");
	}
	return 0;
}

static int verify_identity(const PooplogSpec* spec, psy_audio_Machine* machine)
{
	const psy_audio_MachineInfo* info;
	uintptr_t data_size;

	if (!machine || psy_audio_machine_type(machine) != psy_audio_PLUGIN) {
		return fail_spec(spec, "machine is missing or no longer a Psycle native plugin");
	}
	info = psy_audio_machine_info(machine);
	if (!info || !info->name || strcmp(info->name, spec->expected_name) != 0 ||
			!info->shortname || strcmp(info->shortname, spec->expected_shortname) != 0 ||
			!info->author || strcmp(info->author, "Jeremy Evers") != 0 ||
			!info->modulepath || !strstr(info->modulepath, spec->module_token)) {
		return fail_spec(spec, "identity/module path changed");
	}
	if (psy_audio_machine_num_parameters(machine) != spec->parameter_count) {
		return fail_spec(spec, "production parameter count changed");
	}
	data_size = psy_audio_machine_data_size(machine);
	if (spec->expects_opaque_state) {
		if (data_size == 0) return fail_spec(spec, "FM synth lost historical opaque state");
	} else if (data_size != 0) {
		return fail_spec(spec, "effect unexpectedly acquired opaque state");
	}
	return 0;
}

static int register_native_plugin(const PooplogSpec* spec,
	psy_audio_PluginCatcher* catcher, const char* module_path)
{
	psy_audio_MachineInfo info;
	char catcher_name[256];
	int rc;

	machineinfo_init(&info);
	rc = 0;
	if (!psy_audio_plugin_psycle_test(module_path, catcher->native_root_, &info)) {
		rc = fail_spec(spec, "production native-plugin probe failed");
	} else if (info.type != psy_audio_PLUGIN || !info.name ||
			strcmp(info.name, spec->expected_name) != 0) {
		rc = fail_spec(spec, "production native-plugin probe returned unexpected metadata");
	} else {
		machineinfo_catchername(&info, catcher_name);
		if (strcmp(catcher_name, spec->catcher_name) != 0) {
			fprintf(stderr,
				"phase5-pooplog-family-state: FAIL [%s]: catcher expected %s got %s\n",
				spec->label, spec->catcher_name, catcher_name);
			rc = 1;
		} else {
			psy_audio_plugins_add(&catcher->plugins_, &info);
			if (!psy_audio_plugins_at_id_const(&catcher->plugins_, catcher_name)) {
				rc = fail_spec(spec, "PluginCatcher registration failed");
			}
		}
	}
	machineinfo_dispose(&info);
	return rc;
}

static psy_audio_Machine* make_machine(const PooplogSpec* spec,
	psy_audio_MachineFactory* factory)
{
	return psy_audio_machinefactory_make_machine(factory, psy_audio_PLUGIN,
		spec->catcher_name, psy_INDEX_INVALID);
}

static int exercise_all_parameters(const PooplogSpec* spec,
	psy_audio_Machine* machine)
{
	uintptr_t i;

	for (i = 0; i < spec->parameter_count; ++i) {
		psy_audio_MachineParam* param;
		intptr_t minval;
		intptr_t maxval;
		intptr_t requested;

		param = psy_audio_machine_parameter(machine, i);
		if (!param) return fail_spec(spec, "production parameter surface is incomplete");
		psy_audio_machine_parameter_range(machine, param, &minval, &maxval);
		if (minval > maxval) return fail_spec(spec, "production parameter range is inverted");
		requested = (i & 1u) ? maxval : minval;
		psy_audio_machine_parameter_tweak_scaled(machine, param, requested);
	}
	return 0;
}

static int exercise_preset_roundtrip(const PooplogSpec* spec,
	psy_audio_MachineFactory* factory, psy_audio_Machine* source,
	const Snapshot* expected, const char* path)
{
	psy_audio_Presets saved;
	psy_audio_Presets loaded;
	psy_audio_Preset* captured;
	psy_audio_Preset* reloaded;
	psy_audio_Machine* fresh;
	Snapshot loaded_snapshot;
	Snapshot fresh_snapshot;
	char preset_name[256];
	int status;
	int rc;

	psy_audio_presets_init(&saved);
	psy_audio_presets_init(&loaded);
	snapshot_init(&loaded_snapshot);
	snapshot_init(&fresh_snapshot);
	captured = psy_audio_preset_alloc_init();
	if (!captured) {
		rc = fail_spec(spec, "could not allocate captured preset");
		goto cleanup;
	}
	psy_audio_machine_current_preset(source, captured);
	if (snprintf(preset_name, sizeof(preset_name), "Phase 5B %s state", spec->label) >=
			(int)sizeof(preset_name)) {
		psy_audio_preset_dispose(captured);
		free(captured);
		rc = fail_spec(spec, "preset name overflow");
		goto cleanup;
	}
	psy_audio_preset_set_name(captured, preset_name);
	psy_audio_presets_insert(&saved, 0, captured);

	status = psy_audio_presetsio_save(path, &saved);
	if (status != psy_audio_PRESETIO_OK) {
		rc = fail_spec(spec, "preset save failed");
		goto cleanup;
	}
	status = psy_audio_presetsio_load(path, &loaded, expected->parameter_count,
		expected->data_size, "");
	if (status != psy_audio_PRESETIO_OK || psy_audio_presets_size(&loaded) != 1) {
		rc = fail_spec(spec, "preset reload failed");
		goto cleanup;
	}
	reloaded = psy_audio_presets_at(&loaded, 0);
	if (!reloaded || strcmp(psy_audio_preset_name(reloaded), preset_name) != 0) {
		rc = fail_spec(spec, "preset identity changed across save/load");
		goto cleanup;
	}
	if ((rc = snapshot_from_preset(spec, reloaded, &loaded_snapshot)) != 0 ||
			(rc = snapshot_equal(spec, expected, &loaded_snapshot,
				"preset reload")) != 0) {
		goto cleanup;
	}

	fresh = make_machine(spec, factory);
	if (!fresh) {
		rc = fail_spec(spec, "could not create fresh machine for preset restore");
		goto cleanup;
	}
	psy_audio_machine_tweak_preset(fresh, reloaded);
	if ((rc = verify_identity(spec, fresh)) == 0 &&
			(rc = snapshot_from_machine(spec, fresh, &fresh_snapshot)) == 0) {
		rc = snapshot_equal(spec, expected, &fresh_snapshot, "fresh preset restore");
	}
	psy_audio_machine_deallocate(fresh);

cleanup:
	snapshot_dispose(&loaded_snapshot);
	snapshot_dispose(&fresh_snapshot);
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

static psy_audio_Song* load_song(psy_audio_MachineFactory* factory,
	const char* path)
{
	psy_audio_Song* song;
	psy_audio_SongReader reader;
	int status;

	song = psy_audio_song_alloc_init(factory);
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
	char preset_paths[SPEC_COUNT][4096];
	psy_audio_MachineCallback callback;
	psy_audio_PluginCatcher catcher;
	psy_audio_MachineFactory factory;
	psy_audio_Song* song;
	Snapshot expected[SPEC_COUNT];
	psy_audio_MachineCallback loaded_callback;
	psy_audio_PluginCatcher loaded_catcher;
	psy_audio_MachineFactory loaded_factory;
	psy_audio_Song* loaded;
	uintptr_t i;
	int rc;

	if (argc != (int)SPEC_COUNT + 2) {
		fprintf(stderr,
			"usage: %s OUTPUT_DIR FM_LAB.so FM_LIGHT.so FM_ULTRALIGHT.so DELAY.so "
			"DELAY_LIGHT.so FILTER.so AUTOPAN.so LOFI.so SCRATCH.so\n", argv[0]);
		return 2;
	}
	if (snprintf(song_path, sizeof(song_path), "%s/phase5-pooplog-family.psy", argv[1]) >=
			(int)sizeof(song_path)) {
		return fail_spec(NULL, "song output path is too long");
	}
	for (i = 0; i < SPEC_COUNT; ++i) {
		if (snprintf(preset_paths[i], sizeof(preset_paths[i]),
				"%s/phase5-pooplog-%lu.prs", argv[1], (unsigned long)i) >=
				(int)sizeof(preset_paths[i])) {
			return fail_spec(&SPECS[i], "preset output path is too long");
		}
		snapshot_init(&expected[i]);
	}

	rc = 0;
	psy_audio_init();
	psy_audio_machinecallback_init(&callback);
	psy_audio_plugincatcher_init(&catcher, NULL);
	for (i = 0; i < SPEC_COUNT; ++i) {
		if (register_native_plugin(&SPECS[i], &catcher, argv[i + 2]) != 0) {
			rc = 1;
			goto initial_cleanup;
		}
	}
	psy_audio_machinefactory_init(&factory, &callback, &catcher, NULL);
	psy_audio_machinefactory_createwithoutproxy(&factory);

	song = psy_audio_song_alloc_init(&factory);
	if (!song) {
		rc = fail_spec(NULL, "could not allocate family song");
		goto factory_cleanup;
	}
	psy_audio_machinecallback_set_song(&callback, song);

	for (i = 0; i < SPEC_COUNT; ++i) {
		psy_audio_Machine* machine;
		uintptr_t slot = FIRST_SLOT + i;

		machine = make_machine(&SPECS[i], &factory);
		if (!machine) {
			rc = fail_spec(&SPECS[i], "MachineFactory could not create machine");
			goto song_cleanup;
		}
		psy_audio_machines_insert(psy_audio_song_machines(song), slot, machine);
		psy_audio_machines_connect(psy_audio_song_machines(song),
			psy_audio_wire_make(slot, psy_audio_MASTER_INDEX));

		if ((rc = verify_identity(&SPECS[i], machine)) != 0 ||
				(rc = exercise_all_parameters(&SPECS[i], machine)) != 0 ||
				(rc = snapshot_from_machine(&SPECS[i], machine, &expected[i])) != 0) {
			goto song_cleanup;
		}
		if (SPECS[i].expects_opaque_state && expected[i].data_size == 0) {
			rc = fail_spec(&SPECS[i], "captured FM preset lost opaque state");
			goto song_cleanup;
		}
		if (!SPECS[i].expects_opaque_state && expected[i].data_size != 0) {
			rc = fail_spec(&SPECS[i], "effect captured unexpected opaque state");
			goto song_cleanup;
		}
		if ((rc = exercise_preset_roundtrip(&SPECS[i], &factory, machine,
				&expected[i], preset_paths[i])) != 0) {
			goto song_cleanup;
		}
		printf("phase5-pooplog-family-state: preset PASS [%s], opaque=%lu bytes\n",
			SPECS[i].label, (unsigned long)expected[i].data_size);
	}

	if (save_song(song, song_path) != 0) {
		rc = fail_spec(NULL, "family PSY3 save failed");
		goto song_cleanup;
	}

	psy_audio_machinecallback_init(&loaded_callback);
	psy_audio_plugincatcher_init(&loaded_catcher, NULL);
	for (i = 0; i < SPEC_COUNT; ++i) {
		if (register_native_plugin(&SPECS[i], &loaded_catcher, argv[i + 2]) != 0) {
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
		rc = fail_spec(NULL, "fresh PSY3 reload failed");
	} else {
		psy_audio_machinecallback_set_song(&loaded_callback, loaded);
		for (i = 0; i < SPEC_COUNT && rc == 0; ++i) {
			psy_audio_Machine* machine;
			Snapshot actual;
			uintptr_t slot = FIRST_SLOT + i;

			snapshot_init(&actual);
			machine = psy_audio_machines_at(psy_audio_song_machines(loaded), slot);
			if ((rc = verify_identity(&SPECS[i], machine)) == 0 &&
					(rc = snapshot_from_machine(&SPECS[i], machine, &actual)) == 0) {
				rc = snapshot_equal(&SPECS[i], &expected[i], &actual, "PSY3 reopen");
			}
			if (rc == 0 && !psy_audio_machines_connected(psy_audio_song_machines(loaded),
					psy_audio_wire_make(slot, psy_audio_MASTER_INDEX))) {
				rc = fail_spec(&SPECS[i], "wire to Master did not survive PSY3 reload");
			}
			snapshot_dispose(&actual);
		}
		psy_audio_song_deallocate(loaded);
	}
	psy_audio_machinefactory_dispose(&loaded_factory);
	psy_audio_plugincatcher_dispose(&loaded_catcher);

song_cleanup:
	psy_audio_song_deallocate(song);
factory_cleanup:
	psy_audio_machinefactory_dispose(&factory);
initial_cleanup:
	psy_audio_plugincatcher_dispose(&catcher);
	psy_audio_dispose();
	for (i = 0; i < SPEC_COUNT; ++i) snapshot_dispose(&expected[i]);

	if (rc != 0) return rc;
	printf("phase5-pooplog-family-state: PASS all %u source-built Pooplog machines\n",
		(unsigned int)SPEC_COUNT);
	printf("song: %s\n", song_path);
	return 0;
}
