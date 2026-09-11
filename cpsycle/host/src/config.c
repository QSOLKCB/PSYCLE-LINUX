/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "config.h"
/* host */
#include "configimport.h"
#include "cmdproperties.h"
#include "defaultlang.h"
#include "resources/resource.h"
#include "styles.h"
#include "workspace.h"
/* ui */
#include <uicomponent.h> /* Translator */
/* file */
#include <dir.h>
#include <fileselect.h>


/* prototypes */
static void psycleconfig_init_ini_path(PsycleConfig*);
static void psycleconfig_define_language(PsycleConfig*);
static void psycleconfig_init_sections(PsycleConfig*);
static void psycleconfig_connect(PsycleConfig*, const char* key,
	void* context, void* fp);
static psy_Property* psycleconfig_at(PsycleConfig*, const char* key);

/* vtable */
static psy_ConfigurationVtable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(PsycleConfig* self)
{
	if (!vtable_initialized) {
		vtable = *self->configuration.vtable;
		vtable.connect =
			(psy_fp_configuration_connect)
			psycleconfig_connect;
		vtable.at =
			(psy_fp_configuration_at)
			psycleconfig_at;
		vtable_initialized = TRUE;
	}
	self->configuration.vtable = &vtable;
}

/* implementation */
void psycleconfig_init(PsycleConfig* self, psy_DiskOp* disk_op)
{
	assert(self);	
	
	psy_configuration_init(&self->configuration);
	vtable_init(self);
	self->disk_op = disk_op;
	psy_property_init(&self->config);
	psycleconfig_init_ini_path(self);	
	psy_property_set_comment(&self->config,
		"Psycle Configuration File created by\r\n; " PSYCLE__BUILD__IDENTIFIER("\r\n; "));	
	psycleconfig_define_language(self);
	psycleconfig_init_sections(self);	
}

void psycleconfig_dispose(PsycleConfig* self)
{	
	assert(self);
	
	psy_audio_audioconfig_dispose(&self->audio);
	eventdriverconfig_dispose(&self->input);
	globalconfig_dispose(&self->global);
	psy_dirconfig_dispose(&self->directories);
	visualconfig_dispose(&self->visual);		
	keyboardmiscconfig_dispose(&self->misc);
	psy_audio_midiviewconfig_dispose(&self->midi);
	psy_property_dispose(&self->config);
	free(self->ini_path);
	self->ini_path = NULL;
}

void psycleconfig_init_ini_path(PsycleConfig* self)
{
	psy_Path path;	

	assert(self);	
	
	psy_path_init(&path, NULL);
	psy_path_set_prefix(&path, psy_dir_config());
	psy_path_set_name(&path, PSYCLE_INI);
	self->ini_path = strdup(psy_path_full(&path));	
	psy_path_dispose(&path);
}

void psycleconfig_define_language(PsycleConfig* self)
{
	psy_Dictionary lang;

	assert(self);

	psy_dictionary_init(&lang);
	make_translator_default(&lang);
	psy_translator_set_default(psy_ui_translator(), &lang);
	psy_dictionary_dispose(&lang);
}

void psycleconfig_init_sections(PsycleConfig* self)
{	
	psy_Property* cmds;
	
	assert(self);
	
	globalconfig_init(&self->global, &self->config);	
	visualconfig_init(&self->visual, &self->config, self->disk_op);	
	keyboardmiscconfig_init(&self->misc, &self->config);	
	psy_dirconfig_init(&self->directories, &self->config);	
	psy_audio_audioconfig_init(&self->audio, &self->config);	
	cmds = cmdproperties_create();
	eventdriverconfig_init(&self->input, &self->config, cmds);		
	psy_audio_midiviewconfig_init(&self->midi, &self->config);	
	visualconfig_set_directories(&self->visual, psycleconfig_directories(self));
}

void psycleconfig_save(PsycleConfig* self)
{	
	psy_PropertyWriter writer;

	assert(self);	
		
	psy_propertywriter_init(&writer, &self->config, self->ini_path);
	psy_propertywriter_save(&writer);
	psy_propertywriter_dispose(&writer);	
}

void psycleconfig_load(PsycleConfig* self)
{	
	psy_PropertyReader reader;
	uintptr_t count;

	assert(self);	
					
	psy_propertyreader_init(&reader, &self->config, self->ini_path);
	psy_configuration_preload(psycleconfig_visual(self));
	psy_propertyreader_load(&reader);	
	count = 0;
	while (psy_configuration_postload(psycleconfig_input(self), count++)) {
		psy_propertyreader_load(&reader);		
	}
	count = 0;
	while (psy_configuration_postload(psycleconfig_visual(self), count++)) {
		psy_propertyreader_load(&reader);
	}
	psy_propertyreader_dispose(&reader);	
}

void psycleconfig_connect(PsycleConfig* self, const char* key, void* context,
	void* fp)
{
	psy_Property* p;

	assert(self);

	p = psycleconfig_at(self, key);
	if (p) {
		psy_property_connect(p, context, fp);		
	}	
}

psy_Property* psycleconfig_at(PsycleConfig* self, const char* key)
{
	assert(self);

	return psy_property_at(&self->config, key, PSY_PROPERTY_TYPE_NONE);
}
