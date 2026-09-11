/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(PLUGINSCANVIEW_H)
#define PLUGINSCANVIEW_H

/* host */
#include "clockbar.h"
#include "pluginscanthread.h"
#include "pluginsview.h"
/* ui */
#include <uibutton.h>
#include <uilabel.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PluginScanDescView {
	/*! @extends */
	psy_ui_Component component;	
} PluginScanDescView;

void pluginscandescview_init(PluginScanDescView*, psy_ui_Component* parent);

typedef struct PluginScanStatusView {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Label time_desc;
	ClockBar time;
	psy_ui_Label filenum_desc;
	psy_ui_Label filenum;
	psy_ui_Label pluginnum_desc;
	psy_ui_Label pluginnum;	
} PluginScanStatusView;

void pluginscanstatusview_init(PluginScanStatusView*, psy_ui_Component* parent);

void pluginscanstatusview_update(PluginScanStatusView*, const
	psy_audio_PluginCatcher*);
void pluginscanstatusview_reset(PluginScanStatusView*);

typedef struct PluginScanTaskView {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Component tasks;
	psy_ui_Component buttons;
	psy_ui_Button pause;
	psy_ui_Button abort;
	/* references */
	psy_audio_PluginCatcher* plugincatcher;
} PluginScanTaskView;

void pluginscantaskview_init(PluginScanTaskView*, psy_ui_Component* parent,
	psy_audio_PluginCatcher*);

void pluginscantaskview_select(PluginScanTaskView*, psy_audio_PluginScanTask*);

typedef struct PluginScanProcessView {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Component client;
	PluginScanDescView descview;
	PluginScanTaskView taskview;
	PluginScanStatusView statusview;	
	psy_ui_Label scan;
} PluginScanProcessView;

void pluginscanprocessview_init(PluginScanProcessView*, psy_ui_Component* parent,
	psy_audio_PluginCatcher*);
	
void pluginscanprocessview_stop(PluginScanProcessView*);
void pluginscanprocessview_reset(PluginScanProcessView*);

INLINE void pluginscanprocessview_update(PluginScanProcessView* self, const
	psy_audio_PluginCatcher* catcher)
{
	assert(self);
	
	pluginscanstatusview_update(&self->statusview, catcher);
}

typedef struct PluginScanView {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */			
	PluginScanProcessView processview;	
	/* references */
	PluginScanThread* plugin_scan_thread;
	psy_audio_PluginCatcher* plugin_catcher;	
} PluginScanView;

void pluginscanview_init(PluginScanView*, psy_ui_Component* parent,
	PluginScanThread*);

void pluginscanview_reset(PluginScanView*);
void pluginscanview_stop(PluginScanView*);

#ifdef __cplusplus
}
#endif

#endif /* PLUGINSCANVIEW_H */
