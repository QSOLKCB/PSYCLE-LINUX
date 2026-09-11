/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(VIEWINDEX_H)
#define VIEWINDEX_H

#include "../../detail/psydef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define	VIEW_ID_MACHINES        2
#define	VIEW_ID_PATTERNS        3
#define	VIEW_ID_SAMPLES         4
#define VIEW_ID_INSTRUMENTS     5
#define	VIEW_ID_SONGPROPERTIES  6
#define	VIEW_ID_SETTINGS        7
#define	VIEW_ID_STYLEVIEW       8
#define	VIEW_ID_HELPVIEW        9
#define	VIEW_ID_RENDERVIEW		10
#define	VIEW_ID_FILEVIEW		11
#define	VIEW_ID_CONFIRM			12
#define	VIEW_ID_SCRIPTS         13

#define SECTION_ID_HELPVIEW_HELP			0
#define SECTION_ID_HELPVIEW_ABOUT			1

#define SECTION_ID_MACHINEVIEW_WIRES		0
#define	SECTION_ID_MACHINEVIEW_STACK		1
#define	SECTION_ID_MACHINEVIEW_NEWMACHINE	2
#define	SECTION_ID_MACHINEVIEW_PROPERTIES	3
#define	SECTION_ID_MACHINEVIEW_BANK_MANGER	4

#define SECTION_ID_PATTERNVIEW_TRACKER      0
#define	SECTION_ID_PATTERNVIEW_PIANO        1
#define	SECTION_ID_PATTERNVIEW_HSPLIT       2
#define	SECTION_ID_PATTERNVIEW_VSPLIT       3

#define SECTION_ID_GEAR_MACHINES      0
#define SECTION_ID_GEAR_PATTERNS      1
#define SECTION_ID_GEAR_INSTRUMENTS   2
#define SECTION_ID_GEAR_SAMPLES       3

#define SECTION_ID_SAMPLES_PROPERTIES  0
#define SECTION_ID_SAMPLES_IMPORT      1
#define SECTION_ID_SAMPLES_EDIT        2

#define SECTION_ID_SONGPROPERTIES_EDIT 0
#define SECTION_ID_SONGPROPERTIES_VIEW 1

#define SECTION_ID_FILESELECT 0
#define SECTION_ID_FILEVIEW 1

/* The patternview display modes */
typedef enum {
	PATTERN_DISPLAYMODE_TRACKER,					/* only tracker visible */
	PATTERN_DISPLAYMODE_PIANOROLL,					/* only pianoroll visible */
	PATTERN_DISPLAYMODE_TRACKER_PIANOROLL_VERTICAL,	/* both of them visible */
	PATTERN_DISPLAYMODE_TRACKER_PIANOROLL_HORIZONTAL,
	PATTERN_DISPLAYMODE_INVALID,
	PATTERN_DISPLAYMODE_NUM
} PatternDisplayMode;


/*!
** @struct ViewIndex
*/

typedef struct ViewIndex {
	uintptr_t id;
	uintptr_t section;
	uintptr_t option;
	uintptr_t seqpos;
} ViewIndex;

INLINE ViewIndex viewindex_make_all(uintptr_t id, uintptr_t section,
	uintptr_t option, uintptr_t seqpos)
{
	ViewIndex rv;

	rv.id = id;
	rv.section = section;	
	rv.option = option;
	rv.seqpos = seqpos;
	return rv;
}

INLINE ViewIndex viewindex_make(uintptr_t id)
{
	return viewindex_make_all(id, psy_INDEX_INVALID, psy_INDEX_INVALID,
		psy_INDEX_INVALID);
}

INLINE ViewIndex viewindex_make_section(uintptr_t id, uintptr_t section)
{
	return viewindex_make_all(id, section, psy_INDEX_INVALID,
		psy_INDEX_INVALID);
}

void viewindex_trace(const ViewIndex*);

#ifdef __cplusplus
}
#endif

#endif /* VIEWINDEX_H */
