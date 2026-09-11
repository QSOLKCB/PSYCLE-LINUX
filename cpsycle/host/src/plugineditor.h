/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(PLUGINEDITOR_H)
#define PLUGINEDITOR_H

/* host */
#include "plugineditorbar.h"
#include "titlebar.h"
#include "workspace.h"
/* ui */
#include <uibutton.h>
#include <uicombobox.h>
#include <uitext.h>
#include <uieditor.h>
#include <uilabel.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
** PluginEditor
**
** Editor for lua plugins
*/

typedef struct PluginEditorCreateBar {
	/*! @extends  */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Label desc;
	psy_ui_Edit name;
	psy_ui_Button create;
	/* references */
	Workspace* workspace;
} PluginEditorCreateBar;

void plugineditorcreatebar_init(PluginEditorCreateBar*, psy_ui_Component*
	parent, Workspace*);

typedef struct PluginEditor {
	/*! @extends  */
	psy_ui_Component component;
	/*! @internal */
	TitleBar title_bar_;
	psy_ui_Component view_buttons;
	psy_ui_Component view_buttons_client;
	psy_ui_Button view_float;
	psy_ui_Editor editor;
	psy_ui_Component bar;
	psy_ui_Button newplugin;
	psy_ui_Button reload;
	psy_ui_Button save;
	psy_ui_Component row0;
	psy_ui_Label plugindesc;
	psy_ui_ComboBox pluginselector;
	psy_ui_Component row1;
	psy_ui_Label filedesc;
	psy_ui_ComboBox fileselector;
	PluginEditorCreateBar createbar;	
	char* basepath;	
	uintptr_t instance;	
	double zoom;
	PluginEditorBar status;
	/* references */
	psy_ui_Component* splitter_;
	Workspace* workspace;
} PluginEditor;

void plugineditor_init(PluginEditor*, psy_ui_Component* parent, Workspace*);

void plugineditor_update_font(PluginEditor*);

INLINE void plugineditor_set_splitter(PluginEditor* self, psy_ui_Component* splitter)
{
	assert(self);

	self->splitter_ = splitter;
}

INLINE psy_ui_Component* plugineditor_base(PluginEditor* self)
{
	assert(self);

	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* PLUGINEDITOR_H */
