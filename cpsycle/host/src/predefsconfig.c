/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "predefsconfig.h"
/* container */
#include <properties.h>
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static psy_Property* predefsconfig_make(PredefsConfig*, psy_Property* parent);
	
/* implementation */
void predefsconfig_init(PredefsConfig* self, psy_Property* parent)
{
	assert(self);
	assert(parent);

	psy_customconfiguration_init(&self->configuration);	
	psy_customconfiguration_set_root(&self->configuration,
		predefsconfig_make(self, parent));
}

void predefsconfig_dispose(PredefsConfig* self)
{
	assert(self);
	
	psy_customconfiguration_dispose(&self->configuration);
}

psy_Property* predefsconfig_make(PredefsConfig* self, psy_Property* parent)
{
	psy_Property* rv;
	
	assert(self);

	rv = psy_property_append_section(parent, "Predefs");	
	psy_property_append_str(rv,
		"adsr", "2 0.0 0.0 1.0 1.0 2.0 0.5 3.0 0.0");		
	psy_property_append_str(rv, "def1", "");
	psy_property_append_str(rv, "def2", "");
	psy_property_append_str(rv, "def3", "");
	psy_property_append_str(rv, "def4", "");
	psy_property_append_str(rv, "def5", "");
	psy_property_append_str(rv, "def6", "");
	return rv;
}

void predefsconfig_predef(PredefsConfig* self, intptr_t index, psy_dsp_Envelope* rv)
{
	char key[10];
	const char* values;	

	assert(self && rv);

	if (index > 0) {
		psy_snprintf(key, 10, "def%i", index);
	} else {
		psy_snprintf(key, 10, "adsr");		
	}

	values = psy_property_at_str(psy_customconfiguration_root(
		&self->configuration), key, NULL);
	if (!values || values[0] == '\0') {
		return;
	}
	psy_dsp_envelope_init_dispose(rv);
	psy_dsp_envelope_init(rv);
	if (values) {
		char* text;
		char* token;
		char seps[] = " ";
		int c;
		psy_dsp_seconds_t time;
		double value;

		text = psy_strdup(values);
		token = strtok(text, seps);
		time = 0.f;
		value = 0.f;
		c = 0;
		while (token != 0) {
			if (c == 0) {
				rv->sustainbegin = atoi(token);
			} else {
				if ((c % 2) != 0) {
					time = (psy_dsp_seconds_t)atof(token);
				} else {
					value = atof(token);
					if (c == 2) {
						/* first point */
						psy_dsp_envelope_append(rv,
							psy_dsp_envelopepoint_make_all(
								time, value, 0.f, 0.f, 0.f, 0.f));
					} else {
						psy_dsp_envelope_append(rv,
							psy_dsp_envelopepoint_make_all(
								time, value, 0.f, 5.f, 0.f, 1.f));
					}
				}
			}			
			token = strtok(0, seps);
			++c;
		}
		free(text);
	}
}

void predefsconfig_store_predef(PredefsConfig* self, intptr_t index,
	psy_dsp_Envelope* env)
{
	char key[10];	

	assert(self && env);

	if (index > 0) {
		psy_snprintf(key, 10, "def%i", (int)index);
	} else {
		psy_snprintf(key, 10, "adsr");
	}
	if (psy_dsp_envelope_empty(env)) {
		psy_property_set_str(psy_customconfiguration_root(
			&self->configuration), key, "");
		return;
	}	
	psy_property_set_str(psy_customconfiguration_root(
		&self->configuration), key, psy_dsp_envelope_to_string(env));
}
