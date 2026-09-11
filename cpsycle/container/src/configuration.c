/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "configuration.h"
/* local */
#include "properties.h"
/* platform */
#include "../../detail/portable.h"


/* psy_ConfigurationHints */

psy_ConfigurationHints psy_configurationhints_make(const char* svg)
{
	psy_ConfigurationHints rv;

	if (svg) {
		psy_snprintf(rv.svg, 256, svg);
	}
	else {
		rv.svg[0] = '\0';
	}
	return rv;
}

/* psy_Configuration */

/* prototypes */
static void connect(psy_Configuration* self, const char* key, void* context, void* fp) { assert(self); }
static struct psy_Property* at(psy_Configuration* self, const char* key) { assert(self); return NULL; }
static void preload(psy_Configuration* self) { assert(self); }
static bool postload(psy_Configuration* self, uintptr_t count) { assert(self); return FALSE; }
static void reset(psy_Configuration* self) { assert(self); }
static void set_hints(psy_Configuration* self, psy_ConfigurationHints hints) { assert(self); }

static psy_ConfigurationHints hints(const psy_Configuration* self)
{ 
	assert(self);

	return psy_configurationhints_make(NULL);
}


/* vtable */
static psy_ConfigurationVtable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(psy_Configuration* self)
{
	assert(self);

	if (!vtable_initialized) {		
		vtable.connect = connect;
		vtable.at = at;
		vtable.preload = preload;
		vtable.postload = postload;
		vtable.reset = reset;
		vtable.set_hints = set_hints;
		vtable.hints = hints;
		vtable_initialized = TRUE;
	}
	self->vtable = &vtable;
}

/* implementation */
void psy_configuration_init(psy_Configuration* self)
{
	assert(self);
	
	vtable_init(self);
}

intptr_t psy_configuration_value_int(const psy_Configuration* self,
	const char* key, intptr_t def_value)
{
	psy_Property* p;

	assert(self);

	p = self->vtable->at((psy_Configuration*)self, key);
	if (p) {
		return psy_property_item_int(p);
	}
	return def_value;
}

void psy_configuration_configure(psy_Configuration* self, const char* key)
{
	psy_Property* p;
	
	assert(self);
	
	p = psy_configuration_at(self, key);
	if (p) {
		psy_property_notify(p);
	}
}

bool psy_configuration_value_bool(const psy_Configuration* self, const char* key,
	bool default_value)
{
	psy_Property* p;
	
	assert(self);
	
	p = psy_configuration_at((psy_Configuration*)self, key);
	if (p && psy_property_type(p) == PSY_PROPERTY_TYPE_BOOL) {
		return psy_property_item_bool(p);
	}
	return default_value;
}

double psy_configuration_value_double(const psy_Configuration* self, const char* key,
	double default_value)
{
	psy_Property* p;

	assert(self);

	p = psy_configuration_at((psy_Configuration*)self, key);
	if (p && psy_property_type(p) == PSY_PROPERTY_TYPE_DOUBLE) {
		return psy_property_item_double(p);
	}
	return default_value;
}

const char* psy_configuration_value_str(const psy_Configuration* self,
	const char* key, const char* default_value)
{
	psy_Property* p;
	
	assert(self);
	
	p = psy_configuration_at((psy_Configuration*)self, key);
	if (p && (psy_property_type(p) == PSY_PROPERTY_TYPE_STRING ||
			  psy_property_type(p) == PSY_PROPERTY_TYPE_FONT)) {
		return psy_property_item_str(p);
	}
	return default_value;
}


/* psy_CustomConfiguration */

/* prototypes */
static void psy_customconfiguration_connect(psy_CustomConfiguration*, const char* key,
	void* context, void* fp);
static psy_Property* psy_customconfiguration_at(psy_CustomConfiguration*, const char* key);
static void psy_customconfiguration_set_hints(psy_CustomConfiguration*, psy_ConfigurationHints);
static psy_ConfigurationHints psy_customconfiguration_hints(const psy_CustomConfiguration*);

/* vtable */
static psy_ConfigurationVtable psy_customconfiguration_vtable;
static bool psy_customconfiguration_vtable_initialized = FALSE;

static void psy_customconfiguration_vtable_init(psy_CustomConfiguration* self)
{
	if (!psy_customconfiguration_vtable_initialized) {
		psy_customconfiguration_vtable = *self->configuration.vtable;
		psy_customconfiguration_vtable.connect =
			(psy_fp_configuration_connect)
			psy_customconfiguration_connect;
		psy_customconfiguration_vtable.at =
			(psy_fp_configuration_at)
			psy_customconfiguration_at;
		psy_customconfiguration_vtable.set_hints =
			(psy_fp_configuration_set_hints)
			psy_customconfiguration_set_hints;
		psy_customconfiguration_vtable.hints =
			(psy_fp_configuration_hints)
			psy_customconfiguration_hints;
		psy_customconfiguration_vtable_initialized = TRUE;
	}
	self->configuration.vtable = &psy_customconfiguration_vtable;
}

/* implementation */
void psy_customconfiguration_init(psy_CustomConfiguration* self)
{
	assert(self);	

	psy_configuration_init(&self->configuration);
	psy_customconfiguration_vtable_init(self);
	self->root_ = NULL;
	self->hints_ = psy_configurationhints_make(NULL);
}

void psy_customconfiguration_dispose(psy_CustomConfiguration* self)
{
	assert(self);

}

void psy_customconfiguration_set_root(psy_CustomConfiguration* self,
	psy_Property* root)
{
	assert(self);

	self->root_ = root;
}

psy_Property* psy_customconfiguration_root(psy_CustomConfiguration* self)
{
	return self->root_;
}

void psy_customconfiguration_connect(psy_CustomConfiguration* self, const char* key,
	void* context, void* fp)
{
	psy_Property* p;

	assert(self);

	p = psy_customconfiguration_at(self, key);
	if (p) {
		psy_property_connect(p, context, fp);
	}
}

psy_Property* psy_customconfiguration_at(psy_CustomConfiguration* self, const char* key)
{
	assert(self);
	
	if (psy_strlen(key) == 0) {
		return self->root_;
	}
	return psy_property_at(self->root_, key, PSY_PROPERTY_TYPE_NONE);	
}

void psy_customconfiguration_set_hints(psy_CustomConfiguration* self,
	psy_ConfigurationHints hints)
{
	assert(self);

	self->hints_ = hints;
	psy_property_set_svg(self->root_, hints.svg);
}

psy_ConfigurationHints psy_customconfiguration_hints(const psy_CustomConfiguration* self)
{
	assert(self);

	return self->hints_;
}
