/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(LANGUAGECONFIG_H)
#define LANGUAGECONFIG_H

/* container */
#include <configuration.h>

#ifdef __cplusplus
extern "C" {
#endif

struct psy_Translator;

/*!
** @struct LanguageConfig
** @brief Configures and selects the language of a psy_Translator.
*/
typedef struct LanguageConfig {
	/*! @extends */
	psy_CustomConfiguration configuration;	
	/*! @internal */	
	/* references */	
	struct psy_Translator* translator;
} LanguageConfig;

void languageconfig_init(LanguageConfig*, struct psy_Property* parent,
	struct psy_Translator*);
void languageconfig_dispose(LanguageConfig*);
void languageconfig_update_language(LanguageConfig*);

INLINE psy_Configuration* languageconfig_base(LanguageConfig* self)
{
	assert(self);

	return psy_customconfiguration_base(&self->configuration);
}

#ifdef __cplusplus
}
#endif

#endif /* LANGUAGECONFIG_H */
