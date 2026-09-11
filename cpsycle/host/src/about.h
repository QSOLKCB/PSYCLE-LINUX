/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(ABOUT_H)
#define ABOUT_H

/* host */
#include "workspace.h"
/* ui */
#include <uibutton.h>
#include <uilabel.h>
#include <uinotebook.h>
#include <uiscroller.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Contrib */
typedef struct Contrib {
	/*! @extends  */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Scroller scroller_;
	psy_ui_Component bottom_;
	psy_ui_Label asio_;
	psy_ui_Label sourceforge_;
	psy_ui_Label psycledelics_;
	psy_ui_Label steincopyright_;
	psy_ui_Label headercontrib_;
	psy_ui_Label contrib_;
} Contrib;

void contrib_init(Contrib*, psy_ui_Component* parent);

INLINE psy_ui_Component* contrib_base(Contrib* self)
{
	assert(self);

	return &self->component;
}

/* Version */
typedef struct Version {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Label version_info_;
} Version;

void version_init(Version*, psy_ui_Component* parent);

INLINE psy_ui_Component* version_base(Version* self)
{
	assert(self);

	return &self->component;
}

/* Licence */
typedef struct Licence {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Label licence_info_;
	psy_ui_Scroller scroller_;
} Licence;

void licence_init(Licence*, psy_ui_Component* parent);

INLINE psy_ui_Component* licence_base(Licence* self)
{
	assert(self);

	return &self->component;
}

/*
** @struct About
** @brief Shows information about Psycle.
*/
typedef struct About {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Notebook notebook_;
	psy_ui_Component bottom_;
	psy_ui_Component buttons_;
	Contrib contrib_;
	Version version_;
	Licence licence_;
	psy_ui_Component empty_;
	psy_ui_Button contribbutton_;
	psy_ui_Button versionbutton_;
	psy_ui_Button licencebutton_;
	psy_ui_Button ok_button_;
	psy_ui_Component helpviewbar_;
	psy_ui_Button show_at_start_;
	ViewIndex next_view_;	
	/* references */
	Workspace* workspace_;
} About;

void about_init(About*, psy_ui_Component* parent, Workspace*);

INLINE psy_ui_Component* about_base(About* self)
{
	assert(self);

	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* ABOUT_H */
