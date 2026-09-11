/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "songio.h"
/* local */
#include "exs24loader.h"
#include "itmodule2.h"
#include "lysongexport.h"
#include "machinefactory.h"
#include "midiloader.h"
#include "midisongexport.h"
#include "psy3loader.h"
#include "psy3saver.h"
#include "psy2loader.h"
#include "song.h"
#include "wavsongio.h"
#include "wire.h"
#include "xmdefs.h"
#include "xmsongexport.h"
#include "xmsongloader.h"
/* platform */
#include "../../detail/portable.h"
#include "../../detail/os.h"

#if defined DIVERSALIS__OS__POSIX
#include <errno.h>
#endif

/* song/module filter */
static char load_filters[] =
"All Songs (*.psy *.xm *.it *.s3m *.mod *.wav *.mid)" "|*.psy;*.xm;*.it;*.s3m;*.mod;*.wav;*.mid|"
"Songs (*.psy)"				        "|*.psy|"
"FastTracker II Songs (*.xm)"       "|*.xm|"
"Impulse Tracker Songs (*.it)"      "|*.it|"
"Scream Tracker Songs (*.s3m)"      "|*.s3m|"
"Original Mod Format Songs (*.mod)" "|*.mod|"
"Wav Format Songs (*.wav)"			"|*.wav|"
"Midi Standard Songs (*.mid)"	    "|*.mid|";
static const char save_filters[] = "Songs (*.psy)|*.psy";
/* instrument filter */
static const char instloadfilter[] = "All Instruments and samples|*.psins;*.xi;*.iti;*.wav;*.aif;*.aiff;*.its;*.s3i;*.8svx;*.16sv;*.svx;*.iff;*.exs|Psycle Instrument (*.psins)|*.psins|XM Instruments (*.xi)|*.xi|IT Instruments (*.iti)|*.iti|Wav (PCM) Files (*.wav)|*.wav|Apple AIFF (PCM) Files (*.aif)|*.aif;*.aiff|ST3 Samples (*.s3i)|*.s3i|IT2 Samples (*.its)|*.its|Amiga IFF/SVX Samples (*.svx)|*.8svx;*.16sv;*.svx;*.iff;*.exs|All files (*.*)|*.*||";
static const char instsavefilter[] = "Instrument (*.psins)|*.psins";

/* psy_audio_SongFile */

void psy_audio_songfile_init(psy_audio_SongFile* self)
{
	self->logger = NULL;	
	self->machinesoloed = -1;
	self->legacywires = NULL;
	self->path = NULL;
	self->song = NULL;
	self->file = NULL;	
#if defined DIVERSALIS__OS__POSIX	
	self->convert_dos_utf8 = TRUE;
#else	
	self->convert_dos_utf8 = FALSE;
#endif
	self->use_jbridge = FALSE;
}

void psy_audio_songfile_init_song(psy_audio_SongFile* self,
	psy_audio_Song* song)
{
	psy_audio_songfile_init(self);
	self->song = song;	
}

void psy_audio_songfile_dispose(psy_audio_SongFile* self)
{
	free(self->path);
	self->path = NULL;	
	self->logger = NULL;	
}

void psy_audio_songfile_set_logger(psy_audio_SongFile* self, psy_Logger* logger)
{
	self->logger = logger;
}

int psy_audio_songfile_song_info(psy_audio_SongFile* self, const char* filename)
{
	PsyFile file;
	int status;

	status = PSY_OK;	
	self->err = 0;	
	self->file = &file;	
	psy_strreset(&self->path, filename);
	self->machinesoloed = -1;	
	psy_audio_songfile_message(self, "testing machines for ");
	psy_audio_songfile_message(self, self->path);
	psy_audio_songfile_message(self, "\n");
	if (psyfile_open(self->file, filename)) {
		char header[20];
		char riff[5];
		
		psy_audio_songfile_message(self, "loading ");
		psy_audio_songfile_message(self, self->path);
		psy_audio_songfile_message(self, "\n");		
		psyfile_read(self->file, header, 8);
		header[8] = '\0';
		strncpy(riff, header, 4);
		riff[4] = '\0';		
		if (strcmp(header,"PSY3SONG") == 0) {
#ifdef PSYCLE_USE_PSY3
			psy_audio_PSY3Loader psy3loader;

			psy_audio_psy3loader_init(&psy3loader, self);
			if ((status = psy_audio_psy3loader_check_machines(&psy3loader))) {
				psy_audio_songfile_errfile(self);
			}
			psy_audio_psy3loader_dispose(&psy3loader);
#else
			status = PSY_ERRFILEFORMAT;
#endif			
		} else if (strcmp(header,"PSY2SONG") == 0) {

		} else if (strcmp(riff, "RIFF") == 0) {			
		} else if (strcmp(riff, "IMPM") == 0) {
		} else if (strcmp(riff, "MThd") == 0) {
		} else {
			status = PSY_ERRFILEFORMAT;
		}
		psyfile_close(self->file);		
	} else {		
		status = PSY_ERRFILEFORMAT;
		psy_audio_songfile_errfile(self);
	}
	return status;
}

int psy_audio_songfile_save(psy_audio_SongFile* self, const char* filename)
{		
	int status;
	PsyFile file;	

	status = PSY_OK;
	self->file = &file;
	self->err = 0;	
	psy_strreset(&self->path, filename);	
	if (psyfile_create(self->file, self->path, 1)) {
		psy_audio_PSY3Saver psy3saver;

		psy_audio_psy3saver_init(&psy3saver, self);
		if ((status = psy_audio_psy3saver_save(&psy3saver))) {
			psy_audio_songfile_errfile(self);
		}
		psy_audio_psy3saver_dispose(&psy3saver);
		psyfile_close(self->file);
	} else {
		status = PSY_ERRFILEFORMAT;
		psy_audio_songfile_errfile(self);
	}
	return status;
}

int psy_audio_songfile_exportmodule(psy_audio_SongFile* self,
	const char* filename)
{
	int status;
	PsyFile file;

	status = PSY_OK;
	self->file = &file;
	self->err = 0;	
	psy_strreset(&self->path, filename);
	if (psyfile_create(self->file, self->path, 1)) {
		XMSongExport moduleexport;

		xmsongexport_init(&moduleexport, self);
		if ((status = xmsongexport_exportsong(&moduleexport))) {
			psy_audio_songfile_errfile(self);
		}
		xmsongexport_dispose(&moduleexport);
		psyfile_close(self->file);
	} else {
		status = PSY_ERRFILEFORMAT;
		psy_audio_songfile_errfile(self);
	}
	return status;
}

int psy_audio_songfile_export_midi_file(psy_audio_SongFile* self,
	const char* filename)
{
	int status;
	PsyFile file;

	status = PSY_OK;
	self->file = &file;
	self->err = 0;	
	psy_strreset(&self->path, filename);
	if (psyfile_create(self->file, self->path, 1)) {
		psy_audio_MidiSongExport moduleexport;

		psy_audio_midisongexport_init(&moduleexport, self);
		if ((status = psy_audio_midisongexport_save(&moduleexport))) {
			psy_audio_songfile_errfile(self);
		}
		psy_audio_midisongexport_dispose(&moduleexport);
		psyfile_close(self->file);
	} else {
		status = PSY_ERRFILEFORMAT;
		psy_audio_songfile_errfile(self);
	}
	return status;
}

int psy_audio_songfile_export_ly_file(psy_audio_SongFile* self,
	const char* filename)
{
	int status;
	PsyFile file;

	status = PSY_OK;
	self->file = &file;
	self->err = 0;	
	psy_strreset(&self->path, filename);
	if (psyfile_create(self->file, self->path, 1)) {
		psy_audio_LySongExport lyexport;

		psy_audio_lysongexport_init(&lyexport, self);
		if ((status = psy_audio_lysongexport_save(&lyexport))) {
			psy_audio_songfile_errfile(self);
		}
		psy_audio_lysongexport_dispose(&lyexport);
		psyfile_close(self->file);
	} else {
		status = PSY_ERRFILEFORMAT;
		psy_audio_songfile_errfile(self);
	}
	return status;
}

int psy_audio_songfile_save_instrument(psy_audio_SongFile* self,
	const char* path, psy_audio_Instrument* instrument)
{
	int status;
	PsyFile file;

	status = PSY_OK;
	self->file = &file;
	self->err = 0;	
	psy_strreset(&self->path, path);
	if (psyfile_create(self->file, self->path, 1)) {
		psy_audio_PSY3Saver psy3saver;

		psy_audio_psy3saver_init(&psy3saver, self);
		if (psy_audio_psy3saver_saveinstrument(&psy3saver, instrument)) {
			psy_audio_songfile_errfile(self);
		}
		psy_audio_psy3saver_dispose(&psy3saver);
		psyfile_close(self->file);
	} else {
		status = PSY_ERRFILEFORMAT;
		psy_audio_songfile_errfile(self);
	}
	return status;
}

int psy_audio_songfile_load_instrument(psy_audio_SongFile* self,
	const char* filename, psy_audio_InstrumentIndex index)
{	
	int status;
	psy_Path path;
	PsyFile file;
	
	if (!psyfile_open(&file, filename)) {
		return PSY_ERRFILE;
	}
	self->file = &file;	
	psy_strreset(&self->path, filename);	
	psy_path_init(&path, filename);
	status = PSY_ERRFILE;
	if (strcmp(psy_path_ext(&path), "xi") == 0) {
		XMSongLoader xmsongloader;

		xmsongloader_init(&xmsongloader, self);
		if ((status = xmsongloader_loadxi(&xmsongloader, index))) {
			psy_audio_songfile_errfile(self);
		}
		xmsongloader_dispose(&xmsongloader);		
	} else if (strcmp(psy_path_ext(&path), "psins") == 0) {
		psy_audio_PSY3Loader psy3songloader;

		psy_audio_psy3loader_init(&psy3songloader, self);
		if ((status = psy_audio_psy3loader_loadpsins(&psy3songloader, index))) {
			psy_audio_songfile_errfile(self);
		}
		psy_audio_psy3loader_dispose(&psy3songloader);
	} else if (strcmp(psy_path_ext(&path), "exs") == 0) {
		psy_audio_EXS24Loader instloader;

		psy_audio_exs24loader_init(&instloader, self);
		if ((status = psy_audio_exs24loader_load(&instloader))) {
			psy_audio_songfile_errfile(self);
		}
		psy_audio_exs24loader_dispose(&instloader);
	}
	psyfile_close(self->file);
	return status;
}

void psy_audio_songfile_message(psy_audio_SongFile* self, const char* text)
{
	if (self->logger) {
		psy_logger_output(self->logger, text);
	}	
}

void psy_audio_songfile_warn(psy_audio_SongFile* self, const char* text)
{
	if (self->logger) {
		psy_logger_warn(self->logger, text);
	}
}

void psy_audio_songfile_createmaster(psy_audio_SongFile* self)
{	
	psy_audio_machines_insertmaster(psy_audio_song_machines(self->song),
		psy_audio_machinefactory_make_machine(self->song->machine_factory,
		psy_audio_MASTER, NULL, UINTPTR_MAX));
}

void psy_audio_songfile_use_jbridge(psy_audio_SongFile* self)
{
	self->use_jbridge = TRUE;
	if (self->song) {
		self->song->machine_factory->use_jbridge = self->use_jbridge;
	}
}

void psy_audio_songfile_errfile(psy_audio_SongFile* self)
{
	if (self->logger) {
		psy_logger_error(self->logger, strerror(self->err));
		psy_logger_error(self->logger, "\n");
	}
	self->err = errno;
}

const char* psy_audio_songfile_loadfilter(void)
{
	return load_filters;
}

const char* psy_audio_songfile_standardloadfilter(void)
{
	return PSYCLE_DEFAULT_LOAD_EXTENSION;
}

const char* psy_audio_songfile_savefilter(void)
{
	return save_filters;
}

const char* psy_audio_songfile_standardsavefilter(void)
{
	return PSYCLE_DEFAULT_SAVE_EXTENSION;
}

const char* psy_audio_songfile_instloadfilter(void)
{
	return instloadfilter;
}

const char* psy_audio_songfile_standardinstloadfilter(void)
{
	return "psins";
}

const char* psy_audio_songfile_instsavefilter(void)
{
	return instsavefilter;
}

const char* psy_audio_songfile_standardinstsavefilter(void)
{
	return "psins";
}




/* psy_audio_SongReader */

void psy_audio_songreader_init(psy_audio_SongReader* self,
	psy_audio_Song* song, psy_Logger* logger, bool use_jbridge)
{	
	self->song = song;	
	self->logger = logger;
	self->file = NULL;
	self->machinesoloed = -1;
	self->legacywires = NULL;
	self->path = NULL;	
#if defined DIVERSALIS__OS__POSIX	
	self->convert_dos_utf8 = TRUE;
#else	
	self->convert_dos_utf8 = FALSE;
#endif
	self->use_jbridge = use_jbridge;	
	if (self->song) {
		self->song->machine_factory->use_jbridge = self->use_jbridge;
	}
}

void psy_audio_songreader_dispose(psy_audio_SongReader* self)
{
	free(self->path);
	self->path = NULL;
	self->logger = NULL;
}

void psy_audio_songreader_set_logger(psy_audio_SongReader* self, psy_Logger* logger)
{
	self->logger = logger;
}

int psy_audio_songreader_load(psy_audio_SongReader* self, const char* filename)
{
	PsyFile file;
	int status;

	status = PSY_OK;
	self->err = 0;
	self->file = &file;
	psy_strreset(&self->path, filename);
	self->machinesoloed = -1;
	psy_audio_songreader_message(self, "searching for ");
	psy_audio_songreader_message(self, self->path);
	psy_audio_songreader_message(self, "\n");
	if (psyfile_open(self->file, filename)) {
		char header[20];
		char riff[5];

		psy_audio_songreader_message(self, "loading ");
		psy_audio_songreader_message(self, self->path);
		psy_audio_songreader_message(self, "\n");
		if (self->song) {
			psy_audio_song_clear(self->song);
			psy_audio_machines_startfilemode(psy_audio_song_machines(self->song));
		}
		psyfile_read(self->file, header, 8);
		header[8] = '\0';
		strncpy(riff, header, 4);
		riff[4] = '\0';
		if (self->logger) {
			psy_logger_progress(self->logger, PSY_PROGRESS_STATE_TICK);
		}
		if (strcmp(header, "PSY3SONG") == 0) {
#ifdef PSYCLE_USE_PSY3
			psy_audio_PSY3Loader psy3loader;
			psy_audio_SongFile song_file;

			psy_audio_songfile_init_song(&song_file, self->song);
			song_file.convert_dos_utf8 = self->convert_dos_utf8;
			song_file.use_jbridge = self->use_jbridge;
			song_file.file = self->file;			
			psy_strreset(&song_file.path, self->path);
			psy_audio_psy3loader_init(&psy3loader, &song_file);
			if ((status = psy_audio_psy3loader_load(&psy3loader))) {
				psy_audio_songreader_errfile(self);
			}
			psy_audio_psy3loader_dispose(&psy3loader);
			psy_audio_songfile_dispose(&song_file);
#else
			status = PSY_ERRFILEFORMAT;
#endif			
		}
		else if (strcmp(header, "PSY2SONG") == 0) {
			/* PSY2SONG is the Sign of the file. */
#ifdef PSYCLE_USE_PSY2
			PSY2Loader psy2loader;
			psy_audio_SongFile song_file;

			psy_audio_songfile_init_song(&song_file, self->song);
			song_file.convert_dos_utf8 = self->convert_dos_utf8;
			song_file.use_jbridge = self->use_jbridge;
			song_file.file = self->file;
			psy_strreset(&song_file.path, self->path);
			psy2loader_init(&psy2loader, &song_file);
			psy2loader_load(&psy2loader);
			psy2loader_dispose(&psy2loader);
			psy_audio_songfile_dispose(&song_file);
#else
			status = PSY_ERRFILEFORMAT;
#endif			
		}
		else if (strcmp(riff, "RIFF") == 0) {
			psyfile_read(&file, header, 8);
			header[8] = 0;
			if (strcmp(&header[0], "WAVEfmt ") == 0) {
				psy_audio_SongFile song_file;

				psy_audio_songfile_init_song(&song_file, self->song);
				song_file.convert_dos_utf8 = self->convert_dos_utf8;
				song_file.use_jbridge = self->use_jbridge;
				song_file.file = self->file;
				psy_strreset(&song_file.path, self->path);
				psyfile_close(self->file);
				psy_audio_wav_songio_load(&song_file);
				psy_audio_songfile_dispose(&song_file);
			}
		}
		else if (strcmp(riff, "IMPM") == 0) {
			ITModule2 itmodule;
			psy_audio_SongFile song_file;

			psy_audio_songfile_init_song(&song_file, self->song);
			song_file.convert_dos_utf8 = self->convert_dos_utf8;
			song_file.use_jbridge = self->use_jbridge;
			song_file.file = self->file;
			psy_strreset(&song_file.path, self->path);

			psyfile_seek(self->file, 0);
			itmodule2_init(&itmodule, &song_file);
			if ((status = itmodule2_load(&itmodule))) {
				psy_audio_songreader_errfile(self);
			}
			itmodule2_dispose(&itmodule);
			psy_audio_songfile_dispose(&song_file);
		}
#ifdef PSYCLE_USE_MIDI_FILE
		else if (strcmp(riff, "MThd") == 0) {
			MidiLoader midifile;

			psyfile_seek(self->file, 0);
			midiloader_init(&midifile, self);
			if ((status = midiloader_load(&midifile))) {
				psy_audio_songreader_errfile(self);
			}
			midiloader_dispose(&midifile);
		}
#endif
#ifdef PSYCLE_USE_XM
		else {
			psyfile_read(self->file, header + 8,
				(uint32_t)(psy_strlen(XM_HEADER) - 8));
			header[psy_strlen(XM_HEADER)] = '\0';
			if (strcmp(header, XM_HEADER) == 0) {
				XMSongLoader loader;
				psy_audio_SongFile song_file;

				psy_audio_songfile_init_song(&song_file, self->song);
				song_file.convert_dos_utf8 = self->convert_dos_utf8;
				song_file.use_jbridge = self->use_jbridge;
				song_file.file = self->file;
				psy_strreset(&song_file.path, self->path);

				psyfile_seek(self->file, 0);
				xmsongloader_init(&loader, &song_file);
				if ((status = xmsongloader_load(&loader))) {
					psy_audio_songreader_errfile(self);
				}
				xmsongloader_dispose(&loader);
				psy_audio_songfile_dispose(&song_file);
			}
			else {
				MODSongLoader loader;
				psy_audio_SongFile song_file;

				psy_audio_songfile_init_song(&song_file, self->song);
				song_file.convert_dos_utf8 = self->convert_dos_utf8;
				song_file.use_jbridge = self->use_jbridge;
				song_file.file = self->file;
				psy_strreset(&song_file.path, self->path);

				psyfile_seek(self->file, 0);
				modsongloader_init(&loader, &song_file);
				if ((status = modsongloader_load(&loader))) {
					psy_audio_songreader_errfile(self);
				}
				modsongloader_dispose(&loader);
				psy_audio_songfile_dispose(&song_file);
			}
#else
		status = PSY_ERRFILEFORMAT;
#endif			
		}
	psyfile_close(self->file);
	if (self->song) {
		if (!psy_audio_machines_at(psy_audio_song_machines(self->song),
			psy_audio_MASTER_INDEX)) {
			psy_audio_songreader_createmaster(self);
		}
		psy_audio_machines_solo(psy_audio_song_machines(self->song),
			self->machinesoloed);
		psy_audio_machines_endfilemode(psy_audio_song_machines(self->song));
	}
	}
 else {
		status = PSY_ERRFILEFORMAT;
		psy_audio_songreader_errfile(self);
		}
		if (self->logger) {
			psy_logger_progress(self->logger, PSY_PROGRESS_STATE_END);
			psy_logger_output(self->logger, "ready\n");
		}
		return status;
}

void psy_audio_songreader_message(psy_audio_SongReader* self, const char* text)
{
	if (self->logger) {
		psy_logger_output(self->logger, text);
	}
}

void psy_audio_songreader_errfile(psy_audio_SongReader* self)
{
	if (self->logger) {
		psy_logger_error(self->logger, strerror(self->err));
		psy_logger_error(self->logger, "\n");
	}
	self->err = errno;
}

void psy_audio_songreader_createmaster(psy_audio_SongReader* self)
{
	psy_audio_machines_insertmaster(psy_audio_song_machines(self->song),
		psy_audio_machinefactory_make_machine(self->song->machine_factory,
			psy_audio_MASTER, NULL, UINTPTR_MAX));
}

void psy_audio_songreader_use_jbridge(psy_audio_SongReader* self)
{
	self->use_jbridge = TRUE;
	if (self->song) {
		self->song->machine_factory->use_jbridge = self->use_jbridge;
	}
}