/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(psy_CONFIGURATION_H)
#define psy_CONFIGURATION_H

#include "../../detail/psydef.h"


#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
** Interface that implements the Command Pattern used
** for undo/redo operations
*/

typedef struct psy_ConfigurationHints {
	char svg[256];
} psy_ConfigurationHints;

psy_ConfigurationHints psy_configurationhints_make(const char* svg);

struct psy_Configuration;
struct psy_Property;

typedef void (*psy_fp_configuration_connect)(struct psy_Configuration*,
	const char* key, void* context, void* fp);
typedef struct psy_Property* (*psy_fp_configuration_at)(
	struct psy_Configuration*, const char* key);
typedef void (*psy_fp_configuration_preload)(
	struct psy_Configuration*);
typedef bool (*psy_fp_configuration_postload)(
	struct psy_Configuration*, uintptr_t count);
typedef void (*psy_fp_configuration_reset)(struct psy_Configuration*);	
typedef void (*psy_fp_configuration_set_hints)(struct psy_Configuration*,
	psy_ConfigurationHints);
typedef psy_ConfigurationHints (*psy_fp_configuration_hints)(const struct psy_Configuration*);	


typedef struct psy_ConfigurationVtable {	
	psy_fp_configuration_connect connect;
	psy_fp_configuration_at at;
	psy_fp_configuration_preload preload;
	psy_fp_configuration_postload postload;
	psy_fp_configuration_reset reset;
	psy_fp_configuration_set_hints set_hints;
	psy_fp_configuration_hints hints;
} psy_ConfigurationVtable;

typedef struct psy_Configuration {
	psy_ConfigurationVtable* vtable;
} psy_Configuration;

void psy_configuration_init(psy_Configuration* self);


INLINE void psy_configuration_connect(psy_Configuration* self, const char* key,
	void* context, void* fp)
{
	assert(self);

	self->vtable->connect(self, key, context, fp);
}

INLINE struct psy_Property* psy_configuration_at(psy_Configuration* self,
	const char* key)
{
	assert(self);

	return self->vtable->at(self, key);
}

void psy_configuration_configure(psy_Configuration*, const char* key);

bool psy_configuration_value_bool(const psy_Configuration*, const char* key,
	bool default_value);
intptr_t psy_configuration_value_int(const psy_Configuration*, const char* key,
	intptr_t def_value);
double psy_configuration_value_double(const psy_Configuration*,
	const char* key, double default_value);
const char* psy_configuration_value_str(const psy_Configuration*,
	const char* key, const char* default_value);	

INLINE void psy_configuration_preload(psy_Configuration* self)
{
	assert(self);

	self->vtable->preload(self);
}
	
INLINE bool psy_configuration_postload(psy_Configuration* self, uintptr_t count)
{
	assert(self);

	return self->vtable->postload(self, count);
}

INLINE void psy_configuration_reset(psy_Configuration* self)
{
	assert(self);

	self->vtable->reset(self);
}

INLINE void psy_configuration_set_hints(psy_Configuration* self,
	psy_ConfigurationHints hints)
{
	assert(self);

	self->vtable->set_hints(self, hints);
}

INLINE psy_ConfigurationHints psy_configuration_hints(
	const psy_Configuration* self)
{
	assert(self);

	return self->vtable->hints(self);
}

/* CustomConfiguraton */

typedef struct psy_CustomConfiguration {
	/* implements */
	psy_Configuration configuration;
	/*! @internal */
	psy_ConfigurationHints hints_;
	/* references */
	struct psy_Property* root_;	
} psy_CustomConfiguration;

void psy_customconfiguration_init(psy_CustomConfiguration*);
void psy_customconfiguration_dispose(psy_CustomConfiguration*);

void psy_customconfiguration_set_root(psy_CustomConfiguration*,
	struct psy_Property* root);
struct psy_Property* psy_customconfiguration_root(psy_CustomConfiguration*);

INLINE psy_Configuration* psy_customconfiguration_base(
    psy_CustomConfiguration* self)
{
	assert(self);

	return &self->configuration;
}

INLINE const psy_Configuration* psy_customconfiguration_base_const(
    const psy_CustomConfiguration* self)
{
	assert(self);

	return &self->configuration;
}

#ifdef __cplusplus
}
#endif

#endif /* psy_CONFIGURATION_H */
