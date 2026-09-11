/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "trackercmds.h"
#include "uidef.h"
/* ui */
/* eventdriver */
#include "../../driver/eventdriver.h"
/* platform */
#include "../../detail/portable.h"


static void set_cmd_all(psy_Property* cmds, uintptr_t cmd, uint32_t keycode,
	bool shift, bool ctrl, const char* key, const char* shorttext);
static void set_cmd(psy_Property* cmds, int cmd, uint32_t keycode, const char* key,
	const char* short_text);


void trackercmds_make_edit(psy_Property* parent)
{
	psy_Property* cmds;

	assert(parent);
	
	cmds = psy_property_set_text(psy_property_append_section(parent, "edit"),
		"Edit");	
	set_cmd(cmds, CMD_ROWINSERT, psy_ui_KEY_INSERT, "rowinsert", "ins row");
	set_cmd(cmds, CMD_ROWDELETE, psy_ui_KEY_BACK, "rowdelete", "del row");
	set_cmd(cmds, CMD_ROWCLEAR, psy_ui_KEY_DELETE, "rowclear", "clr row");
	set_cmd_all(cmds, CMD_BLOCKSTART,
		psy_ui_KEY_B, psy_SHIFT_OFF, psy_CTRL_ON,
		"blockstart", "sel start");
	set_cmd_all(cmds, CMD_BLOCKEND,
		psy_ui_KEY_E, psy_SHIFT_OFF, psy_CTRL_ON,
		"blockend", "sel end");
	set_cmd_all(cmds, CMD_BLOCKUNMARK,
		psy_ui_KEY_U, psy_SHIFT_OFF, psy_CTRL_ON,
		"blockunmark", "unmark");
	set_cmd_all(cmds, CMD_BLOCKCUT,
		psy_ui_KEY_X, psy_SHIFT_OFF, psy_CTRL_ON,
		"blockcut", "cut");
	set_cmd_all(cmds, CMD_BLOCKCOPY,
		psy_ui_KEY_C, psy_SHIFT_OFF, psy_CTRL_ON,
		"blockcopy", "copy");
	set_cmd_all(cmds, CMD_BLOCKPASTE,
		psy_ui_KEY_V, psy_SHIFT_OFF, psy_CTRL_ON,
		"blockpaste", "paste");
	set_cmd_all(cmds, CMD_BLOCKMIX,
		psy_ui_KEY_M, psy_SHIFT_OFF, psy_CTRL_ON,
		"blockmix", "mix");
	set_cmd_all(cmds, CMD_BLOCKINTERPOLATE,
		psy_ui_KEY_I, psy_SHIFT_OFF, psy_CTRL_ON,
		"interpolate", "ipol");
	set_cmd_all(cmds, CMD_BLOCKINTERPOLATECURVE,
		psy_ui_KEY_I, psy_SHIFT_ON, psy_CTRL_ON,
		"interpolatecurve", "ipolc");
	set_cmd_all(cmds, CMD_BLOCKSETMACHINE,
		psy_ui_KEY_G, psy_SHIFT_OFF, psy_CTRL_ON,
		"setmachine", "setmac");
	set_cmd_all(cmds, CMD_BLOCKSETINSTR,
		psy_ui_KEY_T, psy_SHIFT_OFF, psy_CTRL_ON,
		"setinstrument", "setins");
	set_cmd_all(cmds, CMD_BLOCKTRANSFORM,
		psy_ui_KEY_R, psy_SHIFT_OFF, psy_CTRL_ON,
		"blocktransform", "btrnsf");
	set_cmd_all(cmds, CMD_BLOCKSWINGFILL,
		psy_ui_KEY_J, psy_SHIFT_OFF, psy_CTRL_ON,
		"blockswingfill", "bswing");
	set_cmd_all(cmds, CMD_BLOCKTRKSWINGFILL,
		psy_ui_KEY_J, psy_SHIFT_ON, psy_CTRL_ON,
		"blocktrackswingfill", "btswing");
	set_cmd_all(cmds, CMD_BLOCKTRKSWINGFILL,
		psy_ui_KEY_J, psy_SHIFT_ON, psy_CTRL_ON,
		"blocktrackswingfill", "btswing");	
	set_cmd_all(cmds, CMD_PATTERN_IMPORT,
		0, psy_SHIFT_OFF, psy_CTRL_OFF,
		"patternimport", "psbim");
	set_cmd_all(cmds, CMD_PATTERN_EXPORT,
		0, psy_SHIFT_OFF, psy_CTRL_OFF,
		"patternexport", "psbex");	
	set_cmd_all(cmds, CMD_BLOCKDELETE,
		psy_ui_KEY_X, psy_SHIFT_ON, psy_CTRL_ON,
		"blockdelete", "blkdel");
	set_cmd_all(cmds, CMD_TRANSPOSEBLOCKINC,
		psy_ui_KEY_F12, psy_SHIFT_OFF, psy_CTRL_ON,
		"transposeblockinc", "Trsp+");
	set_cmd_all(cmds, CMD_TRANSPOSEBLOCKDEC,
		psy_ui_KEY_F11, psy_SHIFT_OFF, psy_CTRL_ON,
		"transposeblockdec", "Trsp-");
	set_cmd_all(cmds, CMD_TRANSPOSEBLOCKINC12,
		psy_ui_KEY_F12, psy_SHIFT_ON, psy_CTRL_ON,
		"transposeblockinc12", "Trsp+12");
	set_cmd_all(cmds, CMD_TRANSPOSEBLOCKDEC12,
		psy_ui_KEY_F11, psy_SHIFT_ON, psy_CTRL_ON,
		"transposeblockdec12", "Trsp-12");
	set_cmd_all(cmds, CMD_SELECTALL,
		psy_ui_KEY_A, psy_SHIFT_OFF, psy_CTRL_ON,
		"selectall", "sel all");
	set_cmd_all(cmds, CMD_SELECTCOL,
		psy_ui_KEY_R, psy_SHIFT_OFF, psy_CTRL_ON,
		"selectcol", "sel col");
	set_cmd_all(cmds, CMD_SELECTBAR,
		psy_ui_KEY_K, psy_SHIFT_OFF, psy_CTRL_ON,
		"selectbar", "sel bar");
	set_cmd(cmds, CMD_SELECTMACHINE,
		psy_ui_KEY_RETURN, "selectmachine",
		"Sel Mac/Ins");
	set_cmd_all(cmds, CMD_UNDO,
		psy_ui_KEY_Z, psy_SHIFT_OFF, psy_CTRL_ON,
		"undo", "undo");
	set_cmd_all(cmds, CMD_REDO,
		psy_ui_KEY_Z, psy_SHIFT_ON, psy_CTRL_ON,
		"redo", "redo");	
}

/* implementation */
void set_cmd_all(psy_Property* cmds, uintptr_t cmd, uint32_t keycode,
	bool shift, bool ctrl, const char* key, const char* shorttext)
{
	char text[256];

	assert(cmds);

	psy_snprintf(text, 256, "cmds.%s", key);
	psy_property_set_hint(psy_property_set_text(psy_property_set_short_text(
		psy_property_set_id(psy_property_append_int(cmds, key,
			psy_audio_encodeinput(keycode, shift, ctrl, 0, 0), 0, 0),
			cmd), shorttext), text), PSY_PROPERTY_HINT_SHORTCUT);
}

void set_cmd(psy_Property* cmds, int cmd, uint32_t keycode, const char* key,
	const char* short_text)
{
	assert(cmds);

	set_cmd_all(cmds, cmd, keycode, psy_SHIFT_OFF, psy_CTRL_OFF, key,
		short_text);
}
