/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(VISUALCONFIG_H)
#define VISUALCONFIG_H

/* host*/
#include "languageconfig.h"
#include "machineviewconfig.h"
#include "machineparamconfig.h"
#include "patternviewconfig.h"
#include "viewindex.h"
/* container */
#include <configuration.h>
#include <properties.h>

#ifdef __cplusplus
extern "C" {
#endif

struct psy_DiskOp;

/*!
** @struct VisualConfig
** @brief Configures the appearence of the host
*/
typedef struct VisualConfig {
	/*! @extends */
	psy_CustomConfiguration configuration;
	/* signals */
	psy_Property* visual;
	psy_Property* defaultfont;
	psy_Property* apptheme;
	psy_Property* vu;
	/*! @internal */
	PatternViewConfig patview;
	MachineViewConfig macview;
	MachineParamConfig macparam;
	/* references */
	psy_Property* parent;
	psy_Configuration* dirconfig;
	struct psy_DiskOp* disk_op;
} VisualConfig;

void visualconfig_init(VisualConfig*, psy_Property* parent, struct psy_DiskOp*);
void visualconfig_dispose(VisualConfig*);

void visualconfig_set_directories(VisualConfig* self, psy_Configuration*);

INLINE psy_Configuration* visualconfig_base(VisualConfig* self)
{
	assert(self);

	return psy_customconfiguration_base(&self->configuration);
}

#ifdef __cplusplus
}
#endif

#endif /* VISUALCONFIG_H */
