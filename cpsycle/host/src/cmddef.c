/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "cmddef.h"

/* driver */
#include "../../driver/eventdriver.h"
/* platform */
#include "../../detail/portable.h"	


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
