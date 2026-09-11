// This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
// copyright 2000-2021 members of the psycle project http://psycle.sourceforge.net

#include "../../detail/prefix.h"

#include "wavsongio.h"

#include "instrument.h"
#include "machinefactory.h"
#include "sample.h"
#include "song.h"
#include "songio.h"

#include <stdlib.h>
#include <string.h>

void psy_audio_wav_songio_load(psy_audio_SongFile* self)
{	
	psy_audio_Sample* sample;
	psy_audio_Instrument* instrument;	
	psy_audio_SampleIndex index;	
	
	sample = psy_audio_sample_alloc_init(0);
	psy_audio_sample_load(sample, self->path);	
	index = psy_audio_sampleindex_make(0, 0);
	psy_audio_samples_insert(psy_audio_song_samples(self->song), sample, index);
	instrument = psy_audio_instrument_allocinit();
	psy_audio_instrument_set_name(instrument, psy_audio_sample_name(sample));
	psy_audio_instrument_setindex(instrument, index.slot);
	psy_audio_instruments_insert(psy_audio_song_instruments(self->song), instrument,
		psy_audio_instrumentindex_make(0, index.slot));
	{
		psy_audio_Pattern* pattern;
		psy_audio_PatternEvent patternevent;
		psy_audio_PatternEntry entry;                                  							

		psy_audio_patternevent_clear(&patternevent);
		patternevent.note = 48;
		pattern = psy_audio_pattern_alloc_init();
		psy_audio_patternentry_init(&entry);
		psy_audio_patternentry_set_event(&entry, patternevent, 0);
		psy_audio_pattern_insert(pattern, 0, 0, psy_dsp_beatpos_zero(),
			&entry);
		psy_audio_patternentry_dispose(&entry);
		psy_audio_pattern_set_length(pattern,
			psy_dsp_beatpos_make_real(
				((sample->numframes / (double)sample->samplerate) / 60 * 125),
				psy_dsp_DEFAULT_PPQ));
		psy_audio_patterns_insert(psy_audio_song_patterns(self->song), 0, pattern);
		psy_audio_song_set_num_song_tracks(self->song, 1);
	}				
	psy_audio_sequence_append_track(psy_audio_song_sequence(self->song),
		psy_audio_sequencetrack_alloc_init());		
	psy_audio_sequence_insert(psy_audio_song_sequence(self->song),
		psy_audio_orderindex_make(0, 0), 0);	
	{
		psy_audio_Machine* machine;

		machine = psy_audio_machinefactory_make_machine(
			self->song->machine_factory, psy_audio_SAMPLER, "",
			psy_INDEX_INVALID);
		if (machine) {
			psy_audio_machines_insert(psy_audio_song_machines(self->song), 0, machine);
			psy_audio_machines_connect(psy_audio_song_machines(self->song),
				psy_audio_wire_make(0, psy_audio_MASTER_INDEX));
		}
	}
}

void psy_audio_wav_songio_save(psy_audio_SongFile* self)
{

}


