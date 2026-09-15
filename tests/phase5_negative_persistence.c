/*
** PSYCLE-LINUX Phase 5C Negative production persistence regression.
**
** Negative has no public parameters and no opaque state.  Preserve production
** discovery, exact host-routed sign inversion, a zero-parameter version-1
** preset, and a fresh PSY3 reopen through independent catcher/factory state.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <buffer.h>
#include <buffercontext.h>
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

#define PARAMETER_COUNT 0u
#define SLOT 0u
#define CATCHER_NAME "negative:0"
#define PRESET_NAME "Phase 5 Negative"

static int fail(const char* message)
{
	fprintf(stderr, "phase5-negative-state: FAIL: %s\n", message);
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
			strcmp(info.name, "Negative") != 0 ||
			!info.shortname || strcmp(info.shortname, "Negative") != 0 ||
			!info.author || strcmp(info.author, "who cares") != 0 ||
			info.plugversion != 0x0100) {
		rc = fail("production probe returned unexpected Negative identity");
	} else {
		machineinfo_catchername(&info, catcher_name);
		if (strcmp(catcher_name, CATCHER_NAME) != 0) {
			fprintf(stderr,
				"phase5-negative-state: FAIL: catcher expected %s got %s\n",
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
		return fail("Negative missing or no longer a native Psycle plugin");
	info = psy_audio_machine_info(machine);
	if (!info || !info->name || strcmp(info->name, "Negative") != 0 ||
			!info->shortname || strcmp(info->shortname, "Negative") != 0 ||
			!info->author || strcmp(info->author, "who cares") != 0 ||
			info->plugversion != 0x0100 ||
			!info->modulepath || !strstr(info->modulepath, "negative"))
		return fail("Negative production identity/module path changed");
	if (psy_audio_machine_num_parameters(machine) != PARAMETER_COUNT)
		return fail("Negative production parameter count changed");
	if (psy_audio_machine_data_size(machine) != 0)
		return fail("Negative unexpectedly acquired opaque plugin state");
	return 0;
}

static int verify_production_audio(psy_audio_Machine* machine)
{
	psy_audio_Buffer buffer;
	psy_audio_BufferContext bc;
	float* left;
	float* right;
	int rc = 0;

	psy_audio_buffer_init(&buffer, 2);
	psy_audio_buffer_allocsamples(&buffer, 5);
	left = psy_audio_buffer_at(&buffer, 0);
	right = psy_audio_buffer_at(&buffer, 1);
	if (!left || !right) {
		psy_audio_buffer_dispose(&buffer);
		return fail("production audio buffer allocation failed");
	}

	left[0] = 1.0f; left[1] = -2.0f; left[2] = 3.5f;
	left[3] = 444.0f; left[4] = 555.0f;
	right[0] = -4.0f; right[1] = 5.0f; right[2] = -6.5f;
	right[3] = -444.0f; right[4] = -555.0f;
	psy_audio_buffercontext_init(&bc, NULL, &buffer, &buffer, 3, 1);
	psy_audio_machine_work(machine, &bc);
	if (left[0] != -1.0f || left[1] != 2.0f || left[2] != -3.5f ||
			right[0] != 4.0f || right[1] != -5.0f || right[2] != 6.5f ||
			left[3] != 444.0f || left[4] != 555.0f ||
			right[3] != -444.0f || right[4] != -555.0f)
		rc = fail("production host-routed sign inversion or block boundary changed");
	psy_audio_buffercontext_dispose(&bc);

	if (rc == 0) {
		left[0] = 7.0f; right[0] = -8.0f;
		psy_audio_buffercontext_init(&bc, NULL, &buffer, &buffer, 0, 1);
		psy_audio_machine_work(machine, &bc);
		if (left[0] != 7.0f || right[0] != -8.0f)
			rc = fail("production zero-length callback modified audio");
		psy_audio_buffercontext_dispose(&bc);
	}

	psy_audio_buffer_dispose(&buffer);
	return rc;
}

static int preset_roundtrip(psy_audio_Machine* source, const char* preset_path,
	const char* module_path)
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
	if (!captured) {
		psy_audio_presets_dispose(&saved);
		psy_audio_presets_dispose(&loaded);
		return fail("preset allocation failed");
	}
	psy_audio_machine_current_preset(source, captured);
	psy_audio_preset_set_name(captured, PRESET_NAME);
	if (psy_audio_preset_num_parameters(captured) != PARAMETER_COUNT ||
			captured->datasize != 0 || captured->data != NULL) {
		psy_audio_preset_dispose(captured);
		free(captured);
		psy_audio_presets_dispose(&saved);
		psy_audio_presets_dispose(&loaded);
		return fail("captured zero-parameter preset geometry changed");
	}
	psy_audio_presets_insert(&saved, 0, captured);
	status = psy_audio_presetsio_save(preset_path, &saved);
	if (status != psy_audio_PRESETIO_OK) {
		rc = fail("Negative zero-parameter preset save failed");
		goto cleanup;
	}
	status = psy_audio_presetsio_load(preset_path, &loaded, PARAMETER_COUNT, 0, "");
	if (status != psy_audio_PRESETIO_OK || psy_audio_presets_size(&loaded) != 1) {
		rc = fail("Negative zero-parameter preset reload failed");
		goto cleanup;
	}
	reloaded = psy_audio_presets_at(&loaded, 0);
	if (!reloaded || strcmp(psy_audio_preset_name(reloaded), PRESET_NAME) != 0 ||
			psy_audio_preset_num_parameters(reloaded) != PARAMETER_COUNT ||
			reloaded->datasize != 0 || reloaded->data != NULL) {
		rc = fail("reloaded Negative preset metadata changed");
		goto cleanup;
	}

	psy_audio_machinecallback_init(&callback);
	psy_audio_plugincatcher_init(&catcher, NULL);
	psy_audio_machinefactory_init(&factory, &callback, &catcher, NULL);
	psy_audio_machinefactory_createwithoutproxy(&factory);
	if (register_native_plugin(&catcher, module_path) != 0) {
		rc = 1;
	} else {
		fresh = make_machine(&factory);
		if (!fresh) rc = fail("independent preset-restore machine creation failed");
		else {
			psy_audio_machine_tweak_preset(fresh, reloaded);
			if (verify_identity(fresh) != 0 || verify_production_audio(fresh) != 0)
				rc = 1;
		}
	}
	if (fresh) psy_audio_machine_deallocate(fresh);
	psy_audio_machinefactory_dispose(&factory);
	psy_audio_plugincatcher_dispose(&catcher);

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

static psy_audio_Song* load_song(psy_audio_MachineFactory* factory,
	const char* path)
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
	psy_audio_MachineCallback loaded_callback;
	psy_audio_PluginCatcher loaded_catcher;
	psy_audio_MachineFactory loaded_factory;
	psy_audio_Song* loaded = NULL;
	int rc = 0;

	if (argc != 3) {
		fprintf(stderr, "usage: %s OUTPUT_DIRECTORY PATH_TO_NEGATIVE_SO\n", argv[0]);
		return 2;
	}
	if (snprintf(song_path, sizeof(song_path), "%s/phase5-negative.psy", argv[1]) >=
			(int)sizeof(song_path) ||
			snprintf(preset_path, sizeof(preset_path), "%s/phase5-negative.prs", argv[1]) >=
			(int)sizeof(preset_path))
		return fail("output path too long");

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
		rc = fail("could not allocate Negative song");
		goto initial_cleanup;
	}
	psy_audio_machinecallback_set_song(&callback, song);
	machine = make_machine(&factory);
	if (!machine || verify_identity(machine) != 0 ||
			verify_production_audio(machine) != 0 ||
			preset_roundtrip(machine, preset_path, argv[2]) != 0) {
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
		rc = fail("Negative PSY3 save failed");
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
		rc = fail("fresh Negative PSY3 reopen failed");
	} else {
		psy_audio_Machine* reopened;
		psy_audio_machinecallback_set_song(&loaded_callback, loaded);
		reopened = psy_audio_machines_at(psy_audio_song_machines(loaded), SLOT);
		if (verify_identity(reopened) != 0 || verify_production_audio(reopened) != 0)
			rc = 1;
		else if (!psy_audio_machines_connected(psy_audio_song_machines(loaded),
				psy_audio_wire_make(SLOT, psy_audio_MASTER_INDEX)))
			rc = fail("Negative wire to Master did not survive PSY3 reload");
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

	printf("phase5-negative-state: PASS\n");
	printf("catcher: negative:0\n");
	printf("state: 0 public parameters, 0 opaque bytes\n");
	printf("audio: production host path exact stereo negation, bounded positive block, zero callback no-op\n");
	printf("preset-factory: independent\n");
	printf("topology: Negative -> Master\n");
	printf("preset: %s\n", preset_path);
	printf("song: %s\n", song_path);
	return 0;
}
