/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "translator.h"
/* local */
#include "inireader.h"
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static void psy_translator_on_read(psy_Translator*, psy_IniReader* sender);	
static void psy_translator_on_test(psy_Translator*, psy_IniReader* sender);	

/* implementation */
void psy_translator_init(psy_Translator* self)
{
	assert(self);
	
	psy_dictionary_init(&self->dictionary_);
	psy_signal_init(&self->signal_language_changed);
	self->test_id_ = NULL;
}

void psy_translator_dispose(psy_Translator* self)
{
	assert(self);

	psy_dictionary_dispose(&self->dictionary_);	
	free(self->test_id_);
	self->test_id_ = NULL;
	psy_signal_dispose(&self->signal_language_changed);
}

void psy_translator_set_default(psy_Translator* self, const psy_Dictionary* lang)
{
	assert(self);
	
	psy_dictionary_reset(&self->dictionary_);
	if (lang) {
		psy_TableIterator it;

		for (it = psy_table_begin(&((psy_Dictionary*)lang)->container);
				!psy_tableiterator_equal(&it, psy_table_end());
				psy_tableiterator_inc(&it)) {
			uintptr_t src_key;
			const char* src_value;

			src_key = psy_tableiterator_key(&it);
			src_value = (const char*)psy_tableiterator_value(&it);
			psy_dictionary_set_hash(&self->dictionary_, src_key, src_value);
		}
	}
}

void psy_translator_reset(psy_Translator* self)
{
	assert(self);
	
	psy_dictionary_reset(&self->dictionary_);	
}

bool psy_translator_load(psy_Translator* self, const char* path)
{	
	int success;
	psy_IniReader reader;

	assert(self);
	
	psy_inireader_init(&reader);
	psy_signal_connect(&reader.signal_read, self,
		psy_translator_on_read);
	success = psy_inireader_load(&reader, path);
	psy_inireader_dispose(&reader);
	psy_signal_emit(&self->signal_language_changed, self, 0);
	return success;	
}

bool psy_translator_test(const psy_Translator* self, const char* path, char* id)
{	
	psy_IniReader reader;

	assert(self);
	
	free(((psy_Translator*)self)->test_id_);
	((psy_Translator*)self)->test_id_ = NULL;
	psy_inireader_init(&reader);
	psy_signal_connect(&reader.signal_read, (psy_Translator*)self,
		psy_translator_on_test);
	id[0] = '\0';
	if (psy_inireader_load(&reader, path) == PSY_OK) {
		if (self->test_id_) {
			psy_snprintf(id, 256, "%s", self->test_id_);
		}
	}
	psy_inireader_dispose(&reader);
	return psy_strlen(id) != 0;	
}

const char* psy_translator_translate(psy_Translator* self, const char* key)
{		
	const char* rv;

	assert(self);

	rv = psy_dictionary_at(&self->dictionary_, key);
	if (rv) {
		return rv;
	}
	return key;
}

const char* psy_translator_lang_id(const psy_Translator* self)
{	
	const char* rv;

	assert(self);
	
	rv = psy_dictionary_at(&self->dictionary_, "lang");
	if (rv) {
		return rv;
	}
	return "en";
}

void psy_translator_on_read(psy_Translator* self, psy_IniReader* sender)
{
	const char* key;
	const char* value;		
	char* full_key;	
	uintptr_t len;

	assert(self);
	
	key = psy_inireader_key(sender);
	value = psy_inireader_value(sender);
	len = psy_strlen(sender->section) + psy_strlen(key) + 1;
	full_key = (char*)malloc(len + 1);
	if (sender->section) {
		psy_snprintf(full_key, len + 1, "%s.%s", sender->section, key);		
	} else {
		psy_snprintf(full_key, len + 1, "%s", key);
	}
	if (psy_dictionary_at(&self->dictionary_, full_key)) {		
		psy_dictionary_set(&self->dictionary_, full_key, value);
	}
	free(full_key);
	full_key = NULL;
}

void psy_translator_on_test(psy_Translator* self, psy_IniReader* sender)
{
	const char* key;
	const char* value;

	assert(self);

	key = psy_inireader_key(sender);
	value = psy_inireader_value(sender);
	if (psy_strlen(sender->section) == 0 && strcmp(key, "lang") == 0) {
		psy_strreset(&self->test_id_, value);
	}
}
