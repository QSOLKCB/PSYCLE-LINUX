/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_audio_SONGIO_H
#define psy_audio_SONGIO_H

/* local */
#include "instruments.h"
#include "wire.h"
/* container */
#include <hashtbl.h>
#include <logger.h>
#include <properties.h>
/* file */
#include <fileio.h>


#ifdef __cplusplus
extern "C" {
#endif

struct psy_audio_Instrument;
struct psy_audio_Song;


/*!
** @struct psy_audio_SongFile
** @brief Loads and saves songs and instruments
*/

typedef struct psy_audio_SongFile {
	struct psy_audio_Song* song;	
	struct PsyFile* file;
	int err;	
	psy_Logger* logger;
	char* path;
	uintptr_t machinesoloed;
	/* 
	** MachineID X [psy_Table* LegacyWires
	** (ConnectionID X psy_audio_LegacyWire*)]
	*/
	psy_audio_LegacyWires* legacywires;
	bool convert_dos_utf8;
	bool use_jbridge;
} psy_audio_SongFile;

void psy_audio_songfile_init(psy_audio_SongFile*);
void psy_audio_songfile_init_song(psy_audio_SongFile*, struct psy_audio_Song*);
void psy_audio_songfile_dispose(psy_audio_SongFile*);

void psy_audio_songfile_set_logger(psy_audio_SongFile*, psy_Logger*);
int psy_audio_songfile_song_info(psy_audio_SongFile*, const char* path);
int psy_audio_songfile_save(psy_audio_SongFile*, const char* path);
int psy_audio_songfile_exportmodule(psy_audio_SongFile*, const char* path);
int psy_audio_songfile_export_midi_file(psy_audio_SongFile*, const char* path);
int psy_audio_songfile_export_ly_file(psy_audio_SongFile*, const char* path);
int psy_audio_songfile_save_instrument(psy_audio_SongFile*,
	const char* filename, struct psy_audio_Instrument*);
int psy_audio_songfile_load_instrument(psy_audio_SongFile*, const char* path,
	psy_audio_InstrumentIndex);
void psy_audio_songfile_warn(psy_audio_SongFile*, const char* text);
void psy_audio_songfile_message(psy_audio_SongFile*, const char* text);
const char* psy_audio_songfile_loadfilter(void);
const char* psy_audio_songfile_standardloadfilter(void);
const char* psy_audio_songfile_savefilter(void);
const char* psy_audio_songfile_standardsavefilter(void);
const char* psy_audio_songfile_instloadfilter(void);
const char* psy_audio_songfile_standardinstloadfilter(void);
const char* psy_audio_songfile_instsavefilter(void);
const char* psy_audio_songfile_standardinstsavefilter(void);
void psy_audio_songfile_errfile(psy_audio_SongFile*);
void psy_audio_songfile_createmaster(psy_audio_SongFile*);
void psy_audio_songfile_use_jbridge(psy_audio_SongFile*);

INLINE PsyFile* psy_audio_songfile_psyfile(psy_audio_SongFile* self)
{
	return self->file;
}


/*!
** @struct psy_audio_SongReader
** @brief Load a song
*/

typedef struct psy_audio_SongReader {
	struct psy_audio_Song* song;
	struct PsyFile* file;
	int err;
	psy_Logger* logger;
	char* path;
	uintptr_t machinesoloed;
	/*
	** MachineID X [psy_Table* LegacyWires
	** (ConnectionID X psy_audio_LegacyWire*)]
	*/
	psy_audio_LegacyWires* legacywires;
	bool convert_dos_utf8;
	bool use_jbridge;
} psy_audio_SongReader;

void psy_audio_songreader_init(psy_audio_SongReader*, struct psy_audio_Song*,
	psy_Logger*, bool use_jbridge);
void psy_audio_songreader_dispose(psy_audio_SongReader*);

void psy_audio_songreader_set_logger(psy_audio_SongReader*, psy_Logger*);
int psy_audio_songreader_load(psy_audio_SongReader*, const char* path);
void psy_audio_songreader_message(psy_audio_SongReader*, const char* text);
void psy_audio_songreader_errfile(psy_audio_SongReader*);
void psy_audio_songreader_createmaster(psy_audio_SongReader*);
void psy_audio_songreader_use_jbridge(psy_audio_SongReader*);

#ifdef __cplusplus
}
#endif

#endif /* psy_audio_SONGIO_H */
