/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(GLOBALCONFIG_H)
#define GLOBALCONFIG_H

/* host */
#include "compatconfig.h"
#include "generalconfig.h"
#include "languageconfig.h"
/* container */
#include <configuration.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct GlobalConfig
*/
typedef struct GlobalConfig {
	/*! @extends */
	psy_CustomConfiguration configuration;		
	/*! @internal */
	LanguageConfig lang_;
	GeneralConfig general_;
	CompatConfig compat_;
} GlobalConfig;

void globalconfig_init(GlobalConfig*, struct psy_Property* parent);
void globalconfig_dispose(GlobalConfig*);

INLINE psy_Configuration* globalconfig_lang(GlobalConfig* self)
{
	assert(self);

	return languageconfig_base(&self->lang_);
}

INLINE psy_Configuration* globalconfig_general(GlobalConfig* self)
{
	assert(self);

	return generalconfig_base(&self->general_);
}

INLINE const psy_Configuration* globalconfig_general_const(
    const GlobalConfig* self)
{
	assert(self);

	return generalconfig_base_const(&self->general_);
}

INLINE psy_Configuration* globalconfig_compat(GlobalConfig* self)
{
	assert(self);

	return compatconfig_base(&self->compat_);
}

INLINE psy_Configuration* globalconfig_base(GlobalConfig* self)
{
	assert(self);

	return psy_customconfiguration_base(&self->configuration);
}

#ifdef __cplusplus
}
#endif

#endif /* GLOBALCONFIG_H */
