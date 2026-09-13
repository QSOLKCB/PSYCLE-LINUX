/*
** PSYCLE-LINUX Phase 5C Druttis family production persistence regression.
**
** Registers every retained Druttis machine through PluginCatcher/MachineFactory,
** verifies public parameter endpoints, preset round-trips and a fresh PSY3 reopen.
** Slicit's historical 16-program opaque bank is seeded with hidden program state
** so successful restoration depends on PutData rather than the visible parameters.
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

#define SPEC_COUNT 7u

typedef struct DruttisSpec {
	const char* label;
	const char* catcher_name;
	const char* expected_name;
	const char* expected_shortname;
	const char* module_token;
	uintptr_t parameter_count;
	uintptr_t expected_opaque_size;
	uint64_t expected_opaque_hash;
} DruttisSpec;

/* Slicit PROG is 134 bytes (3 shorts + 16 * 4 shorts); sixteen programs are
** therefore 2144 bytes. The hash is filled after the observation CI run. */
static const DruttisSpec SPECS[SPEC_COUNT] = {
	{"EQ-3", "eq3:0", "EQ-3", "EQ-3", "eq3", 12u, 0u, UINT64_C(0)},
	{"FeedMe", "feedme:0", "FeedMe 1.2", "FeedMe", "feedme", 24u, 0u, UINT64_C(0)},
	{"Koruz", "koruz:0", "Koruz", "Koruz", "koruz", 14u, 0u, UINT64_C(0)},
	{"Phantom", "phantom:0", "Phantom 1.2", "Phantom", "phantom", 55u, 0u, UINT64_C(0)},
	{"Plucked String", "pluckedstring:0", "Plucked String 1.2", "Plucked String", "pluckedstring", 7u, 0u, UINT64_C(0)},
	{"Slicit", "slicit:0", "Slicit", "Slicit", "slicit", 68u, 2144u, UINT64_C(0)},
	{"Sublime", "sublime:0", "Sublime 1.1", "Sublime", "sublime", 60u, 0u, UINT64_C(0)},
};

typedef struct Snapshot {
	uintptr_t parameter_count;
	intptr_t* parameters;
	uintptr_t data_size;
	unsigned char* data;
} Snapshot;

static int fail_spec(const DruttisSpec* spec, const char* message)
{
	fprintf(stderr, "phase5-druttis-family-state: FAIL [%s]: %s\n",
		spec ? spec->label : "family", message);
	return 1;
}

static uint64_t opaque_hash(const unsigned char* data, uintptr_t size)
{
	uint64_t hash = UINT64_C(1469598103934665603);
	uintptr_t i;
	for (i = 0; i < size; ++i) {
		hash ^= (uint64_t)data[i];
		hash *= UINT64_C(1099511628211);
	}
	return hash;
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

static int snapshot_from_preset(const DruttisSpec* spec,
	psy_audio_Preset* preset, Snapshot* out)
{
	uintptr_t i;
	snapshot_dispose(out);
	out->parameter_count = psy_audio_preset_num_parameters(preset);
	if (out->parameter_count != spec->parameter_count)
		return fail_spec(spec, "captured preset parameter count changed");
	if (out->parameter_count > 0) {
		out->parameters = (intptr_t*)malloc(out->parameter_count * sizeof(intptr_t));
		if (!out->parameters) return fail_spec(spec, "parameter snapshot allocation failed");
		for (i = 0; i < out->parameter_count; ++i)
			out->parameters[i] = psy_audio_preset_value(preset, i);
	}
	out->data_size = preset->datasize;
	if (out->data_size > 0) {
		if (!preset->data) return fail_spec(spec, "opaque size is nonzero but data pointer is null");
		out->data = (unsigned char*)malloc(out->data_size);
		if (!out->data) return fail_spec(spec, "opaque snapshot allocation failed");
		memcpy(out->data, preset->data, out->data_size);
	}
	return 0;
}

static int snapshot_from_machine(const DruttisSpec* spec,
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

static int snapshot_equal(const DruttisSpec* spec, const Snapshot* expected,
	const Snapshot* actual, const char* context)
{
	uintptr_t i;
	if (expected->parameter_count != actual->parameter_count)
		return fail_spec(spec, "persistence parameter count changed");
	for (i = 0; i < expected->parameter_count; ++i) {
		if (expected->parameters[i] != actual->parameters[i]) {
			fprintf(stderr,
				"phase5-druttis-family-state: FAIL [%s]: %s parameter %lu expected %ld got %ld\n",
				spec->label, context, (unsigned long)i,
				(long)expected->parameters[i], (long)actual->parameters[i]);
			return 1;
		}
	}
	if (expected->data_size != actual->data_size)
		return fail_spec(spec, "persistence opaque size changed");
	if (expected->data_size > 0 && (!expected->data || !actual->data ||
			memcmp(expected->data, actual->data, expected->data_size) != 0))
		return fail_spec(spec, "persistence opaque bytes changed");
	return 0;
}

static int author_is_druttis(const char* author)
{
	return author && strncmp(author, "Druttis on ", 11) == 0;
}

static int verify_identity(const DruttisSpec* spec, psy_audio_Machine* machine)
{
	const psy_audio_MachineInfo* info;
	uintptr_t data_size;
	if (!machine || psy_audio_machine_type(machine) != psy_audio_PLUGIN)
		return fail_spec(spec, "machine is missing or no longer a native plugin");
	info = psy_audio_machine_info(machine);
	if (!info || !info->name || strcmp(info->name, spec->expected_name) != 0 ||
			!info->shortname || strcmp(info->shortname, spec->expected_shortname) != 0 ||
			!author_is_druttis(info->author) || !info->modulepath ||
			!strstr(info->modulepath, spec->module_token))
		return fail_spec(spec, "identity/module path changed");
	if (psy_audio_machine_num_parameters(machine) != spec->parameter_count)
		return fail_spec(spec, "production parameter count changed");
	data_size = psy_audio_machine_data_size(machine);
	if (data_size != spec->expected_opaque_size) {
		fprintf(stderr,
			"phase5-druttis-family-state: FAIL [%s]: opaque size expected %lu got %lu\n",
			spec->label, (unsigned long)spec->expected_opaque_size, (unsigned long)data_size);
		return 1;
	}
	return 0;
}

static int register_native(const DruttisSpec* spec,
	psy_audio_PluginCatcher* catcher, const char* path)
{
	psy_audio_MachineInfo info;
	char catcher_name[256];
	int rc = 0;
	machineinfo_init(&info);
	if (!psy_audio_plugin_psycle_test(path, catcher->native_root_, &info)) {
		rc = fail_spec(spec, "production native-plugin probe failed");
	} else if (info.type != psy_audio_PLUGIN || !info.name ||
			strcmp(info.name, spec->expected_name) != 0) {
		rc = fail_spec(spec, "production native-plugin probe returned unexpected metadata");
	} else {
		machineinfo_catchername(&info, catcher_name);
		if (strcmp(catcher_name, spec->catcher_name) != 0) {
			fprintf(stderr,
				"phase5-druttis-family-state: FAIL [%s]: catcher expected %s got %s\n",
				spec->label, spec->catcher_name, catcher_name);
			rc = 1;
		} else {
			psy_audio_plugins_add(&catcher->plugins_, &info);
		}
	}
	machineinfo_dispose(&info);
	return rc;
}

static psy_audio_Machine* make_machine(const DruttisSpec* spec,
	psy_audio_MachineFactory* factory)
{
	return psy_audio_machinefactory_make_machine(factory, psy_audio_PLUGIN,
		spec->catcher_name, psy_INDEX_INVALID);
}

static psy_audio_MachineParam* find_named_parameter(psy_audio_Machine* machine,
	const char* name)
{
	uintptr_t i;
	char text[128];
	for (i = 0; i < psy_audio_machine_num_parameters(machine); ++i) {
		psy_audio_MachineParam* param = psy_audio_machine_parameter(machine, i);
		if (!param) continue;
		text[0] = '\0';
		if (psy_audio_machine_parameter_name(machine, param, text) && strcmp(text, name) == 0)
			return param;
	}
	return NULL;
}

static int tweak_named(const DruttisSpec* spec, psy_audio_Machine* machine,
	const char* name, intptr_t value)
{
	psy_audio_MachineParam* param = find_named_parameter(machine, name);
	intptr_t minval, maxval, actual;
	if (!param) return fail_spec(spec, "named Slicit parameter disappeared");
	psy_audio_machine_parameter_range(machine, param, &minval, &maxval);
	if (value < minval || value > maxval) return fail_spec(spec, "Slicit seed out of range");
	psy_audio_machine_parameter_tweak_scaled(machine, param, value);
	actual = psy_audio_machine_parameter_scaled_value(machine, param);
	if (actual != value) return fail_spec(spec, "Slicit seed did not apply exactly");
	return 0;
}

static int read_named(const DruttisSpec* spec, psy_audio_Machine* machine,
	const char* name, intptr_t expected, const char* context)
{
	psy_audio_MachineParam* param = find_named_parameter(machine, name);
	intptr_t actual;
	if (!param) return fail_spec(spec, "named Slicit restore parameter disappeared");
	actual = psy_audio_machine_parameter_scaled_value(machine, param);
	if (actual != expected) {
		fprintf(stderr,
			"phase5-druttis-family-state: FAIL [%s]: %s %s expected %ld got %ld\n",
			spec->label, context, name, (long)expected, (long)actual);
		return 1;
	}
	return 0;
}

static int exercise_public_endpoints(const DruttisSpec* spec,
	psy_audio_Machine* machine)
{
	uintptr_t i;
	for (i = 0; i < spec->parameter_count; ++i) {
		psy_audio_MachineParam* param = psy_audio_machine_parameter(machine, i);
		intptr_t minval, maxval, requested, actual;
		if (!param) return fail_spec(spec, "production parameter surface is incomplete");
		psy_audio_machine_parameter_range(machine, param, &minval, &maxval);
		requested = (i & 1u) ? maxval : minval;
		psy_audio_machine_parameter_tweak_scaled(machine, param, requested);
		actual = psy_audio_machine_parameter_scaled_value(machine, param);
		if (actual != requested) {
			fprintf(stderr,
				"phase5-druttis-family-state: FAIL [%s]: endpoint %lu requested %ld got %ld\n",
				spec->label, (unsigned long)i, (long)requested, (long)actual);
			return 1;
		}
	}
	printf("phase5-druttis-family-state: endpoint PASS [%s]\n", spec->label);
	return 0;
}

static int seed_slicit_hidden_program(const DruttisSpec* spec,
	psy_audio_Machine* machine)
{
	if (spec->expected_opaque_size == 0) return 0;
	if (tweak_named(spec, machine, "Program Nr.", 1) != 0 ||
			tweak_named(spec, machine, "No. Steps", 7) != 0 ||
			tweak_named(spec, machine, "Speed Factor", 3) != 0 ||
			tweak_named(spec, machine, "Filter Type", 2) != 0 ||
			tweak_named(spec, machine, "Level 1", 73) != 0 ||
			tweak_named(spec, machine, "Attack 1", 91) != 0 ||
			tweak_named(spec, machine, "Pan 1", 44) != 0 ||
			tweak_named(spec, machine, "F.Freq 1", 211) != 0 ||
			tweak_named(spec, machine, "Program Nr.", 0) != 0 ||
			tweak_named(spec, machine, "No. Steps", 3) != 0 ||
			tweak_named(spec, machine, "Speed Factor", 0) != 0 ||
			tweak_named(spec, machine, "Filter Type", 0) != 0 ||
			tweak_named(spec, machine, "Level 1", 17) != 0 ||
			tweak_named(spec, machine, "Attack 1", 23) != 0 ||
			tweak_named(spec, machine, "Pan 1", 199) != 0 ||
			tweak_named(spec, machine, "F.Freq 1", 31) != 0)
		return 1;
	printf("phase5-druttis-family-state: opaque-only program seed PASS [Slicit]\n");
	return 0;
}

static int verify_slicit_hidden_program(const DruttisSpec* spec,
	psy_audio_Machine* machine, const char* context)
{
	int rc = 0;
	if (spec->expected_opaque_size == 0) return 0;
	if (tweak_named(spec, machine, "Program Nr.", 1) != 0 ||
			read_named(spec, machine, "No. Steps", 7, context) != 0 ||
			read_named(spec, machine, "Speed Factor", 3, context) != 0 ||
			read_named(spec, machine, "Filter Type", 2, context) != 0 ||
			read_named(spec, machine, "Level 1", 73, context) != 0 ||
			read_named(spec, machine, "Attack 1", 91, context) != 0 ||
			read_named(spec, machine, "Pan 1", 44, context) != 0 ||
			read_named(spec, machine, "F.Freq 1", 211, context) != 0)
		rc = 1;
	if (tweak_named(spec, machine, "Program Nr.", 0) != 0) rc = 1;
	if (rc == 0)
		printf("phase5-druttis-family-state: opaque-only program restore PASS [Slicit] %s\n",
			context);
	return rc;
}

static int canonicalize_for_persistence(const DruttisSpec* spec,
	psy_audio_Machine* machine, Snapshot* expected)
{
	Snapshot raw;
	uint64_t hash;
	int rc;
	snapshot_init(&raw);
	if ((rc = snapshot_from_machine(spec, machine, &raw)) != 0) return rc;
	if (spec->expected_opaque_size > 0) {
		if (raw.data_size != spec->expected_opaque_size || !raw.data) {
			snapshot_dispose(&raw);
			return fail_spec(spec, "Slicit opaque-state geometry changed");
		}
		psy_audio_machine_put_data(machine, raw.data);
	}
	snapshot_dispose(&raw);
	if ((rc = verify_slicit_hidden_program(spec, machine, "source canonicalization")) != 0)
		return rc;
	if ((rc = snapshot_from_machine(spec, machine, expected)) != 0) return rc;
	if (spec->expected_opaque_size > 0) {
		hash = opaque_hash(expected->data, expected->data_size);
		printf("druttis-opaque-hash[Slicit]=0x%016llx size=%lu\n",
			(unsigned long long)hash, (unsigned long)expected->data_size);
		if (spec->expected_opaque_hash != 0 && hash != spec->expected_opaque_hash)
			return fail_spec(spec, "frozen Slicit opaque hash changed");
	}
	return 0;
}

static int exercise_preset_roundtrip(const DruttisSpec* spec,
	psy_audio_MachineFactory* factory, psy_audio_Machine* source,
	const Snapshot* expected, const char* path)
{
	psy_audio_Presets saved, loaded;
	psy_audio_Preset* captured;
	psy_audio_Preset* reloaded;
	psy_audio_Machine* fresh = NULL;
	Snapshot loaded_snapshot, fresh_snapshot;
	char name[256];
	int status, rc = 0;
	psy_audio_presets_init(&saved);
	psy_audio_presets_init(&loaded);
	snapshot_init(&loaded_snapshot);
	snapshot_init(&fresh_snapshot);
	captured = psy_audio_preset_alloc_init();
	if (!captured) { rc = fail_spec(spec, "preset allocation failed"); goto cleanup; }
	psy_audio_machine_current_preset(source, captured);
	snprintf(name, sizeof(name), "Phase 5C Druttis %s state", spec->label);
	psy_audio_preset_set_name(captured, name);
	psy_audio_presets_insert(&saved, 0, captured);
	status = psy_audio_presetsio_save(path, &saved);
	if (status != psy_audio_PRESETIO_OK) { rc = fail_spec(spec, "preset save failed"); goto cleanup; }
	status = psy_audio_presetsio_load(path, &loaded, expected->parameter_count,
		expected->data_size, "");
	if (status != psy_audio_PRESETIO_OK || psy_audio_presets_size(&loaded) != 1) {
		rc = fail_spec(spec, "preset reload failed"); goto cleanup;
	}
	reloaded = psy_audio_presets_at(&loaded, 0);
	if ((rc = snapshot_from_preset(spec, reloaded, &loaded_snapshot)) != 0 ||
			(rc = snapshot_equal(spec, expected, &loaded_snapshot, "preset reload")) != 0)
		goto cleanup;
	fresh = make_machine(spec, factory);
	if (!fresh) { rc = fail_spec(spec, "fresh preset machine creation failed"); goto cleanup; }
	psy_audio_machine_tweak_preset(fresh, reloaded);
	if ((rc = verify_identity(spec, fresh)) == 0 &&
			(rc = verify_slicit_hidden_program(spec, fresh, "fresh preset restore")) == 0 &&
			(rc = snapshot_from_machine(spec, fresh, &fresh_snapshot)) == 0)
		rc = snapshot_equal(spec, expected, &fresh_snapshot, "fresh preset restore");
cleanup:
	if (fresh) psy_audio_machine_deallocate(fresh);
	snapshot_dispose(&loaded_snapshot);
	snapshot_dispose(&fresh_snapshot);
	psy_audio_presets_dispose(&saved);
	psy_audio_presets_dispose(&loaded);
	return rc;
}

static int save_song(psy_audio_Song* song, const char* path)
{
	psy_audio_SongFile sf;
	int status;
	psy_audio_songfile_init_song(&sf, song);
	status = psy_audio_songfile_save(&sf, path);
	psy_audio_songfile_dispose(&sf);
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
	if (status != PSY_OK) { psy_audio_song_deallocate(song); return NULL; }
	return song;
}

int main(int argc, char** argv)
{
	char song_path[4096];
	char preset_paths[SPEC_COUNT][4096];
	psy_audio_MachineCallback callback, loaded_callback;
	psy_audio_PluginCatcher catcher, loaded_catcher;
	psy_audio_MachineFactory factory, loaded_factory;
	psy_audio_Song* song = NULL;
	psy_audio_Song* loaded = NULL;
	Snapshot expected[SPEC_COUNT];
	uintptr_t i;
	int rc = 0;

	if (argc != (int)SPEC_COUNT + 2) {
		fprintf(stderr, "usage: %s OUT EQ3.so FEEDME.so KORUZ.so PHANTOM.so PLUCKED.so SLICIT.so SUBLIME.so\n", argv[0]);
		return 2;
	}
	if (snprintf(song_path, sizeof(song_path), "%s/phase5-druttis-family.psy", argv[1]) >= (int)sizeof(song_path))
		return fail_spec(NULL, "song path too long");
	for (i = 0; i < SPEC_COUNT; ++i) {
		snapshot_init(&expected[i]);
		snprintf(preset_paths[i], sizeof(preset_paths[i]), "%s/phase5-druttis-%lu.prs",
			argv[1], (unsigned long)i);
	}

	psy_audio_init();
	psy_audio_machinecallback_init(&callback);
	psy_audio_plugincatcher_init(&catcher, NULL);
	for (i = 0; i < SPEC_COUNT; ++i) {
		if (register_native(&SPECS[i], &catcher, argv[i + 2]) != 0) { rc = 1; goto initial_cleanup; }
	}
	psy_audio_machinefactory_init(&factory, &callback, &catcher, NULL);
	psy_audio_machinefactory_createwithoutproxy(&factory);
	song = psy_audio_song_alloc_init(&factory);
	if (!song) { rc = fail_spec(NULL, "family song allocation failed"); goto factory_cleanup; }
	psy_audio_machinecallback_set_song(&callback, song);

	for (i = 0; i < SPEC_COUNT; ++i) {
		psy_audio_Machine* machine = make_machine(&SPECS[i], &factory);
		if (!machine) { rc = fail_spec(&SPECS[i], "MachineFactory creation failed"); goto song_cleanup; }
		psy_audio_machines_insert(psy_audio_song_machines(song), i, machine);
		psy_audio_machines_connect(psy_audio_song_machines(song),
			psy_audio_wire_make(i, psy_audio_MASTER_INDEX));
		if ((rc = verify_identity(&SPECS[i], machine)) != 0 ||
				(rc = exercise_public_endpoints(&SPECS[i], machine)) != 0 ||
				(rc = seed_slicit_hidden_program(&SPECS[i], machine)) != 0 ||
				(rc = canonicalize_for_persistence(&SPECS[i], machine, &expected[i])) != 0 ||
				(rc = exercise_preset_roundtrip(&SPECS[i], &factory, machine,
					&expected[i], preset_paths[i])) != 0)
			goto song_cleanup;
		printf("phase5-druttis-family-state: preset PASS [%s], opaque=%lu bytes\n",
			SPECS[i].label, (unsigned long)expected[i].data_size);
	}
	if (save_song(song, song_path) != 0) { rc = fail_spec(NULL, "family PSY3 save failed"); goto song_cleanup; }

	psy_audio_machinecallback_init(&loaded_callback);
	psy_audio_plugincatcher_init(&loaded_catcher, NULL);
	for (i = 0; i < SPEC_COUNT; ++i) {
		if (register_native(&SPECS[i], &loaded_catcher, argv[i + 2]) != 0) {
			rc = 1; psy_audio_plugincatcher_dispose(&loaded_catcher); goto song_cleanup;
		}
	}
	psy_audio_machinefactory_init(&loaded_factory, &loaded_callback, &loaded_catcher, NULL);
	psy_audio_machinefactory_createwithoutproxy(&loaded_factory);
	loaded = load_song(&loaded_factory, song_path);
	if (!loaded) {
		rc = fail_spec(NULL, "fresh PSY3 reopen failed");
	} else {
		psy_audio_machinecallback_set_song(&loaded_callback, loaded);
		for (i = 0; i < SPEC_COUNT && rc == 0; ++i) {
			psy_audio_Machine* machine = psy_audio_machines_at(psy_audio_song_machines(loaded), i);
			Snapshot actual;
			snapshot_init(&actual);
			if ((rc = verify_identity(&SPECS[i], machine)) == 0 &&
					(rc = verify_slicit_hidden_program(&SPECS[i], machine, "PSY3 reopen")) == 0 &&
					(rc = snapshot_from_machine(&SPECS[i], machine, &actual)) == 0)
				rc = snapshot_equal(&SPECS[i], &expected[i], &actual, "PSY3 reopen");
			if (rc == 0 && !psy_audio_machines_connected(psy_audio_song_machines(loaded),
					psy_audio_wire_make(i, psy_audio_MASTER_INDEX)))
				rc = fail_spec(&SPECS[i], "wire to Master did not survive PSY3 reopen");
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
	printf("phase5-druttis-family-state: PASS all %u source-built Druttis machines\n",
		(unsigned int)SPEC_COUNT);
	printf("song: %s\n", song_path);
	return 0;
}
