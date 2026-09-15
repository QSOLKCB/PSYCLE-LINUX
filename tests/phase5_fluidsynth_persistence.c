/*
** PSYCLE-LINUX Phase 5C FluidSynth SF2 Player production persistence regression.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <buffer.h>
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

#define PARAMETER_COUNT 24u
#define DATA_SIZE 5184u
#define SLOT 0u
#define CATCHER_NAME "fluidsynth:0"
#define PRESET_NAME "Phase 5 FluidSynth"
#define MAX_INSTR 64
#define SF_PATH_SIZE 4096

struct InstrState {
	int bank;
	int prog;
	int pitch;
	int wheel;
};

struct FluidState {
	int version;
	char sf_path[SF_PATH_SIZE];
	struct InstrState instr[MAX_INSTR];
	int cur_channel;
	int reverb_on;
	int roomsize;
	int damping;
	int width;
	int reverb_level;
	int chorus_on;
	int chorus_nr;
	int chorus_level;
	int chorus_speed;
	int chorus_depth_ms;
	int chorus_type;
	int polyphony;
	int interpolation;
	int gain;
};

static int fail(const char* message)
{
	fprintf(stderr, "phase5-fluidsynth-state: FAIL: %s\n", message);
	return 1;
}

static void init_state(struct FluidState* state, const char* soundfont)
{
	int i;
	memset(state, 0, sizeof(*state));
	state->version = 3;
	snprintf(state->sf_path, sizeof(state->sf_path), "%s", soundfont);
	for (i = 0; i < MAX_INSTR; ++i) {
		state->instr[i].bank = 0;
		state->instr[i].prog = 0;
		state->instr[i].pitch = 8192;
		state->instr[i].wheel = 2;
	}
	state->instr[0].pitch = 9000;
	state->instr[0].wheel = 12;
	state->cur_channel = 0;
	state->reverb_on = 0;
	state->roomsize = 40;
	state->damping = 45;
	state->width = 80;
	state->reverb_level = 400;
	state->chorus_on = 0;
	state->chorus_nr = 4;
	state->chorus_level = 600;
	state->chorus_speed = 4;
	state->chorus_depth_ms = 10;
	state->chorus_type = 0;
	state->polyphony = 64;
	state->interpolation = 1;
	state->gain = 80;
}

static int state_matches(const struct FluidState* actual,
	const struct FluidState* expected)
{
	if (!actual || !expected) return 0;
	if (actual->version != 3 || strcmp(actual->sf_path, expected->sf_path) != 0 ||
			actual->cur_channel != expected->cur_channel ||
			actual->reverb_on != expected->reverb_on || actual->roomsize != expected->roomsize ||
			actual->damping != expected->damping || actual->width != expected->width ||
			actual->reverb_level != expected->reverb_level ||
			actual->chorus_on != expected->chorus_on || actual->chorus_nr != expected->chorus_nr ||
			actual->chorus_level != expected->chorus_level || actual->chorus_speed != expected->chorus_speed ||
			actual->chorus_depth_ms != expected->chorus_depth_ms || actual->chorus_type != expected->chorus_type ||
			actual->polyphony != expected->polyphony || actual->interpolation != expected->interpolation ||
			actual->gain != expected->gain)
		return 0;
	return actual->instr[0].pitch == expected->instr[0].pitch &&
		actual->instr[0].wheel == expected->instr[0].wheel;
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
			strcmp(info.name, "FluidSynth SF2 player") != 0 || !info.shortname ||
			strcmp(info.shortname, "FluidSynth") != 0 || info.plugversion != 0x0101) {
		rc = fail("production probe returned unexpected FluidSynth identity");
	} else {
		machineinfo_catchername(&info, catcher_name);
		if (strcmp(catcher_name, CATCHER_NAME) != 0) {
			fprintf(stderr, "phase5-fluidsynth-state: FAIL: catcher expected %s got %s\n",
				CATCHER_NAME, catcher_name);
			rc = 1;
		} else {
			psy_audio_plugins_add(&catcher->plugins_, &info);
			if (!psy_audio_plugins_at_id_const(&catcher->plugins_, catcher_name))
				rc = fail("PluginCatcher registration disappeared");
		}
	}
	machineinfo_dispose(&info);
	return rc;
}

static psy_audio_Machine* make_machine(psy_audio_MachineFactory* factory)
{
	return psy_audio_machinefactory_make_machine(factory, psy_audio_PLUGIN,
		CATCHER_NAME, psy_INDEX_INVALID);
}

static int verify_identity(psy_audio_Machine* machine)
{
	const psy_audio_MachineInfo* info;
	if (!machine || psy_audio_machine_type(machine) != psy_audio_PLUGIN)
		return fail("FluidSynth missing or no longer a native Psycle plugin");
	info = psy_audio_machine_info(machine);
	if (!info || !info->name || strcmp(info->name, "FluidSynth SF2 player") != 0 ||
			!info->shortname || strcmp(info->shortname, "FluidSynth") != 0 ||
			info->plugversion != 0x0101 || !info->modulepath ||
			!strstr(info->modulepath, "fluidsynth"))
		return fail("FluidSynth production identity/module path changed");
	if (psy_audio_machine_num_parameters(machine) != PARAMETER_COUNT)
		return fail("FluidSynth 24-slot production ABI changed");
	if (psy_audio_machine_data_size(machine) != DATA_SIZE)
		return fail("FluidSynth opaque SYNPAR size changed");
	return 0;
}

static int apply_state(psy_audio_Machine* machine, const struct FluidState* expected)
{
	psy_audio_Preset captured;
	const struct FluidState* actual;
	if (verify_identity(machine) != 0) return 1;
	psy_audio_machine_put_data(machine, (uint8_t*)expected);
	psy_audio_preset_init(&captured);
	psy_audio_machine_current_preset(machine, &captured);
	if (psy_audio_preset_num_parameters(&captured) != PARAMETER_COUNT ||
			captured.datasize != DATA_SIZE || !captured.data) {
		psy_audio_preset_dispose(&captured);
		return fail("captured FluidSynth preset geometry changed");
	}
	actual = (const struct FluidState*)captured.data;
	if (!state_matches(actual, expected)) {
		psy_audio_preset_dispose(&captured);
		return fail("FluidSynth opaque state did not apply exactly");
	}
	psy_audio_preset_dispose(&captured);
	return 0;
}

static int preset_roundtrip(psy_audio_Machine* source, const char* preset_path,
	const char* module_path, const struct FluidState* expected)
{
	psy_audio_Presets saved;
	psy_audio_Presets loaded;
	psy_audio_Preset* captured;
	psy_audio_Preset* reloaded;
	psy_audio_MachineCallback callback;
	psy_audio_PluginCatcher catcher;
	psy_audio_MachineFactory factory;
	psy_audio_Machine* fresh = NULL;
	int status;
	int rc = 0;

	psy_audio_presets_init(&saved);
	psy_audio_presets_init(&loaded);
	captured = psy_audio_preset_alloc_init();
	if (!captured) return fail("preset allocation failed");
	psy_audio_machine_current_preset(source, captured);
	psy_audio_preset_set_name(captured, PRESET_NAME);
	if (psy_audio_preset_num_parameters(captured) != PARAMETER_COUNT ||
			captured->datasize != DATA_SIZE || !captured->data ||
			!state_matches((const struct FluidState*)captured->data, expected)) {
		rc = fail("captured FluidSynth preset state changed");
		goto cleanup;
	}
	psy_audio_presets_insert(&saved, 0, captured);
	captured = NULL;
	status = psy_audio_presetsio_save(preset_path, &saved);
	if (status != psy_audio_PRESETIO_OK) {
		rc = fail("FluidSynth preset save failed");
		goto cleanup;
	}
	status = psy_audio_presetsio_load(preset_path, &loaded, PARAMETER_COUNT, DATA_SIZE, "");
	if (status != psy_audio_PRESETIO_OK || psy_audio_presets_size(&loaded) != 1) {
		rc = fail("FluidSynth preset reload failed");
		goto cleanup;
	}
	reloaded = psy_audio_presets_at(&loaded, 0);
	if (!reloaded || reloaded->datasize != DATA_SIZE || !reloaded->data ||
			!state_matches((const struct FluidState*)reloaded->data, expected)) {
		rc = fail("reloaded FluidSynth preset opaque state changed");
		goto cleanup;
	}

	psy_audio_machinecallback_init(&callback);
	psy_audio_plugincatcher_init(&catcher, NULL);
	psy_audio_machinefactory_init(&factory, &callback, &catcher, NULL);
	psy_audio_machinefactory_createwithoutproxy(&factory);
	if (register_native_plugin(&catcher, module_path) != 0) rc = 1;
	else {
		fresh = make_machine(&factory);
		if (!fresh) rc = fail("independent preset-restore machine creation failed");
		else {
			psy_audio_machine_tweak_preset(fresh, reloaded);
			rc = apply_state(fresh, expected);
		}
	}
	if (fresh) psy_audio_machine_deallocate(fresh);
	psy_audio_machinefactory_dispose(&factory);
	psy_audio_plugincatcher_dispose(&catcher);

cleanup:
	if (captured) {
		psy_audio_preset_dispose(captured);
		free(captured);
	}
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
	struct FluidState expected;
	psy_audio_MachineCallback callback;
	psy_audio_PluginCatcher catcher;
	psy_audio_MachineFactory factory;
	psy_audio_Song* song = NULL;
	psy_audio_Machine* machine = NULL;
	psy_audio_MachineCallback loaded_callback;
	psy_audio_PluginCatcher loaded_catcher;
	psy_audio_MachineFactory loaded_factory;
	psy_audio_Song* loaded = NULL;
	int rc = 0;

	if (argc != 4) {
		fprintf(stderr, "usage: %s OUTPUT_DIRECTORY PATH_TO_FLUIDSYNTH_SO PATH_TO_SF2\n", argv[0]);
		return 2;
	}
	if (sizeof(struct FluidState) != DATA_SIZE) return fail("test SYNPAR layout changed");
	if (snprintf(song_path, sizeof(song_path), "%s/phase5-fluidsynth.psy", argv[1]) >= (int)sizeof(song_path) ||
			snprintf(preset_path, sizeof(preset_path), "%s/phase5-fluidsynth.prs", argv[1]) >= (int)sizeof(preset_path))
		return fail("output path too long");
	init_state(&expected, argv[3]);

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
		rc = fail("could not allocate FluidSynth song");
		goto initial_cleanup;
	}
	psy_audio_machinecallback_set_song(&callback, song);
	machine = make_machine(&factory);
	if (!machine || apply_state(machine, &expected) != 0 ||
			preset_roundtrip(machine, preset_path, argv[2], &expected) != 0) {
		rc = 1;
		if (machine) psy_audio_machine_deallocate(machine);
		machine = NULL;
		goto song_cleanup;
	}
	psy_audio_machines_insert(psy_audio_song_machines(song), SLOT, machine);
	machine = NULL;
	psy_audio_machines_connect(psy_audio_song_machines(song),
		psy_audio_wire_make(SLOT, psy_audio_MASTER_INDEX));
	if (save_song(song, song_path) != 0) {
		rc = fail("FluidSynth PSY3 save failed");
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
	if (!loaded) rc = fail("fresh FluidSynth PSY3 reopen failed");
	else {
		psy_audio_Machine* reopened;
		psy_audio_Preset snapshot;
		psy_audio_machinecallback_set_song(&loaded_callback, loaded);
		reopened = psy_audio_machines_at(psy_audio_song_machines(loaded), SLOT);
		psy_audio_preset_init(&snapshot);
		if (verify_identity(reopened) != 0) rc = 1;
		else {
			psy_audio_machine_current_preset(reopened, &snapshot);
			if (snapshot.datasize != DATA_SIZE || !snapshot.data ||
					!state_matches((const struct FluidState*)snapshot.data, &expected))
				rc = fail("fresh PSY3 reopen changed FluidSynth opaque state");
			else if (!psy_audio_machines_connected(psy_audio_song_machines(loaded),
					psy_audio_wire_make(SLOT, psy_audio_MASTER_INDEX)))
				rc = fail("FluidSynth wire to Master did not survive PSY3 reload");
		}
		psy_audio_preset_dispose(&snapshot);
		psy_audio_song_deallocate(loaded);
		loaded = NULL;
	}
	psy_audio_machinefactory_dispose(&loaded_factory);
	psy_audio_plugincatcher_dispose(&loaded_catcher);

song_cleanup:
	if (song) psy_audio_song_deallocate(song);
initial_cleanup:
	psy_audio_machinefactory_dispose(&factory);
	psy_audio_plugincatcher_dispose(&catcher);
	psy_audio_dispose();
	if (rc != 0) return rc;

	printf("phase5-fluidsynth-state: PASS\n");
	printf("catcher: fluidsynth:0\n");
	printf("state: opaque-bytes=5184 version=3 sf2=TimGM6mb channels=64 gain=80 polyphony=64\n");
	printf("preset-factory: independent\n");
	printf("topology: FluidSynth SF2 player -> Master\n");
	printf("preset: %s\n", preset_path);
	printf("song: %s\n", song_path);
	return 0;
}
