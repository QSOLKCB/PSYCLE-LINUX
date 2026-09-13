/*
** PSYCLE-LINUX Phase 5B Pooplog family production persistence regression.
**
** Every retained source-built Pooplog machine is registered through the real
** PluginCatcher/MachineFactory path. The test first proves public endpoint tweaks
** actually take effect, then canonicalizes selector-driven FM state through its
** historical opaque PutData/GetData representation before using that state as
** the preset and PSY3 persistence oracle.
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

typedef struct PooplogSpec {
	const char* label;
	const char* catcher_name;
	const char* expected_name;
	const char* expected_shortname;
	const char* module_token;
	uintptr_t parameter_count;
	uintptr_t expected_opaque_size;
	uint64_t expected_opaque_hash;
} PooplogSpec;

/* Historical opaque sizes and the hashes of a canonical endpoint fixture are
** frozen independently of the runtime round-trip oracle. A layout/field-order
** change therefore fails even when the changed binary can read its own output. */
static const PooplogSpec SPECS[SPEC_COUNT] = {
	{"FM Laboratory", "pooplog-fm-laboratory:0", "Pooplog FM Laboratory0.68b",
		"Pooplog", "pooplog-fm-laboratory", 101u, 1508u,
		UINT64_C(0x89f6ae2978a8997a)},
	{"FM Light", "pooplog-fm-light:0", "Pooplog FM Light0.68b",
		"Pooplog Light", "pooplog-fm-light", 57u, 468u,
		UINT64_C(0xef4c752ed666c783)},
	{"FM UltraLight", "pooplog-fm-ultralight:0", "Pooplog FM UltraLight0.68b",
		"Pooplog UltraL", "pooplog-fm-ultralight", 45u, 344u,
		UINT64_C(0xd7fee89fd7e1bcb6)},
	{"Delay", "pooplog-delay:0", "Pooplog Delay 0.04b",
		"Pooplog Delay", "pooplog-delay", 43u, 0u, UINT64_C(0)},
	{"Delay Light", "pooplog-delay-light:0", "Pooplog Delay Light 0.04b",
		"Pooplog Delay L", "pooplog-delay-light", 28u, 0u, UINT64_C(0)},
	{"Filter", "pooplog-filter:0", "Pooplog Filter 0.06b",
		"Pooplog Filter", "pooplog-filter", 16u, 0u, UINT64_C(0)},
	{"Autopan", "pooplog-autopan:0", "Pooplog Autopan 0.06b",
		"Pooplog Autopan", "pooplog-autopan", 9u, 0u, UINT64_C(0)},
	{"Lofi", "pooplog-lofi-processor:0", "Pooplog Lofi Processor 0.04b",
		"Pooplog Lofi", "pooplog-lofi-processor", 4u, 0u, UINT64_C(0)},
	{"Scratch", "pooplog-scratch-master:0", "Pooplog Scratch Master 0.06b",
		"Pooplog Scratch", "pooplog-scratch-master", 8u, 0u, UINT64_C(0)},
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

static int snapshot_from_preset(const PooplogSpec* spec,
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
		if (!preset->data) {
			snapshot_dispose(out);
			return fail_spec(spec, "opaque size is nonzero but data pointer is null");
		}
		out->data = (unsigned char*)malloc(out->data_size);
		if (!out->data) {
			snapshot_dispose(out);
			return fail_spec(spec, "opaque snapshot allocation failed");
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
		fprintf(stderr, "phase5-pooplog-family-state: FAIL [%s]: %s parameter count changed\n",
			spec->label, context);
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
			"phase5-pooplog-family-state: FAIL [%s]: %s opaque size expected %lu got %lu\n",
			spec->label, context, (unsigned long)expected->data_size,
			(unsigned long)actual->data_size);
		return 1;
	}
	if (expected->data_size > 0) {
		if (!expected->data || !actual->data ||
				memcmp(expected->data, actual->data, expected->data_size) != 0) {
			uintptr_t offset;
			for (offset = 0; offset < expected->data_size; ++offset) {
				if (!expected->data || !actual->data ||
						expected->data[offset] != actual->data[offset]) break;
			}
			fprintf(stderr,
				"phase5-pooplog-family-state: FAIL [%s]: %s opaque bytes differ at %lu\n",
				spec->label, context, (unsigned long)offset);
			return 1;
		}
	}
	return 0;
}

static int verify_identity(const PooplogSpec* spec, psy_audio_Machine* machine)
{
	const psy_audio_MachineInfo* info;
	uintptr_t data_size;
	if (!machine || psy_audio_machine_type(machine) != psy_audio_PLUGIN)
		return fail_spec(spec, "machine is missing or no longer a native plugin");
	info = psy_audio_machine_info(machine);
	if (!info || !info->name || strcmp(info->name, spec->expected_name) != 0 ||
			!info->shortname || strcmp(info->shortname, spec->expected_shortname) != 0 ||
			!info->author || strcmp(info->author, "Jeremy Evers") != 0 ||
			!info->modulepath || !strstr(info->modulepath, spec->module_token))
		return fail_spec(spec, "identity/module path changed");
	if (psy_audio_machine_num_parameters(machine) != spec->parameter_count)
		return fail_spec(spec, "production parameter count changed");
	data_size = psy_audio_machine_data_size(machine);
	if (data_size != spec->expected_opaque_size) {
		fprintf(stderr,
			"phase5-pooplog-family-state: FAIL [%s]: opaque size expected %lu got %lu\n",
			spec->label, (unsigned long)spec->expected_opaque_size,
			(unsigned long)data_size);
		return 1;
	}
	return 0;
}

static int register_native(const PooplogSpec* spec,
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
				"phase5-pooplog-family-state: FAIL [%s]: catcher expected %s got %s\n",
				spec->label, spec->catcher_name, catcher_name);
			rc = 1;
		} else {
			psy_audio_plugins_add(&catcher->plugins_, &info);
			if (!psy_audio_plugins_at_id_const(&catcher->plugins_, catcher_name))
				rc = fail_spec(spec, "PluginCatcher registration failed");
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

static int exercise_public_endpoints(const PooplogSpec* spec,
	psy_audio_Machine* machine)
{
	uintptr_t i;
	for (i = 0; i < spec->parameter_count; ++i) {
		psy_audio_MachineParam* param = psy_audio_machine_parameter(machine, i);
		intptr_t minval;
		intptr_t maxval;
		intptr_t requested;
		intptr_t actual;
		if (!param) return fail_spec(spec, "production parameter surface is incomplete");
		psy_audio_machine_parameter_range(machine, param, &minval, &maxval);
		if (minval > maxval) return fail_spec(spec, "production parameter range is inverted");
		requested = (i & 1u) ? maxval : minval;
		psy_audio_machine_parameter_tweak_scaled(machine, param, requested);
		actual = psy_audio_machine_parameter_scaled_value(machine, param);
		if (actual != requested) {
			fprintf(stderr,
				"phase5-pooplog-family-state: FAIL [%s]: endpoint parameter %lu requested %ld got %ld\n",
				spec->label, (unsigned long)i, (long)requested, (long)actual);
			return 1;
		}
	}
	printf("phase5-pooplog-family-state: endpoint PASS [%s]\n", spec->label);
	return 0;
}

static int canonicalize_for_persistence(const PooplogSpec* spec,
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
			return fail_spec(spec, "FM synth captured unexpected opaque-state geometry");
		}
		/* Selector-driven Pooplog parameters include wrapped/canonical forms in
		** the opaque structure (for example OSC phase). Re-applying the machine's
		** own opaque state refreshes Vals exactly as a real preset/song load does;
		** that canonical post-load state is the correct persistence oracle. */
		psy_audio_machine_put_data(machine, raw.data);
	}
	snapshot_dispose(&raw);
	if ((rc = snapshot_from_machine(spec, machine, expected)) != 0) return rc;
	if (spec->expected_opaque_size > 0) {
		hash = opaque_hash(expected->data, expected->data_size);
		printf("pooplog-opaque-hash[%s]=0x%016llx size=%lu\n", spec->label,
			(unsigned long long)hash, (unsigned long)expected->data_size);
		if (hash != spec->expected_opaque_hash) {
			fprintf(stderr,
				"phase5-pooplog-family-state: FAIL [%s]: canonical opaque hash "
				"expected 0x%016llx got 0x%016llx\n", spec->label,
				(unsigned long long)spec->expected_opaque_hash,
				(unsigned long long)hash);
			return 1;
		}
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
	psy_audio_Machine* fresh = NULL;
	Snapshot loaded_snapshot;
	Snapshot fresh_snapshot;
	char name[256];
	int status;
	int rc = 0;

	psy_audio_presets_init(&saved);
	psy_audio_presets_init(&loaded);
	snapshot_init(&loaded_snapshot);
	snapshot_init(&fresh_snapshot);
	captured = psy_audio_preset_alloc_init();
	if (!captured) { rc = fail_spec(spec, "preset allocation failed"); goto cleanup; }
	psy_audio_machine_current_preset(source, captured);
	if (snprintf(name, sizeof(name), "Phase 5B %s state", spec->label) >= (int)sizeof(name)) {
		psy_audio_preset_dispose(captured); free(captured);
		rc = fail_spec(spec, "preset name overflow"); goto cleanup;
	}
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
	if (!reloaded || strcmp(psy_audio_preset_name(reloaded), name) != 0) {
		rc = fail_spec(spec, "preset identity changed"); goto cleanup;
	}
	if ((rc = snapshot_from_preset(spec, reloaded, &loaded_snapshot)) != 0 ||
			(rc = snapshot_equal(spec, expected, &loaded_snapshot, "preset reload")) != 0)
		goto cleanup;

	fresh = make_machine(spec, factory);
	if (!fresh) { rc = fail_spec(spec, "fresh preset machine creation failed"); goto cleanup; }
	psy_audio_machine_tweak_preset(fresh, reloaded);
	if ((rc = verify_identity(spec, fresh)) == 0 &&
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
	psy_audio_MachineCallback callback;
	psy_audio_PluginCatcher catcher;
	psy_audio_MachineFactory factory;
	psy_audio_Song* song = NULL;
	Snapshot expected[SPEC_COUNT];
	psy_audio_MachineCallback loaded_callback;
	psy_audio_PluginCatcher loaded_catcher;
	psy_audio_MachineFactory loaded_factory;
	psy_audio_Song* loaded = NULL;
	uintptr_t i;
	int rc = 0;

	if (argc != (int)SPEC_COUNT + 2) {
		fprintf(stderr, "usage: %s OUT FM_LAB.so FM_LIGHT.so FM_ULTRALIGHT.so DELAY.so DELAY_LIGHT.so FILTER.so AUTOPAN.so LOFI.so SCRATCH.so\n", argv[0]);
		return 2;
	}
	if (snprintf(song_path, sizeof(song_path), "%s/phase5-pooplog-family.psy", argv[1]) >= (int)sizeof(song_path))
		return fail_spec(NULL, "song path too long");
	for (i = 0; i < SPEC_COUNT; ++i) {
		snapshot_init(&expected[i]);
		if (snprintf(preset_paths[i], sizeof(preset_paths[i]), "%s/phase5-pooplog-%lu.prs",
				argv[1], (unsigned long)i) >= (int)sizeof(preset_paths[i]))
			return fail_spec(&SPECS[i], "preset path too long");
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
				(rc = canonicalize_for_persistence(&SPECS[i], machine, &expected[i])) != 0 ||
				(rc = exercise_preset_roundtrip(&SPECS[i], &factory, machine,
					&expected[i], preset_paths[i])) != 0)
			goto song_cleanup;
		printf("phase5-pooplog-family-state: preset PASS [%s], opaque=%lu bytes\n",
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
	printf("phase5-pooplog-family-state: PASS all %u source-built Pooplog machines\n",
		(unsigned int)SPEC_COUNT);
	printf("song: %s\n", song_path);
	return 0;
}
