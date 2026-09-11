/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "luaui.h"
/* local */
#include "luadimension.h"
#include "luacomponent.h"
#include "luabutton.h"
#include "luacombobox.h"
#include "luaedit.h"
#include "lualabel.h"
#include "lualistbox.h"
#include "luafont.h"
#include "luafontinfo.h"
#include "luagraphics.h"
#include "luaopendialog.h"
#include "luamargin.h"
#include "luapoint.h"
#include "luarectangle.h"
#include "luasavedialog.h"
/* script */
#include <psyclescript.h>

/* platform */
#include "../../detail/portable.h"

static int psy_luaui_exportcmodules(psy_LuaUi*);

void psy_luaui_init(psy_LuaUi* self, psy_PsycleScript* script)
{
	assert(self);	
	assert(script);

	self->script = script;
	psy_luaui_exportcmodules(self);
}

void psy_luaui_dispose(psy_LuaUi* self)
{
	assert(self);	
}

int psy_luaui_exportcmodules(psy_LuaUi* self)
{	
	psyclescript_require(self->script, "psycle.ui.dimension",
		psy_luaui_dimension_open);
	psyclescript_require(self->script, "psycle.ui.boxspace",
		psy_luaui_margin_open);
	psyclescript_require(self->script, "psycle.ui.point",
		psy_luaui_point_open);
	psyclescript_require(self->script, "psycle.ui.rect",
		psy_luaui_rectangle_open);
	psyclescript_require(self->script, "psycle.ui.graphics",
		psy_luaui_graphics_open);
	psyclescript_require(self->script, "psycle.ui.fileopen",
		psy_luaui_opendialog_open);
	psyclescript_require(self->script, "psycle.ui.filesave",
		psy_luaui_savedialog_open);
	psyclescript_require(self->script, "psycle.ui.window",
		psy_luaui_component_open);	
	psyclescript_require(self->script, "psycle.ui.text",
		psy_luaui_label_open);
	psyclescript_require(self->script, "psycle.ui.edit",
		psy_luaui_edit_open);
	psyclescript_require(self->script, "psycle.ui.listview",
		psy_luaui_listbox_open);
	psyclescript_require(self->script, "psycle.ui.combobox",
		psy_luaui_combobox_open);
	psyclescript_require(self->script, "psycle.ui.button",
		psy_luaui_button_open);
	psyclescript_require(self->script, "psycle.ui.fontinfo",
		psy_luaui_fontinfo_open);
	psyclescript_require(self->script, "psycle.ui.font",
		psy_luaui_font_open);
	return 1;
}
