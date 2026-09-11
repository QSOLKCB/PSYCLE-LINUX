/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_audio_SEQUENCECMDS_H
#define psy_audio_SEQUENCECMDS_H

#ifdef __cplusplus
extern "C" {
#endif

/* audio */
#include "sequence.h"

/*
** psy_audio_SequenceInsertCommand
*/

typedef struct psy_audio_SequenceInsertCommand {
	/*! @extends  */
	psy_Command command;
	psy_audio_Sequence* sequence;
	psy_audio_SequenceSelection* selection;
	psy_audio_OrderIndex index;
	uintptr_t patidx;
	psy_audio_SequenceSelection restoreselection;	
} psy_audio_SequenceInsertCommand;

psy_audio_SequenceInsertCommand* psy_audio_sequenceinsertcommand_alloc(
	psy_audio_Sequence*, psy_audio_SequenceSelection* selection,
	psy_audio_OrderIndex, uintptr_t patidx);

/*
** psy_audio_SequenceSampleInsertCommand
*/

typedef struct psy_audio_SequenceSampleInsertCommand {
	/*! @extends  */
	psy_Command command;
	psy_audio_Sequence* sequence;
	psy_audio_SequenceSelection* selection;
	psy_audio_OrderIndex index;
	psy_audio_SampleIndex sampleindex;
	psy_audio_SequenceSelection restoreselection;
} psy_audio_SequenceSampleInsertCommand;

psy_audio_SequenceSampleInsertCommand* psy_audio_sequencesampleinsertcommand_alloc(
	psy_audio_Sequence*, psy_audio_SequenceSelection* selection,
	psy_audio_OrderIndex, psy_audio_SampleIndex);


/*
** psy_audio_SequenceMarkerInsertCommand
*/

typedef struct psy_audio_SequenceMarkerInsertCommand {
	/*! @extends  */
	psy_Command command;
	psy_audio_Sequence* sequence;
	psy_audio_SequenceSelection* selection;
	psy_audio_OrderIndex index;
	char* text;
	psy_audio_SequenceSelection restoreselection;
} psy_audio_SequenceMarkerInsertCommand;

psy_audio_SequenceMarkerInsertCommand* psy_audio_sequencemarkerinsertcommand_alloc(
	psy_audio_Sequence*, psy_audio_SequenceSelection* selection,
	psy_audio_OrderIndex, const char* text);

/*
** psy_audio_SequenceRemoveCommand
*/
typedef struct psy_audio_SequenceRemoveCommand {
	/*! @extends  */
	psy_Command command;
	psy_audio_Sequence* sequence;
	psy_audio_SequenceSelection* selection;		
	psy_audio_SequenceSelection restoreselection;
	psy_audio_Sequence restoresequence;	
} psy_audio_SequenceRemoveCommand;

psy_audio_SequenceRemoveCommand* psy_audio_sequenceremovecommand_alloc(
	psy_audio_Sequence*, psy_audio_SequenceSelection* selection);

typedef struct psy_audio_SequenceClearCommand {
	/*! @extends  */
	psy_Command command;
	psy_audio_Sequence* sequence;
	psy_audio_SequenceSelection* selection;		
	psy_audio_SequenceSelection restoreselection;
	psy_audio_Sequence restoresequence;
} psy_audio_SequenceClearCommand;

psy_audio_SequenceClearCommand* psy_audio_sequenceclearcommand_alloc(
	psy_audio_Sequence*, psy_audio_SequenceSelection* selection);

typedef struct psy_audio_SequenceChangePatternCommand {
	/*! @extends  */
	psy_Command command;
	psy_audio_Sequence* sequence;
	psy_audio_SequenceSelection* selection;
	psy_audio_SequenceSelection restoreselection;
	psy_audio_Sequence restoresequence;
	intptr_t step;
	bool success;
} psy_audio_SequenceChangePatternCommand;

psy_audio_SequenceChangePatternCommand* psy_audio_sequencechangepatterncommand_alloc(
	psy_audio_Sequence*, psy_audio_SequenceSelection* selection,
	intptr_t step);



/* psy_audio_PatternRemoveCmd */
typedef struct psy_audio_PatternRemoveCmd {
	/*! @extends  */
	psy_Command command;	
} psy_audio_PatternRemoveCmd;

void psy_audio_patternremovecmd_init(psy_audio_PatternRemoveCmd*);

INLINE psy_Command* psy_audio_patternremovecmd_base(
	psy_audio_PatternRemoveCmd* self)
{
	return &self->command;
}

psy_audio_PatternRemoveCmd* psy_audio_patternremovecmd_alloc_init(void);

/* psy_audio_PatternTransposeCmd */
typedef struct psy_audio_PatternTransposeCmd {
	/*! @extends  */
	psy_Command command;
	psy_Table nodemark;
	intptr_t offset;
} psy_audio_PatternTransposeCmd;

void psy_audio_patterntransposecmd_init(psy_audio_PatternTransposeCmd*,
	intptr_t offset);
	
psy_audio_PatternTransposeCmd* psy_audio_patterntransposecmd_alloc_init(
	intptr_t offset);
	
INLINE psy_Command* psy_audio_patterntransposecmd_base(
	psy_audio_PatternTransposeCmd* self)
{
	return &self->command;
}

/*! 
** @struct psy_audio_PatternCopyCmd
*/
typedef struct psy_audio_PatternCopyCmd {
	/*! @implements */
	psy_Command command;
	psy_audio_Pattern* dest;
	psy_audio_PatternNode* prev;
	psy_dsp_beatpos_t abs_offset;
	intptr_t trackoffset;
} psy_audio_PatternCopyCmd;

void psy_audio_patterncopycmd_init(psy_audio_PatternCopyCmd*,
	psy_audio_Pattern* dest,
	psy_dsp_beatpos_t abs_offset,
	intptr_t trackoffset);


typedef bool (*psy_audio_fp_matches)(const uintptr_t test,
	const uintptr_t reference);
typedef uintptr_t (*psy_audio_fp_replacewith)(const uintptr_t current,
	const uintptr_t newval);

typedef enum psy_audio_PatternSearchSelector {
	psy_audio_PATTERNSEARCHSELECTOR_EMPTY     = 1001,
	psy_audio_PATTERNSEARCHSELECTOR_NON_EMPTY = 1002,
	psy_audio_PATTERNSEARCHSELECTOR_ALL       = 1003
} psy_audio_PatternSearchSelector;

typedef enum psy_audio_PatternReplaceSelector {
	psy_audio_PATTERNREPLACESELECTOR_SET_EMPTY = 1001,
	psy_audio_PATTERNREPLACESELECTOR_KEEP      = 1002,	
} psy_audio_PatternReplaceSelector;

typedef struct psy_audio_PatternSearchReplaceMode
{	
	psy_audio_fp_matches notematcher;
	psy_audio_fp_matches instmatcher;
	psy_audio_fp_matches machmatcher;
	psy_audio_fp_replacewith notereplacer;
	psy_audio_fp_replacewith instreplacer;
	psy_audio_fp_replacewith machreplacer;
	psy_audio_fp_replacewith tweakreplacer;
	uintptr_t notereference;
	uintptr_t instreference;
	uintptr_t machreference;
	uintptr_t notereplace;
	uintptr_t instreplace;
	uintptr_t machreplace;
	uintptr_t tweakreplace;
} psy_audio_PatternSearchReplaceMode;


psy_audio_PatternSearchReplaceMode
psy_audio_patternsearchreplacemode_make(
	int searchnote, int searchinst, int searchmach, int replnote,
	int replinst, int replmach, bool repltweak);

INLINE bool psy_audio_patternsearchreplacemode_match(const 
	psy_audio_PatternSearchReplaceMode* self,
	psy_audio_PatternEvent ev)
{
	return (self->notematcher(ev.note, self->notereference) &&
		self->instmatcher(ev.inst, self->instreference) &&
		self->machmatcher(ev.mach, self->machreference));
}

INLINE bool psy_audio_patternsearchreplacemode_matchesall(uintptr_t t, uintptr_t r)
{
	return TRUE;
}

INLINE bool psy_audio_patternsearchreplacemode_matchesempty(uintptr_t test,
	uintptr_t r)
{
	return test == 0xff;
}

INLINE bool psy_audio_patternsearchreplacemode_matchesnonempty(uintptr_t test,
	uintptr_t r)
{
	return test != 0xff;
}

INLINE bool psy_audio_patternsearchreplacemode_matchesequal(uintptr_t test,
	uintptr_t reference)
{
	return test == reference;
}

INLINE uintptr_t psy_audio_patternsearchreplacemode_replacewithempty(
	uintptr_t c, uintptr_t nv)
{
	return 0xff;
}

INLINE uintptr_t psy_audio_patternsearchreplacemode_replacewithcurrent(
	uintptr_t current, uintptr_t nv)
{
	return current;
}

INLINE uintptr_t psy_audio_patternsearchreplacemode_replacewithnewval(
	uintptr_t c, uintptr_t newval)
{	
	return newval;
}
	
	
/*! 
** @struct psy_audio_PatternSearchCmdCommand
*/
typedef struct psy_audio_PatternSearchCmd {
	/*! @extends */
	psy_Command command;
	/*! @internal */
	psy_audio_PatternSearchReplaceMode search_pattern;	
	bool replace;
	psy_audio_SequenceCursor result;
} psy_audio_PatternSearchCmd;

void psy_audio_patternsearchcmd_init(psy_audio_PatternSearchCmd*,
	psy_audio_PatternSearchReplaceMode, bool replace);

psy_audio_PatternSearchCmd* psy_audio_patternsearchcmd_alloc_init(
	psy_audio_PatternSearchReplaceMode, bool replace);

psy_audio_SequenceCursor psy_audio_patternsearchcmd_result(
	const psy_audio_PatternSearchCmd*);

INLINE psy_Command* psy_audio_patternsearchcmd_base(
	psy_audio_PatternSearchCmd* self)
{
	return &self->command;
}

typedef struct psy_audio_SwingFill {
	int tempo;
	int width;
	double variance;
	double phase;
	bool offset;
} psy_audio_SwingFill;

/*! 
** @struct psy_audio_PatternSwingFillCmdCommand
*/
typedef struct psy_audio_PatternSwingFillCmd {
	/*! @extends */
	psy_Command command;
	/*! @internal */
	psy_audio_SwingFill swingfill;	
	psy_audio_SequenceTrack* track;
	double twopi;			
	double var;
	double step;
	double index;
	double dcoffs;
} psy_audio_PatternSwingFillCmd;

void psy_audio_patternswingfillcmd_init(psy_audio_PatternSwingFillCmd*,
	psy_audio_SwingFill);

psy_audio_PatternSwingFillCmd* psy_audio_patternswingfillcmd_alloc_init(
	psy_audio_SwingFill);

INLINE psy_Command* psy_audio_patternswingfillcmd_base(
	psy_audio_PatternSwingFillCmd* self)
{
	return &self->command;
}

/*! 
** @struct psy_audio_LinearInterpolate
*/
typedef struct psy_audio_LinearInterpolate {
	double step;
	psy_dsp_beatpos_t beginoffset;
	intptr_t startval;
	intptr_t endval;
	int twk_type;
	int twk_inst;
	int twk_mac;	
	uintptr_t minval;
	uintptr_t maxval;
} psy_audio_LinearInterpolate;

psy_audio_LinearInterpolate psy_audio_linearinterpolate_make(
	psy_audio_Sequence*, psy_audio_BlockSelection,
	uintptr_t minval, uintptr_t maxval,
	int twk_type, int twk_inst, int twk_mac);


/*! 
** @struct psy_audio_PatternLinearInterpolateCmdCommand
*/
typedef struct psy_audio_PatternLinearInterpolateCmd {
	/*! @extends */
	psy_Command command;
	/*! @internal */			
	psy_audio_LinearInterpolate interpolate;
} psy_audio_PatternLinearInterpolateCmd;

void psy_audio_patternlinearinterpolatecmd_init(
	psy_audio_PatternLinearInterpolateCmd*,
	psy_audio_LinearInterpolate);

psy_audio_PatternLinearInterpolateCmd*
psy_audio_patternlinearinterpolatecmd_alloc_init(
	psy_audio_LinearInterpolate);

INLINE psy_Command* psy_audio_patternlinearinterpolatecmd_base(
	psy_audio_PatternLinearInterpolateCmd* self)
{
	return &self->command;
}

/*! 
** @struct psy_audio_HermiteInterpolate
*/
typedef struct psy_audio_HermiteInterpolate {	
	psy_dsp_beatpos_t begin;	
	intptr_t val0;
	intptr_t val1;
	intptr_t val2;
	intptr_t val3;
	intptr_t distance;	
} psy_audio_HermiteInterpolate;

psy_audio_HermiteInterpolate psy_audio_hermiteinterpolate_make(
	psy_audio_Sequence*, psy_audio_BlockSelection);


/*! 
** @struct psy_audio_PatternHermiteInterpolateCmdCommand
*/
typedef struct psy_audio_PatternHermiteInterpolateCmd {
	/*! @extends */
	psy_Command command;
	/*! @internal */			
	psy_audio_HermiteInterpolate interpolate;
} psy_audio_PatternHermiteInterpolateCmd;

void psy_audio_patternhermiteinterpolatecmd_init(
	psy_audio_PatternHermiteInterpolateCmd*,
	psy_audio_HermiteInterpolate);

psy_audio_PatternHermiteInterpolateCmd*
psy_audio_patternhermiteinterpolatecmd_alloc_init(
	psy_audio_HermiteInterpolate);

INLINE psy_Command* psy_audio_patternhermiteinterpolatecmd_base(
	psy_audio_PatternHermiteInterpolateCmd* self)
{
	return &self->command;
}


/*!
** @struct psy_audio_PatternPasteCmd
*/
typedef struct psy_audio_PatternPasteCmd {
	/*! @extends */
	psy_Command command;
	/*! @internal */
	bool mix;
	psy_audio_Pattern* src;
	psy_audio_SequenceCursor begin;
	psy_audio_PatternNode* curr;
} psy_audio_PatternPasteCmd;

void psy_audio_patternpastecmd_init(psy_audio_PatternPasteCmd*,
	psy_audio_Pattern* src, psy_audio_SequenceCursor begin,
	bool mix);

psy_audio_PatternPasteCmd*
psy_audio_patternpastecmd_alloc_init(psy_audio_Pattern* src,
	psy_audio_SequenceCursor begin, bool mix);

INLINE psy_Command* psy_audio_patternpastecmd_base(
	psy_audio_PatternPasteCmd* self)
{
	return &self->command;
}


/*! 
** @struct psy_audio_PatternBlockCmd
*/
typedef struct psy_audio_PatternBlockCmd {
	/*! @extends */
	psy_Command command;
	psy_audio_BlockSelection block;
	bool grid;
	psy_audio_Sequence* sequence;
	psy_audio_Pattern restore;
	/*! @internal */
	psy_Command* cmd;
} psy_audio_PatternBlockCmd;

void psy_audio_patternblockcmd_init(psy_audio_PatternBlockCmd*,
	psy_audio_Sequence*, psy_audio_BlockSelection, bool grid,
	psy_Command*);
	
psy_audio_PatternBlockCmd* psy_audio_patternblockcmd_alloc_init(
	psy_audio_Sequence*, psy_audio_BlockSelection, bool grid,
	psy_Command*);

INLINE psy_Command* psy_audio_patternblockcmd_base(
	psy_audio_PatternBlockCmd* self)
{
	return &self->command;
}


/* Pattern cmds */

/* InsertCommand */
typedef struct InsertCommand {
	/*! @extends  */
	psy_Command command;
	/*! @internal */
	psy_audio_SequenceCursor cursor;
	psy_audio_SequenceCursor cursor_at_noteoff;
	psy_audio_Pattern* pattern;
	psy_audio_PatternEvent event;
	psy_audio_PatternEvent oldevent;
	psy_audio_PatternEvent oldevent_at_noteoff;
	psy_dsp_beatpos_t duration;
	/* references */
	psy_audio_Sequence* sequence;
} InsertCommand;

InsertCommand* insertcommand_allocinit(psy_audio_Pattern*,
	psy_audio_SequenceCursor, psy_audio_PatternEvent,
	psy_dsp_beatpos_t duration, /* 0 : endless/no noteoff */
	psy_audio_Sequence*);

/* RemoveCommand */
typedef struct RemoveCommand {
	/*! @extends  */
	psy_Command command;
	psy_audio_SequenceCursor cursor;
	psy_audio_Pattern* pattern;
	psy_audio_PatternEvent event;
	psy_audio_PatternEvent oldevent;
	/* references */
	psy_audio_Sequence* sequence;
} RemoveCommand;

RemoveCommand* removecommand_allocinit(psy_audio_Pattern*,
	psy_audio_SequenceCursor, psy_audio_Sequence*);


#ifdef __cplusplus
}
#endif

#endif /* psy_audio_SEQUENCECMDS_H */
