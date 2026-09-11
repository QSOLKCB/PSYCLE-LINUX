/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(CMDDEF_H)
#define CMDDEF_H

/* container */
#include <properties.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
** Defines a property with shortcut defaults for the keyboard driver
** key		: cmd id used by the trackerview
** text		: "cmds.key" language dictionary key used by the translator
** shorttext: short description for the keyboard help view
** value	: encoded key shortcut (keycode/shift/ctrl)
*/

void set_cmd_all(psy_Property* cmds, uintptr_t cmd, uint32_t keycode,
	bool shift, bool ctrl, const char* key, const char* shorttext);
void set_cmd(psy_Property* cmds, int cmd, uint32_t keycode, const char* key,
	const char* short_text);

#ifdef __cplusplus
}
#endif

#endif /* CMDDEF_H */
