/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_ui_RESOURCES_H
#define psy_ui_RESOURCES_H

/* container */
#include <hashtbl.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef enum psy_ui_ResourceType {
	psy_ui_RESOURCE_TYPE_RAW,
	psy_ui_RESOURCE_TYPE_SVG	
} psy_ui_ResourceType;

/*! @struct psy_ui_Resource */

typedef struct psy_ui_Resource {
	/*! @internal */
	psy_ui_ResourceType type;
	psy_fp_disposefunc dispose;
	void* data;
} psy_ui_Resource;

void psy_ui_resource_init(psy_ui_Resource*);
void psy_ui_resource_init_all(psy_ui_Resource*, char* data,
	psy_ui_ResourceType, psy_fp_disposefunc);
void psy_ui_resource_dispose(psy_ui_Resource*);
psy_ui_Resource* psy_ui_resource_alloc(void);
psy_ui_Resource* psy_ui_resource_alloc_init(void);
psy_ui_Resource* psy_ui_resource_alloc_init_all(void* data,
	psy_ui_ResourceType, psy_fp_disposefunc);


/*! @struct psy_ui_Resources */

typedef struct psy_ui_Resources {
	/*! @internal */	
	psy_Table resources_;
} psy_ui_Resources;

void psy_ui_resources_init(psy_ui_Resources*);
void psy_ui_resources_dispose(psy_ui_Resources*);

void psy_ui_resources_add(psy_ui_Resources*, const char* key,
	psy_ui_Resource*);
const psy_ui_Resource* psy_ui_resources_at(const psy_ui_Resources*,
	const char* key);


#ifdef __cplusplus
}
#endif

#endif /* psy_ui_RESOURCES_H */
