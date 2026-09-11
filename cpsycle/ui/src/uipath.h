/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_ui_PATH_H
#define psy_ui_PATH_H

/* local */
#include "uicolour.h"
#include "uigeometry.h"
/* container */
#include <list.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef enum psy_ui_PathCmdType {
	psy_ui_PATHCMD_TYPE_START = 0,
	psy_ui_PATHCMD_TYPE_MOVE_TO = 1,
	psy_ui_PATHCMD_TYPE_LINE_TO = 2,
	psy_ui_PATHCMD_TYPE_HORIZONTAL_LINE_TO = 3,
	psy_ui_PATHCMD_TYPE_VERTICAL_LINE_TO = 4,
	psy_ui_PATHCMD_TYPE_CURVE_TO = 5,
	psy_ui_PATHCMD_TYPE_SMOOTH_CURVE_TO = 6,
	psy_ui_PATHCMD_TYPE_QUADRATIC_BEZIER_CURVE = 7,
	psy_ui_PATHCMD_TYPE_SMOOTH_BEZIER_CURVE = 8,
	psy_ui_PATHCMD_TYPE_ELLIPTICAL_ARC = 9,
	psy_ui_PATHCMD_TYPE_CIRCLE = 10,
	psy_ui_PATHCMD_TYPE_CLOSE = 11,	
} psy_ui_PathCmdType;


/*! @struct psy_ui_PathCmd */

typedef struct psy_ui_PathCmd {
	psy_ui_PathCmdType type;
	psy_ui_RealPoint p1;
	psy_ui_RealPoint p2;
	psy_ui_RealPoint p3;
} psy_ui_PathCmd;

psy_ui_PathCmd* psy_ui_pathcmd_alloc(void);

INLINE psy_ui_PathCmd psy_ui_pathcmd_make_move_to(psy_ui_RealPoint pt)
{
	psy_ui_PathCmd rv;

	rv.type = psy_ui_PATHCMD_TYPE_MOVE_TO;
	rv.p1 = pt;
	rv.p2 = psy_ui_realpoint_zero();
	rv.p3 = psy_ui_realpoint_zero();
	return rv;
}

INLINE psy_ui_PathCmd psy_ui_pathcmd_make_line_to(psy_ui_RealPoint pt)
{
	psy_ui_PathCmd rv;

	rv.type = psy_ui_PATHCMD_TYPE_LINE_TO;
	rv.p1 = pt;
	rv.p2 = psy_ui_realpoint_zero();
	rv.p3 = psy_ui_realpoint_zero();
	return rv;
}

INLINE psy_ui_PathCmd psy_ui_pathcmd_make_curve_to(psy_ui_RealPoint c1,
	psy_ui_RealPoint c2, psy_ui_RealPoint pt)
{
	psy_ui_PathCmd rv;

	rv.type = psy_ui_PATHCMD_TYPE_CURVE_TO;
	rv.p1 = c1;
	rv.p2 = c2;
	rv.p3 = pt;
	return rv;
}

INLINE psy_ui_PathCmd psy_ui_pathcmd_make_circle(psy_ui_RealPoint pt, double r)
{
	psy_ui_PathCmd rv;

	rv.type = psy_ui_PATHCMD_TYPE_CIRCLE;
	rv.p1 = pt;
	rv.p2.x = r;	
	return rv;
}


INLINE psy_ui_PathCmd psy_ui_pathcmd_make_close(void)
{
	psy_ui_PathCmd rv;

	rv.type = psy_ui_PATHCMD_TYPE_CLOSE;
	rv.p1 = psy_ui_realpoint_zero();
	rv.p2 = psy_ui_realpoint_zero();
	rv.p3 = psy_ui_realpoint_zero();
	return rv;
}


/*! @struct psy_ui_Path */

typedef struct psy_ui_Path {
	/*! @internal */
	psy_List* elements;
	psy_ui_Colour fill;
	psy_ui_Colour stroke;
	psy_ui_RealPoint cp;
} psy_ui_Path;

void psy_ui_path_init(psy_ui_Path*);
void psy_ui_path_dispose(psy_ui_Path*);

psy_ui_Path* psy_ui_path_alloc(void);
psy_ui_Path* psy_ui_path_alloc_init(void);

void psy_ui_path_copy(psy_ui_Path*, const psy_ui_Path* other);
psy_ui_Path* psy_ui_path_clone(const psy_ui_Path*);
void psy_ui_path_clear(psy_ui_Path*);

void psy_ui_path_add(psy_ui_Path*, psy_ui_PathCmd);
void psy_ui_path_move_to(psy_ui_Path*, psy_ui_RealPoint);
void psy_ui_path_line_to(psy_ui_Path*, psy_ui_RealPoint);
void psy_ui_path_close(psy_ui_Path*);
bool psy_ui_path_closed(const psy_ui_Path*);
void psy_ui_path_set_stroke(psy_ui_Path*, psy_ui_Colour);
void psy_ui_path_set_fill(psy_ui_Path*, psy_ui_Colour);
void psy_ui_path_curve_to(psy_ui_Path*, psy_ui_RealPoint c1, psy_ui_RealPoint c2,
	psy_ui_RealPoint);
void psy_ui_path_circle(psy_ui_Path*, psy_ui_RealPoint pt, double r);
intptr_t psy_ui_path_parse(psy_ui_Path*, const char* str);

#ifdef __cplusplus
}
#endif

#endif /* psy_ui_PATH_H */
