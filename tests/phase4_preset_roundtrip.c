/*
** PSYCLE-LINUX Phase 4 preset save/load compatibility regression.
**
** Exercises Psycle's retained version-1 preset-file serializer with both
** integer parameter values and opaque plugin state. All data is deterministic
** and project-authored at test time.
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <preset.h>
#include <presetio.h>
#include <presets.h>

#define PARAM_COUNT 4u
#define STATE_SIZE 12u

static const int PARAMS_A[PARAM_COUNT] = { 0, 0x1234, 0x7fff, 0xffff };
static const int PARAMS_B[PARAM_COUNT] = { 0x42, 0x2048, 0x8000, 0xbeef };
static const unsigned char STATE_A[STATE_SIZE] = {
	0x50, 0x53, 0x59, 0x43, 0x4c, 0x45, 0x00, 0x01, 0x11, 0x22, 0x33, 0x44
};
static const unsigned char STATE_B[STATE_SIZE] = {
	0x50, 0x53, 0x59, 0x43, 0x4c, 0x45, 0x00, 0x02, 0xaa, 0xbb, 0xcc, 0xdd
};

static int fail(const char* message)
{
	fprintf(stderr, "phase4-preset-roundtrip: FAIL: %s\n", message);
	return 1;
}

static psy_audio_Preset* make_preset(const char* name, const int* params,
	const unsigned char* state, uintptr_t state_size)
{
	psy_audio_Preset* preset;
	uintptr_t i;

	preset = psy_audio_preset_alloc_init();
	if (!preset) {
		return NULL;
	}
	psy_audio_preset_set_name(preset, name);
	for (i = 0; i < PARAM_COUNT; ++i) {
		psy_audio_preset_set_value(preset, i, params[i]);
	}
	if (state_size > 0) {
		psy_audio_preset_put_data(preset, state_size, (void*)state);
	}
	return preset;
}

static int verify_preset(psy_audio_Preset* preset, const char* name,
	const int* params, const unsigned char* state, uintptr_t state_size)
{
	uintptr_t i;

	if (!preset) {
		return fail("expected preset is missing");
	}
	if (strcmp(psy_audio_preset_name(preset), name) != 0) {
		return fail("preset name changed across save/load");
	}
	if (psy_audio_preset_num_parameters(preset) != PARAM_COUNT) {
		return fail("preset parameter count changed across save/load");
	}
	for (i = 0; i < PARAM_COUNT; ++i) {
		if (psy_audio_preset_value(preset, i) != params[i]) {
			fprintf(stderr,
				"phase4-preset-roundtrip: FAIL: parameter %lu mismatch: expected %d, got %ld\n",
				(unsigned long)i, params[i],
				(long)psy_audio_preset_value(preset, i));
			return 1;
		}
	}
	if (preset->datasize != state_size) {
		return fail("opaque preset-state size changed across save/load");
	}
	if (state_size > 0 && (!preset->data || memcmp(preset->data, state, state_size) != 0)) {
		return fail("opaque preset-state bytes changed across save/load");
	}
	return 0;
}

static int exercise_file(const char* path, uintptr_t state_size)
{
	psy_audio_Presets saved;
	psy_audio_Presets loaded;
	psy_audio_Preset* a;
	psy_audio_Preset* b;
	int status;
	int rc;

	psy_audio_presets_init(&saved);
	psy_audio_presets_init(&loaded);
	a = make_preset("Deterministic A", PARAMS_A, STATE_A, state_size);
	b = make_preset("Deterministic B", PARAMS_B, STATE_B, state_size);
	if (!a || !b) {
		if (a) {
			psy_audio_preset_dispose(a);
			free(a);
		}
		if (b) {
			psy_audio_preset_dispose(b);
			free(b);
		}
		psy_audio_presets_dispose(&saved);
		psy_audio_presets_dispose(&loaded);
		return fail("could not allocate deterministic presets");
	}
	psy_audio_presets_insert(&saved, 0, a);
	psy_audio_presets_insert(&saved, 1, b);

	status = psy_audio_presetsio_save(path, &saved);
	if (status != psy_audio_PRESETIO_OK) {
		fprintf(stderr, "phase4-preset-roundtrip: save failed: %s (%d)\n",
			psy_audio_presetsio_statusstr(status), status);
		psy_audio_presets_dispose(&saved);
		psy_audio_presets_dispose(&loaded);
		return 1;
	}
	status = psy_audio_presetsio_load(path, &loaded, PARAM_COUNT, state_size, "");
	if (status != psy_audio_PRESETIO_OK) {
		fprintf(stderr, "phase4-preset-roundtrip: load failed: %s (%d)\n",
			psy_audio_presetsio_statusstr(status), status);
		psy_audio_presets_dispose(&saved);
		psy_audio_presets_dispose(&loaded);
		return 1;
	}
	if (psy_audio_presets_size(&loaded) != 2) {
		psy_audio_presets_dispose(&saved);
		psy_audio_presets_dispose(&loaded);
		return fail("preset count changed across save/load");
	}
	rc = verify_preset(psy_audio_presets_at(&loaded, 0), "Deterministic A",
		PARAMS_A, STATE_A, state_size);
	if (rc == 0) {
		rc = verify_preset(psy_audio_presets_at(&loaded, 1), "Deterministic B",
			PARAMS_B, STATE_B, state_size);
	}
	psy_audio_presets_dispose(&saved);
	psy_audio_presets_dispose(&loaded);
	return rc;
}

int main(int argc, char** argv)
{
	char state_path[4096];
	char parameter_path[4096];

	if (argc != 2) {
		fprintf(stderr, "usage: %s OUTPUT_DIRECTORY\n", argv[0]);
		return 2;
	}
	if (snprintf(state_path, sizeof(state_path), "%s/phase4-state-presets.prs", argv[1]) >=
			(int)sizeof(state_path) ||
			snprintf(parameter_path, sizeof(parameter_path), "%s/phase4-parameter-presets.prs",
				argv[1]) >= (int)sizeof(parameter_path)) {
		return fail("output path is too long");
	}
	if (exercise_file(parameter_path, 0) != 0) {
		return 1;
	}
	if (exercise_file(state_path, STATE_SIZE) != 0) {
		return 1;
	}
	printf("phase4-preset-roundtrip: PASS\n");
	printf("parameter-presets: %s\n", parameter_path);
	printf("state-presets: %s\n", state_path);
	return 0;
}
