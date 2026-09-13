/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "song.h"
/* local */
#include "constants.h"
#include "machinefactory.h"
#include "songio.h"
/* platform */
#include "../../detail/portable.h"


/* psy_audio_SongProperties */

/* prototypes */
static void song_init_properties(psy_audio_Song*);
static void song_init_machines(psy_audio_Song*);
static void song_init_patterns(psy_audio_Song*);
static void song_init_sequence(psy_audio_Song*);

/* implementation */
void psy_audio_songproperties_init(psy_audio_SongProperties* self,
	const char* title,
	const char* credits,
	const char* comments)
{
	assert(self);

	self->title_ = psy_strdup(title);
	self->credits_ = psy_strdup(credits);
	self->comments_ = psy_strdup(comments);
	self->bpm_ = (double) 125.0;
	self->lpb_ = 4;	
	self->octave_ = 4;
	self->tpb_ = 24;
	self->extra_ticks_per_beat_ = 0;
	self->sampler_index_ = 0x3E;
	self->auto_note_off_ = FALSE;
}

void psy_audio_songproperties_init_all(psy_audio_SongProperties* self,
	const char* title, const char* credits, const char* comments,
	int octave,
	uintptr_t lpb,
	int tpb,
	int extra_ticks_per_beat,
	double bpm,
	uintptr_t sampler_index,
	bool auto_note_off)
{
	assert(self);

	self->title_ = psy_strdup(title);
	self->credits_ = psy_strdup(credits);
	self->comments_ = psy_strdup(comments);
	self->lpb_ = lpb;	
	self->octave_ = octave;
	self->tpb_ = tpb;
	self->extra_ticks_per_beat_ = extra_ticks_per_beat;
	self->bpm_ = bpm;
	self->sampler_index_ = sampler_index;
	self->auto_note_off_ = auto_note_off;
}

void psy_audio_songproperties_dispose(psy_audio_SongProperties* self)
{
	assert(self);

	free(self->title_);
	self->title_ = NULL;
	free(self->credits_);
	self->credits_ = NULL;
	free(self->comments_);
	self->comments_ = NULL;
}

void psy_audio_songproperties_copy(psy_audio_SongProperties* self,
	const psy_audio_SongProperties* other)
{
	assert(self);

	if (self != other) {
		psy_audio_songproperties_dispose(self);
		self->title_ = psy_strdup(other->title_);
		self->credits_ = psy_strdup(other->credits_);
		self->comments_ = psy_strdup(other->comments_);
		self->bpm_ = other->bpm_;
		self->lpb_ = other->lpb_;
		self->octave_ = other->octave_;
		self->tpb_ = other->tpb_;
		self->extra_ticks_per_beat_ = other->extra_ticks_per_beat_;
		self->sampler_index_ = other->sampler_index_;
		self->auto_note_off_ = other->auto_note_off_;
	}
}

void psy_audio_songproperties_set_title(psy_audio_SongProperties* self,
	const char* title)
{
	psy_strreset(&self->title_, title);
}

void psy_audio_songproperties_set_comments(psy_audio_SongProperties* self,
	const char* comments)
{
	psy_strreset(&self->comments_, comments);
}

void psy_audio_songproperties_set_credits(psy_audio_SongProperties* self,
	const char* credits)
{
	psy_strreset(&self->credits_, credits);
}


/* psy_audio_Song */

/* implementation */
void psy_audio_song_init(psy_audio_Song* self, psy_audio_MachineFactory*
	machine_factory)
{
	assert(self);

	self->machine_factory = machine_factory;
	if (self->machine_factory && self->machine_factory->machinecallback) {
		psy_audio_machinecallback_set_song(
			self->machine_factory->machinecallback,
			self);
	}	
	psy_audio_songproperties_init(&self->properties_, "Untitled", "Unnamed",
		"No Comments");	
	song_init_machines(self);
	song_init_patterns(self);
	song_init_sequence(self);
	psy_audio_samples_init(&self->samples_);
	psy_audio_instruments_init(&self->instruments_);	
}

void song_init_machines(psy_audio_Song* self)
{
	assert(self);

	psy_audio_machines_init(&self->machines_);
	if (self->machine_factory) {
		psy_audio_machines_insertmaster(&self->machines_,
			psy_audio_machinefactory_make_machine(self->machine_factory,
				psy_audio_MASTER, NULL, psy_INDEX_INVALID));
		psy_audio_machines_select(&self->machines_, psy_audio_MASTER_INDEX);
	}
}

void song_init_patterns(psy_audio_Song* self)
{
	assert(self);

	psy_audio_patterns_init(&self->patterns_);
	psy_audio_patterns_insert(&self->patterns_, 0,
		psy_audio_pattern_alloc_init());
}

void song_init_sequence(psy_audio_Song* self)
{
	assert(self);

	psy_audio_sequence_init(&self->sequence_, &self->patterns_, &self->samples_);
	psy_audio_sequence_append_track(&self->sequence_,
		psy_audio_sequencetrack_alloc_init());		
	psy_audio_sequence_insert(&self->sequence_,
		psy_audio_orderindex_make(0, 0), 0);
}

void psy_audio_song_dispose(psy_audio_Song* self)
{
	assert(self);

	psy_audio_songproperties_dispose(&self->properties_);
	psy_audio_machines_dispose(&self->machines_);
	psy_audio_sequence_dispose(&self->sequence_);
	psy_audio_patterns_dispose(&self->patterns_);
	psy_audio_samples_dispose(&self->samples_);
	psy_audio_instruments_dispose(&self->instruments_);		
}

psy_audio_Song* psy_audio_song_alloc(void)
{
	return (psy_audio_Song*)malloc(sizeof(psy_audio_Song));
}

psy_audio_Song* psy_audio_song_alloc_init(psy_audio_MachineFactory*
	machine_factory)
{
	psy_audio_Song* rv;

	rv = psy_audio_song_alloc();
	if (rv) {
		psy_audio_song_init(rv, machine_factory);
	}
	return rv;
}

void psy_audio_song_deallocate(psy_audio_Song* self)
{
	assert(self);
	
	psy_audio_song_dispose(self);
	free(self);	
}

void psy_audio_song_clear(psy_audio_Song* self)
{
	assert(self);

	psy_audio_sequence_clear(&self->sequence_);
	psy_audio_patterns_clear(&self->patterns_);
	psy_audio_machines_clear(&self->machines_);
}

void psy_audio_song_set_properties(psy_audio_Song* self,
	const psy_audio_SongProperties* properties)
{	
	assert(self);

	psy_audio_songproperties_copy(&self->properties_, properties);
}

void psy_audio_song_set_bpm(psy_audio_Song* self, double bpm)
{
	assert(self);

	psy_audio_songproperties_set_bpm(&self->properties_, bpm);		
}

void psy_audio_song_insert_virtual_generator(psy_audio_Song* self,
	uintptr_t virtual_inst, uintptr_t mac_idx, uintptr_t inst_idx)
{
	assert(self);

	/*
	** && mac != NULL && (mac->_type == psy_audio_SAMPLER ||
	**    mac->_type == psy_audio_XMSAMPLER))
	*/
	if (virtual_inst >= MAX_MACHINES && virtual_inst < MAX_VIRTUALINSTS) {
		psy_audio_Machine* machine;
		
		machine = psy_audio_machines_at(&self->machines_, virtual_inst);
		if (machine) {
			psy_audio_machines_remove(&self->machines_, virtual_inst, FALSE);
		}
		machine = psy_audio_machinefactory_make_machine_from_path(
			self->machine_factory, psy_audio_VIRTUALGENERATOR, NULL,
			mac_idx, inst_idx);
		if (machine) {
			psy_audio_machine_set_edit_name(machine, "Virtual Generator");
			psy_audio_machines_insert(&self->machines_, virtual_inst, machine);
		}
	}
}
