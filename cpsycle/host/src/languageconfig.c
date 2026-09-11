/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "languageconfig.h"
/* file */
#include <dir.h>
#include <translator.h>
/* container */
#include <properties.h>
/* platform */
#include "../../detail/portable.h"
#include "../../detail/os.h"

#ifdef PSYCLE_DEFAULT_LANG_USER
#ifdef DIVERSALIS__OS__MICROSOFT 
#include <Windows.h>
#endif
#endif

static psy_Property* languageconfig_make_language_choice(LanguageConfig*,
	psy_Property* parent);
static void languageconfig_make_language_list(LanguageConfig*);
static int languageconfig_enum_language_dir(LanguageConfig*, const char* path,
	int flag);
static void languageconfig_set_default_language(LanguageConfig*);
static const char* languageconfig_default_language_key(LanguageConfig*);
static const char* languageconfig_choose_lang(LanguageConfig* self, int id);
static void languageconfig_connect_choice(LanguageConfig*);
static void languageconfig_on_choice(LanguageConfig*, psy_Property* sender);

/* implementation */
void languageconfig_init(LanguageConfig* self, psy_Property* parent,
	psy_Translator* translator)
{
	assert(self);
	assert(parent);
	assert(translator);

	psy_customconfiguration_init(&self->configuration);
	self->translator = translator;	
	psy_customconfiguration_set_root(&self->configuration,
		languageconfig_make_language_choice(self, parent));	
	languageconfig_make_language_list(self);
	languageconfig_set_default_language(self);
	languageconfig_update_language(self);
	languageconfig_connect_choice(self);
}

void languageconfig_dispose(LanguageConfig* self)
{
	assert(self);

	psy_customconfiguration_dispose(&self->configuration);
}

psy_Property* languageconfig_make_language_choice(LanguageConfig* self,
	psy_Property* parent)
{
	assert(self);

	return psy_property_set_text(psy_property_set_hint(
		psy_property_append_choice(parent, "lang", 0),
		PSY_PROPERTY_HINT_LIST), "settings.global.language");
}

void languageconfig_make_language_list(LanguageConfig* self)
{
	char currdir[4096];
	
	assert(self);
	
#if defined DIVERSALIS__OS__POSIX
	psy_snprintf(currdir, 4096, "%s", "./host");
#else
	psy_workdir(currdir);
#endif
	if (psy_strlen(currdir) > 0) {
		psy_dir_enumerate(self, currdir, "*.ini", 0, (psy_fp_findfile)
			languageconfig_enum_language_dir);
	}
}

int languageconfig_enum_language_dir(LanguageConfig* self, const char* path,
	int flag)
{
	char lang[256];

	assert(self);

	if (psy_translator_test(self->translator, path, lang)) {
		psy_property_set_text(psy_property_append_str(
			psy_customconfiguration_root(&self->configuration), lang,
			path), lang);
	}
	return TRUE;
}

void languageconfig_set_default_language(LanguageConfig* self)
{
	psy_Property* defaultlang;

	assert(self);

	if ((defaultlang = psy_property_find(
			psy_customconfiguration_root(&self->configuration),
			languageconfig_default_language_key(self),
			PSY_PROPERTY_TYPE_NONE))) {	
		uintptr_t index;

		index = psy_property_index(defaultlang);
		if (index != psy_INDEX_INVALID) {
			psy_property_set_item_int(
				psy_customconfiguration_root(&self->configuration),
				index);
		}
	}
}

void languageconfig_connect_choice(LanguageConfig* self)
{
	assert(self);

	psy_configuration_connect(languageconfig_base(self), "",
		self, languageconfig_on_choice);
}

const char* languageconfig_default_language_key(LanguageConfig* self)
{
#ifdef DIVERSALIS__OS__MICROSOFT 
	#if defined PSYCLE_DEFAULT_LANG_USER
	LANGID langid;
	
	langid = GetUserDefaultUILanguage() & 0xFFFF;
	return languageconfig_choose_lang(self, langid);
	#endif
#endif	
#if defined PSYCLE_DEFAULT_LANG_ES
		return "es";
#elif defined PSYCLE_DEFAULT_LANG_EN
		return "en";
#else
		return "en";
#endif	
}

const char* languageconfig_choose_lang(LanguageConfig* self, int id)
{
	assert(self);

	/*
	** map spanish countries to spanish and the rest to english
	** lang ids from https://www.codeproject.com/Articles/11040/Multiple-language-support-for-MFC-applications-wit
	** (Herbert Yu)
	*/
	switch (id) {
	case 0x040a:	/* ESP	ESP	Spanish(Spain, Traditional Sort) */
	case 0x080a:	/* ESM	ESP	Spanish(Mexican) */
	case 0x0c0a:	/* ESN	ESP	Spanish(Spain, Modern Sort) */
	case 0x100a:	/* ESG	ESP	Spanish(Guatemala) */
	case 0x140a:	/* ESC	ESP	Spanish(Costa Rica) */
	case 0x180a:	/* ESA	ESP	Spanish(Panama) */
	case 0x1c0a:	/* ESD	ESP	Spanish(Dominican Republic) */
	case 0x200a:	/* ESV	ESP	Spanish(Venezuela) */
	case 0x240a:	/* ESO	ESP	Spanish(Colombia) */
	case 0x280a:	/* ESR	ESP	Spanish(Peru) */
	case 0x2c0a:	/* ESS	ESP	Spanish(Argentina) */
	case 0x300a:	/* ESF	ESP	Spanish(Ecuador) */
	case 0x340a:	/* ESL	ESP	Spanish(Chile) */
	case 0x380a:	/* ESY	ESP	Spanish(Uruguay) */
	case 0x3c0a:	/* ESZ	ESP	Spanish(Paraguay) */
	case 0x400a:	/* ESB	ESP	Spanish(Bolivia) */
	case 0x440a:	/* ESE	ESP	Spanish(El Salvador) */
	case 0x480a:	/* ESH	ESP	Spanish(Honduras) */
	case 0x4c0a:	/* ESI	ESP	Spanish(Nicaragua) */
	case 0x500a:	/* ESU	ESP	Spanish(Puerto Rico) */
		return "es";
	default:
		return "en";
	}	
}

void languageconfig_update_language(LanguageConfig* self)
{
	psy_Property* lang;

	assert(self);
	
	if ((lang = psy_property_at_choice(psy_customconfiguration_root(
			&self->configuration)))) {
		/*
		** This updates also the ui components over the translator language
		** changed signal the ui is connected to.
		*/
		psy_translator_load(self->translator, psy_property_item_str(lang));
	}
}

void languageconfig_on_choice(LanguageConfig* self, psy_Property* sender)
{
	languageconfig_update_language(self);
}
