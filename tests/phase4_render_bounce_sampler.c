/*
** PSYCLE-LINUX Phase 4 render -> WAV -> Sampler compatibility harness.
**
** The source WAV is generated deterministically at test time. Psycle imports it
** as a Sampler song, renders that song through the production Player and
** FileOutDriver, then imports the rendered WAV back through Psycle's historical
** WAV song path and proves the bounced sample survives PSY3 save/reload.
*/

#include <math.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <audioconfig.h>
#include <audiodriverplugin.h>
#include <fileoutdriver.h>
#include <machine.h>
#include <machinefactory.h>
#include <player.h>
#include <plugincatcher.h>
#include <properties.h>
#include <sample.h>
#include <samples.h>
#include <song.h>
#include <songio.h>
#include <thread.h>
#include <wire.h>

#define SOURCE_RATE 44100u
#define SOURCE_FRAMES 11025u
#define SOURCE_FREQ 440.0
#define SOURCE_AMPLITUDE 12000.0
#define EXPECTED_RENDER_CHANNELS 2u
#define EXPECTED_RENDER_BITS 16u
#define RENDER_TIMEOUT_TICKS 500u
#define RENDER_WAIT_US 10000u
#define MIN_RENDER_PEAK 100.0
#define FILEOUT_BLOCK_FRAMES 1024u
#define SOURCE_PLAYBACK_STEP 0.5
#define EXPECTED_RENDER_GAIN 0.5
#define DEFAULT_ATTACK_SECONDS 0.005
#define SOURCE_SIGNAL_TOLERANCE 64.0
#define PCM_TOLERANCE 0.5


typedef struct RenderStopState {
	atomic_bool stopped;
} RenderStopState;

typedef struct WavInfo {
	uint16_t format_tag;
	uint16_t channels;
	uint32_t sample_rate;
	uint16_t bits_per_sample;
	uint32_t data_bytes;
	uint32_t frames;
	long file_size;
	int peak;
} WavInfo;

static int fail(const char* message)
{
	fprintf(stderr, "phase4-render-bounce-sampler: FAIL: %s\n", message);
	return 1;
}

static uint16_t read_u16_le(const unsigned char* p)
{
	return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t read_u32_le(const unsigned char* p)
{
	return (uint32_t)p[0] |
		((uint32_t)p[1] << 8) |
		((uint32_t)p[2] << 16) |
		((uint32_t)p[3] << 24);
}

static int write_bytes(FILE* file, const void* data, size_t size)
{
	return fwrite(data, 1, size, file) == size ? 0 : -1;
}

static int write_u16_le(FILE* file, uint16_t value)
{
	unsigned char bytes[2];

	bytes[0] = (unsigned char)(value & 0xffu);
	bytes[1] = (unsigned char)((value >> 8) & 0xffu);
	return write_bytes(file, bytes, sizeof(bytes));
}

static int write_u32_le(FILE* file, uint32_t value)
{
	unsigned char bytes[4];

	bytes[0] = (unsigned char)(value & 0xffu);
	bytes[1] = (unsigned char)((value >> 8) & 0xffu);
	bytes[2] = (unsigned char)((value >> 16) & 0xffu);
	bytes[3] = (unsigned char)((value >> 24) & 0xffu);
	return write_bytes(file, bytes, sizeof(bytes));
}

static int16_t source_pcm_value(uintptr_t frame)
{
	double phase;

	phase = 2.0 * M_PI * SOURCE_FREQ * (double)frame / (double)SOURCE_RATE;
	return (int16_t)lrint(sin(phase) * SOURCE_AMPLITUDE);
}

static double expected_render_sample(uintptr_t frame)
{
	double source_position;
	uintptr_t source_frame;
	double fraction;
	double sample;
	double envelope;

	/* WAV-song import emits note 48 while the retained Sampler uses middle-C
	** pitch as its unity-rate reference, so this fixture plays at half speed.
	** The centered mono voice contributes half amplitude to each stereo side. */
	source_position = (double)frame * SOURCE_PLAYBACK_STEP;
	source_frame = (uintptr_t)source_position;
	fraction = source_position - (double)source_frame;
	sample = (double)source_pcm_value(source_frame);
	if (source_frame + 1u < SOURCE_FRAMES) {
		sample += ((double)source_pcm_value(source_frame + 1u) - sample) * fraction;
	}
	envelope = (double)frame / (DEFAULT_ATTACK_SECONDS * (double)SOURCE_RATE);
	if (envelope > 1.0) {
		envelope = 1.0;
	}
	return sample * EXPECTED_RENDER_GAIN * envelope;
}

static int compare_rendered_pcm_with_source(const char* path, const WavInfo* info)
{
	FILE* file;
	uintptr_t frame;

	file = fopen(path, "rb");
	if (!file) {
		return fail("could not reopen rendered WAV for source comparison");
	}
	if (fseek(file, 44, SEEK_SET) != 0) {
		fclose(file);
		return fail("could not seek to rendered WAV PCM for source comparison");
	}
	for (frame = 0; frame < SOURCE_FRAMES; ++frame) {
		unsigned char left_bytes[2];
		unsigned char right_bytes[2];
		int16_t left;
		int16_t right;
		double expected;

		if (fread(left_bytes, 1, 2, file) != 2 ||
				fread(right_bytes, 1, 2, file) != 2) {
			fclose(file);
			return fail("rendered WAV ended during deterministic source comparison");
		}
		left = (int16_t)read_u16_le(left_bytes);
		right = (int16_t)read_u16_le(right_bytes);
		expected = expected_render_sample(frame);
		if (fabs((double)left - expected) > SOURCE_SIGNAL_TOLERANCE ||
				fabs((double)right - expected) > SOURCE_SIGNAL_TOLERANCE) {
			fprintf(stderr,
				"phase4-render-bounce-sampler: FAIL: source/render mismatch at frame %lu: expected %.3f, got L=%d R=%d\n",
				(unsigned long)frame, expected, (int)left, (int)right);
			fclose(file);
			return 1;
		}
		if (abs((int)left - (int)right) > 1) {
			fclose(file);
			return fail("deterministic mono source did not render identically to both stereo channels");
		}
	}
	fclose(file);
	return 0;
}

static int write_source_wav(const char* path)
{
	FILE* file;
	uint32_t data_bytes;
	uint32_t i;
	int failed;

	file = fopen(path, "wb");
	if (!file) {
		return fail("could not create deterministic source WAV");
	}
	data_bytes = SOURCE_FRAMES * 2u;
	failed = 0;
	failed |= write_bytes(file, "RIFF", 4);
	failed |= write_u32_le(file, 36u + data_bytes);
	failed |= write_bytes(file, "WAVE", 4);
	failed |= write_bytes(file, "fmt ", 4);
	failed |= write_u32_le(file, 16u);
	failed |= write_u16_le(file, 1u);
	failed |= write_u16_le(file, 1u);
	failed |= write_u32_le(file, SOURCE_RATE);
	failed |= write_u32_le(file, SOURCE_RATE * 2u);
	failed |= write_u16_le(file, 2u);
	failed |= write_u16_le(file, 16u);
	failed |= write_bytes(file, "data", 4);
	failed |= write_u32_le(file, data_bytes);
	for (i = 0; i < SOURCE_FRAMES && !failed; ++i) {
		failed |= write_u16_le(file, (uint16_t)source_pcm_value(i));
	}
	if (failed || ferror(file)) {
		fclose(file);
		return fail("short write while creating deterministic source WAV");
	}
	if (fclose(file) != 0) {
		return fail("could not finalize deterministic source WAV");
	}
	return 0;
}

static int inspect_rendered_wav(const char* path, WavInfo* info)
{
	FILE* file;
	unsigned char header[44];
	uint32_t riff_size;
	uint32_t byte_rate;
	uint16_t block_align;
	uint32_t frame_bytes;
	uint32_t i;
	int peak;

	memset(info, 0, sizeof(*info));
	file = fopen(path, "rb");
	if (!file) {
		return fail("FileOutDriver did not create the rendered WAV");
	}
	if (fread(header, 1, sizeof(header), file) != sizeof(header)) {
		fclose(file);
		return fail("rendered WAV is shorter than the canonical PCM header");
	}
	if (memcmp(header, "RIFF", 4) != 0 ||
			memcmp(header + 8, "WAVE", 4) != 0 ||
			memcmp(header + 12, "fmt ", 4) != 0 ||
			memcmp(header + 36, "data", 4) != 0) {
		fclose(file);
		return fail("rendered WAV does not contain the expected RIFF/WAVE PCM layout");
	}
	if (read_u32_le(header + 16) != 16u) {
		fclose(file);
		return fail("rendered WAV fmt chunk is not canonical PCM");
	}
	info->format_tag = read_u16_le(header + 20);
	info->channels = read_u16_le(header + 22);
	info->sample_rate = read_u32_le(header + 24);
	byte_rate = read_u32_le(header + 28);
	block_align = read_u16_le(header + 32);
	info->bits_per_sample = read_u16_le(header + 34);
	info->data_bytes = read_u32_le(header + 40);
	riff_size = read_u32_le(header + 4);

	if (fseek(file, 0, SEEK_END) != 0) {
		fclose(file);
		return fail("could not determine rendered WAV size");
	}
	info->file_size = ftell(file);
	if (info->file_size < 44) {
		fclose(file);
		return fail("rendered WAV file size is invalid");
	}
	if ((uint32_t)(info->file_size - 8) != riff_size) {
		fclose(file);
		return fail("rendered WAV RIFF size does not match the actual file size");
	}
	if ((uint32_t)(info->file_size - 44) != info->data_bytes) {
		fclose(file);
		return fail("rendered WAV data chunk size does not match the audio bytes written");
	}
	if (info->format_tag != 1u ||
			info->channels != EXPECTED_RENDER_CHANNELS ||
			info->sample_rate != SOURCE_RATE ||
			info->bits_per_sample != EXPECTED_RENDER_BITS) {
		fclose(file);
		return fail("rendered WAV format changed from 44.1 kHz stereo 16-bit PCM");
	}
	frame_bytes = (uint32_t)block_align;
	if (frame_bytes != info->channels * (info->bits_per_sample / 8u) ||
			frame_bytes == 0u || info->data_bytes % frame_bytes != 0u) {
		fclose(file);
		return fail("rendered WAV block alignment or frame count is invalid");
	}
	if (byte_rate != info->sample_rate * frame_bytes) {
		fclose(file);
		return fail("rendered WAV byte rate is inconsistent with its format");
	}
	info->frames = info->data_bytes / frame_bytes;
	if (info->frames == 0u) {
		fclose(file);
		return fail("rendered WAV contains no audio frames");
	}
	/* FileOut renders fixed 1,024-frame blocks. The deterministic source
	** is 11,025 frames, so only the final block may extend the file. */
	if (info->frames < SOURCE_FRAMES ||
			info->frames >= SOURCE_FRAMES + FILEOUT_BLOCK_FRAMES) {
		fclose(file);
		return fail("rendered WAV duration falls outside the final-buffer tolerance");
	}
	if (fseek(file, 44, SEEK_SET) != 0) {
		fclose(file);
		return fail("could not seek to rendered PCM data");
	}
	peak = 0;
	for (i = 0; i < info->data_bytes / 2u; ++i) {
		unsigned char bytes[2];
		int16_t sample;
		int magnitude;

		if (fread(bytes, 1, 2, file) != 2) {
			fclose(file);
			return fail("rendered WAV PCM data is shorter than its declared size");
		}
		sample = (int16_t)read_u16_le(bytes);
		magnitude = sample < 0 ? -(int)sample : (int)sample;
		if (magnitude > peak) {
			peak = magnitude;
		}
	}
	fclose(file);
	info->peak = peak;
	if ((double)peak < MIN_RENDER_PEAK) {
		return fail("rendered WAV is effectively silent");
	}
	return 0;
}

static psy_audio_Song* load_song(psy_audio_MachineFactory* factory,
	const char* path)
{
	psy_audio_Song* song;
	psy_audio_SongReader reader;
	int status;

	song = psy_audio_song_alloc_init(factory);
	if (!song) {
		return NULL;
	}
	psy_audio_songreader_init(&reader, song, NULL, FALSE);
	status = psy_audio_songreader_load(&reader, path);
	psy_audio_songreader_dispose(&reader);
	if (status != PSY_OK) {
		fprintf(stderr, "phase4-render-bounce-sampler: load failed for %s (%d)\n",
			path, status);
		psy_audio_song_deallocate(song);
		return NULL;
	}
	return song;
}

static int save_song(psy_audio_Song* song, const char* path)
{
	psy_audio_SongFile songfile;
	int status;

	psy_audio_songfile_init_song(&songfile, song);
	status = psy_audio_songfile_save(&songfile, path);
	psy_audio_songfile_dispose(&songfile);
	if (status != PSY_OK) {
		fprintf(stderr, "phase4-render-bounce-sampler: save failed for %s (%d)\n",
			path, status);
		return 1;
	}
	return 0;
}

static void on_render_stopped(RenderStopState* state, psy_AudioDriver* sender)
{
	(void)sender;
	atomic_store_explicit(&state->stopped, true, memory_order_release);
}

static int verify_bounced_song(psy_audio_Song* song, const WavInfo* info)
{
	psy_audio_Sample* sample;
	psy_audio_Machine* machine;
	double peak;
	uintptr_t channel;
	uintptr_t frame;

	sample = psy_audio_samples_at(psy_audio_song_samples(song),
		psy_audio_sampleindex_make(0, 0));
	if (!sample) {
		return fail("rendered WAV did not import into sample slot 0:0");
	}
	if (sample->channels.numchannels != info->channels) {
		return fail("rendered WAV channel count changed during Sampler import");
	}
	if (psy_audio_sample_num_frames(sample) != info->frames) {
		return fail("rendered WAV frame count changed during Sampler import");
	}
	if (fabs(sample->samplerate - (double)info->sample_rate) > 0.5) {
		return fail("rendered WAV sample rate changed during Sampler import");
	}
	peak = 0.0;
	for (channel = 0; channel < sample->channels.numchannels; ++channel) {
		const float* pcm;

		pcm = sample->channels.samples[channel];
		if (!pcm) {
			return fail("rendered WAV imported with a missing PCM channel");
		}
		for (frame = 0; frame < psy_audio_sample_num_frames(sample); ++frame) {
			double magnitude;

			magnitude = fabs((double)pcm[frame]);
			if (magnitude > peak) {
				peak = magnitude;
			}
		}
	}
	if (peak < MIN_RENDER_PEAK) {
		return fail("rendered WAV became silent during Sampler import");
	}
	machine = psy_audio_machines_at(psy_audio_song_machines(song), 0);
	if (!machine || psy_audio_machine_type(machine) != psy_audio_SAMPLER) {
		return fail("rendered WAV import did not create the built-in Sampler");
	}
	if (!psy_audio_machines_connected(psy_audio_song_machines(song),
			psy_audio_wire_make(0, psy_audio_MASTER_INDEX))) {
		return fail("rendered WAV Sampler is not connected to Master");
	}
	return 0;
}

static int compare_imported_pcm_with_wav(psy_audio_Song* song,
	const char* path, const WavInfo* info)
{
	psy_audio_Sample* sample;
	FILE* file;
	uintptr_t frame;
	uintptr_t channel;

	sample = psy_audio_samples_at(psy_audio_song_samples(song),
		psy_audio_sampleindex_make(0, 0));
	if (!sample || sample->channels.numchannels != info->channels ||
			psy_audio_sample_num_frames(sample) != info->frames) {
		return fail("cannot compare rendered WAV with imported Sampler PCM");
	}
	file = fopen(path, "rb");
	if (!file) {
		return fail("could not reopen rendered WAV for PCM comparison");
	}
	if (fseek(file, 44, SEEK_SET) != 0) {
		fclose(file);
		return fail("could not seek to rendered WAV PCM for comparison");
	}
	for (frame = 0; frame < info->frames; ++frame) {
		for (channel = 0; channel < info->channels; ++channel) {
			unsigned char bytes[2];
			int16_t expected;
			double actual;

			if (fread(bytes, 1, 2, file) != 2) {
				fclose(file);
				return fail("rendered WAV ended during Sampler PCM comparison");
			}
			expected = (int16_t)read_u16_le(bytes);
			actual = (double)sample->channels.samples[channel][frame];
			if (fabs(actual - (double)expected) > PCM_TOLERANCE) {
				fprintf(stderr,
					"phase4-render-bounce-sampler: FAIL: WAV/Sampler PCM mismatch at frame %lu channel %lu: expected %d, got %.3f\n",
					(unsigned long)frame, (unsigned long)channel,
					(int)expected, actual);
				fclose(file);
				return 1;
			}
		}
	}
	fclose(file);
	return 0;
}

static int compare_bounced_pcm(psy_audio_Song* before, psy_audio_Song* after)
{
	psy_audio_Sample* lhs;
	psy_audio_Sample* rhs;
	uintptr_t channels;
	uintptr_t frames;
	uintptr_t channel;
	uintptr_t frame;

	lhs = psy_audio_samples_at(psy_audio_song_samples(before),
		psy_audio_sampleindex_make(0, 0));
	rhs = psy_audio_samples_at(psy_audio_song_samples(after),
		psy_audio_sampleindex_make(0, 0));
	if (!lhs || !rhs) {
		return fail("bounced sample is missing before or after PSY3 reload");
	}
	if (lhs->channels.numchannels != rhs->channels.numchannels ||
			psy_audio_sample_num_frames(lhs) != psy_audio_sample_num_frames(rhs) ||
			fabs(lhs->samplerate - rhs->samplerate) > 0.5) {
		return fail("bounced sample geometry changed across PSY3 reload");
	}
	channels = lhs->channels.numchannels;
	frames = psy_audio_sample_num_frames(lhs);
	for (channel = 0; channel < channels; ++channel) {
		for (frame = 0; frame < frames; ++frame) {
			double delta;

			delta = fabs((double)lhs->channels.samples[channel][frame] -
				(double)rhs->channels.samples[channel][frame]);
			if (delta > PCM_TOLERANCE) {
				return fail("bounced PCM changed across PSY3 save/reload");
			}
		}
	}
	return 0;
}

int main(int argc, char** argv)
{
	char source_path[4096];
	char render_path[4096];
	char psy_path[4096];
	psy_Property* config;
	psy_audio_AudioConfig audioconfig;
	psy_audio_MachineCallback player_callback;
	psy_audio_Player player;
	psy_audio_Song* source_song;
	psy_AudioDriver* fileout;
	psy_AudioDriver* original_driver;
	RenderStopState stop_state;
	WavInfo wav_info;
	psy_audio_MachineCallback import_callback;
	psy_audio_PluginCatcher import_catcher;
	psy_audio_MachineFactory import_factory;
	psy_audio_Song* bounced_song;
	psy_audio_MachineCallback reload_callback;
	psy_audio_PluginCatcher reload_catcher;
	psy_audio_MachineFactory reload_factory;
	psy_audio_Song* reloaded_song;
	uintptr_t tick;
	int rc;

	if (argc != 2) {
		fprintf(stderr, "usage: %s OUTPUT_DIRECTORY\n", argv[0]);
		return 2;
	}
	if (snprintf(source_path, sizeof(source_path), "%s/phase4-render-source.wav",
			argv[1]) >= (int)sizeof(source_path) ||
			snprintf(render_path, sizeof(render_path), "%s/phase4-rendered-bounce.wav",
				argv[1]) >= (int)sizeof(render_path) ||
			snprintf(psy_path, sizeof(psy_path), "%s/phase4-rendered-bounce.psy",
				argv[1]) >= (int)sizeof(psy_path)) {
		return fail("output path is too long");
	}
	if (write_source_wav(source_path) != 0) {
		return 1;
	}

	psy_audio_init();
	config = psy_property_allocinit_key(NULL);
	if (!config) {
		psy_audio_dispose();
		return fail("could not allocate player configuration");
	}
	psy_audio_audioconfig_init(&audioconfig, config);
	psy_audio_machinecallback_init(&player_callback);
	psy_audio_player_init(&player, &player_callback, NULL,
		psy_audio_audioconfig_base(&audioconfig),
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

	source_song = load_song(&player.machinefactory, source_path);
	if (!source_song) {
		psy_audio_player_dispose(&player);
		psy_audio_audioconfig_dispose(&audioconfig);
		psy_property_deallocate(config);
		psy_audio_dispose();
		return 1;
	}
	psy_audio_machinecallback_set_song(&player_callback, source_song);
	psy_audio_machinecallback_set_player(&player_callback, &player);

	fileout = psy_audio_create_fileout_driver();
	if (!fileout) {
		psy_audio_song_deallocate(source_song);
		psy_audio_player_dispose(&player);
		psy_audio_audioconfig_dispose(&audioconfig);
		psy_property_deallocate(config);
		psy_audio_dispose();
		return fail("could not create FileOutDriver");
	}
	psy_property_set_str((psy_Property*)psy_audiodriver_configuration(fileout),
		"outputpath", render_path);
	psy_audiodriver_configure(fileout, NULL);

	original_driver = player.audiodrivers.driver_plugin.client;
	player.audiodrivers.driver_plugin.client = fileout;
	psy_audio_audiodriverplugin_connect(&player.audiodrivers.driver_plugin,
		player.audiodrivers.systemhandle,
		player.audiodrivers.context,
		(AUDIODRIVERWORKFN)player.audiodrivers.fp);
	psy_audio_player_set_song(&player, source_song);
	psy_audio_sequencer_stop_loop(&player.sequencer);
	psy_audio_player_set_position(&player, 0.0);
	psy_audio_player_start(&player);
	atomic_init(&stop_state.stopped, false);
	psy_signal_connect(&fileout->signal_stop, &stop_state, on_render_stopped);
	if (psy_audiodriver_open(fileout) != 0) {
		rc = fail("FileOutDriver failed to open");
	} else {
		rc = 0;
		for (tick = 0; tick < RENDER_TIMEOUT_TICKS &&
				!atomic_load_explicit(&stop_state.stopped, memory_order_acquire); ++tick) {
			psy_sleep_for(RENDER_WAIT_US);
		}
		if (!atomic_load_explicit(&stop_state.stopped, memory_order_acquire)) {
			rc = fail("FileOutDriver did not finish within the regression timeout");
		}
	}
	psy_audio_player_stop(&player);
	psy_audiodriver_close(fileout);
	player.audiodrivers.driver_plugin.client = original_driver;
	psy_audio_audiodriverplugin_connect(&player.audiodrivers.driver_plugin,
		player.audiodrivers.systemhandle,
		player.audiodrivers.context,
		(AUDIODRIVERWORKFN)player.audiodrivers.fp);

	if (rc == 0) {
		rc = inspect_rendered_wav(render_path, &wav_info);
	}
	if (rc == 0) {
		rc = compare_rendered_pcm_with_source(render_path, &wav_info);
	}

	psy_audio_machinecallback_init(&import_callback);
	psy_audio_plugincatcher_init(&import_catcher, NULL);
	psy_audio_machinefactory_init(&import_factory, &import_callback,
		&import_catcher, NULL);
	bounced_song = NULL;
	if (rc == 0) {
		bounced_song = load_song(&import_factory, render_path);
		if (!bounced_song) {
			rc = 1;
		} else {
			psy_audio_machinecallback_set_song(&import_callback, bounced_song);
			rc = verify_bounced_song(bounced_song, &wav_info);
			if (rc == 0) {
				rc = compare_imported_pcm_with_wav(bounced_song,
					render_path, &wav_info);
			}
		}
	}
	if (rc == 0) {
		rc = save_song(bounced_song, psy_path);
	}

	psy_audio_machinecallback_init(&reload_callback);
	psy_audio_plugincatcher_init(&reload_catcher, NULL);
	psy_audio_machinefactory_init(&reload_factory, &reload_callback,
		&reload_catcher, NULL);
	reloaded_song = NULL;
	if (rc == 0) {
		reloaded_song = load_song(&reload_factory, psy_path);
		if (!reloaded_song) {
			rc = 1;
		} else {
			psy_audio_machinecallback_set_song(&reload_callback, reloaded_song);
			rc = verify_bounced_song(reloaded_song, &wav_info);
			if (rc == 0) {
				rc = compare_imported_pcm_with_wav(reloaded_song,
					render_path, &wav_info);
			}
			if (rc == 0) {
				rc = compare_bounced_pcm(bounced_song, reloaded_song);
			}
		}
	}

	if (reloaded_song) {
		psy_audio_song_deallocate(reloaded_song);
	}
	psy_audio_machinefactory_dispose(&reload_factory);
	psy_audio_plugincatcher_dispose(&reload_catcher);
	if (bounced_song) {
		psy_audio_song_deallocate(bounced_song);
	}
	psy_audio_machinefactory_dispose(&import_factory);
	psy_audio_plugincatcher_dispose(&import_catcher);

	psy_audiodriver_deallocate(fileout);
	psy_audio_player_set_empty_song(&player);
	psy_audio_song_deallocate(source_song);
	psy_audio_player_dispose(&player);
	psy_audio_audioconfig_dispose(&audioconfig);
	psy_property_deallocate(config);
	psy_audio_dispose();

	if (rc != 0) {
		return rc;
	}
	printf("phase4-render-bounce-sampler: PASS\n");
	printf("source-wav: %s\n", source_path);
	printf("rendered-wav: %s\n", render_path);
	printf("rendered-frames: %u\n", wav_info.frames);
	printf("rendered-peak: %d\n", wav_info.peak);
	printf("saved-psy: %s\n", psy_path);
	return 0;
}
