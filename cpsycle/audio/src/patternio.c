/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "patternio.h"
/* local */
#include "pattern.h"
/* file */
#include <dir.h>
/* std */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define EVENT_SIZE 5

/* prototypes */
static void loadblock(FILE*, psy_audio_Pattern*, double bpl);
static void saveblock(FILE*, psy_audio_Pattern*, double bpl, uintptr_t songtracks);
/* implementation */
void psy_audio_patternio_load(psy_audio_Pattern* pattern, const psy_Path* path, double bpl)
{
	FILE* fp;

	if ((fp = fopen(psy_path_full(path), "rb"))) {
		loadblock(fp, pattern, bpl);
	}
	fclose(fp);
}

void loadblock(FILE* fp, psy_audio_Pattern* pattern, double bpl)
{
	int32_t nt, nl;

	fread(&nt, sizeof(int32_t), 1, fp);
	fread(&nl, sizeof(int32_t), 1, fp);	
	if ((nt > 0) && (nl > 0)) {	
		psy_audio_PatternNode* node = 0;		
		unsigned char* source;
		int track;
		int line;
		
		source = (unsigned char*)malloc(nl * nt * EVENT_SIZE);
		for (track = 0; track < nt; ++track) {
			for (line = 0; line < nl; ++line) {
				unsigned char* ptrack;

				ptrack = &source[(line * nt + track) * EVENT_SIZE];
				fread(ptrack, sizeof(char), EVENT_SIZE, fp);
			}
		}
		for (line = 0; line < nl; ++line) {
			for (track = 0; track < nt; ++track) {			
				/* if (l < MAX_LINES && t < MAX_TRACKS) */
					unsigned char* ptrack;
					psy_audio_PatternEvent event;
					double offset;					

					ptrack = &source[(line * nt + track) * EVENT_SIZE];
					offset = bpl * line;
					/*
					** Psy3 PatternEntry format
					** type				offset
					** uint8_t note;		0
					** uint8_t inst;		1
					** uint8_t mach;		2
					** uint8_t cmd;			3
					** uint8_t parameter;	4
					*/
					psy_audio_patternevent_clear(&event);					
					event.note = ptrack[0];
					event.inst = (ptrack[1] == 0xFF)
						? event.inst = psy_audio_NOTECOMMANDS_INST_EMPTY
						: ptrack[1];
					event.mach = (ptrack[2] == 0xFF)
						? event.mach = psy_audio_NOTECOMMANDS_EMPTY
						: ptrack[2];
					event.cmd = ptrack[3];
					event.parameter = ptrack[4];
					if (!psy_audio_patternevent_empty(&event)) {
						psy_audio_PatternEntry entry;
						
						psy_audio_patternentry_init(&entry);
						psy_audio_patternentry_set_event(&entry, event, 0);
						node = psy_audio_pattern_insert(pattern, node, track, 
							psy_dsp_beatpos_make_real(offset, psy_dsp_DEFAULT_PPQ),
							&entry);
						psy_audio_patternentry_dispose(&entry);							
					}				
			}
		}
		free(source);
	}
}

void psy_audio_patternio_save(psy_audio_Pattern* pattern, const psy_Path* path, double bpl, uintptr_t songtracks)
{
	FILE* fp;
	
	if ((fp = fopen(psy_path_full(path), "wb"))) {
		saveblock(fp, pattern, bpl, songtracks);
		fflush(fp);
		fclose(fp);
	}
}

void saveblock(FILE* fp, psy_audio_Pattern* pattern, double bpl, uintptr_t songtracks)
{	
	int nlines;		
	unsigned char* dest;
	uintptr_t track;
	int line;
	psy_audio_PatternNode* node;
	
	nlines = (int)(psy_dsp_beatpos_real(psy_audio_pattern_length(pattern)) / bpl + (double) 0.5f);
	fwrite(&songtracks, sizeof(int32_t), 1, fp);
	fwrite(&nlines, sizeof(int32_t), 1, fp);
	dest = (unsigned char*)malloc(nlines * songtracks * EVENT_SIZE);
	for (line = 0; line < nlines; ++line) {
		for (track = 0; track < songtracks; ++track) {
			unsigned char* ptrack;

			ptrack = &dest[(track * nlines + line) * EVENT_SIZE];
			ptrack[0] = 255;
			ptrack[1] = 255;
			ptrack[2] = 255;
			ptrack[3] = 0;
			ptrack[4] = 0;			
		}
	}
	for (node = psy_audio_pattern_begin(pattern); node != NULL;
			psy_audio_patternnode_next(&node)) {
		psy_audio_PatternEntry* entry;
		psy_audio_PatternEvent* ev;		
		unsigned char* ptrack;

		entry = psy_audio_patternnode_entry(node->entry);
		if (psy_audio_patternentry_track(entry) < songtracks) {
			ev = psy_audio_patternentry_front(entry);
			line = (int)(psy_dsp_beatpos_real(psy_audio_patternentry_offset(entry)) / bpl);
			track = psy_audio_patternentry_track(entry);
			ptrack = &dest[(track * nlines + line) * EVENT_SIZE];
			ptrack[0] = (uint8_t)(ev->note & 0xFF);
			ptrack[1] = (uint8_t)(ev->inst & 0xFF);
			ptrack[2] = (uint8_t)(ev->mach & 0xFF);
			ptrack[3] = (uint8_t)(ev->cmd & 0xFF);
			ptrack[4] = (uint8_t)(ev->parameter & 0xFF);
		}
	}
	fwrite(dest, sizeof(char), nlines * songtracks * EVENT_SIZE, fp);
	free(dest);
}
