/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(RENDERVIEW_H)
#define RENDERVIEW_H

/* host */
#include "propertiesview.h"
#include "renderprogressview.h"
/* ui */
#include <uilabel.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct RenderView
** @brief View for saving a song as a Windows PCM Wav.
*/
typedef struct RenderView {
	/*! @extends  */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Label viewbar;
	PropertiesView view;	
	RenderProgressView progress_view;
	psy_Property* properties;	
	psy_Property* driver_configure;
	psy_AudioDriver* psy_audio_FileOutDriver;		
	int restore_loop_mode;
	psy_dsp_DitherSettings restore_dither;
	/* references */
	psy_AudioDriver* curr_audio_driver;
	Workspace* workspace;
} RenderView;

void renderview_init(RenderView*, psy_ui_Component* parent,
	psy_ui_Component* tabbarparent, Workspace*);

INLINE psy_ui_Component* renderview_base(RenderView* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* RENDERVIEW_H */
