/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
**  copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "plugins.h"
/* local */
#include "dummy.h"
#include "duplicator.h"
#include "duplicator2.h"
#include "master.h"
#include "mixer.h"
#include "audiorecorder.h"
#include "plugin.h"
#include "luaplugin.h"
#include "sampler.h"
#include "xmsampler.h"
#ifdef PSYCLE_USE_VST2
#include "vstplugin.h"
#endif
#ifdef PSYCLE_USE_LADSPA
#include "ladspaplugin.h"
#endif
#ifdef PSYCLE_USE_LV2
#include "lv2plugin.h"
#endif
/* container */
#include <qsort.h>
/* file */
#include <dir.h>
/* platform*/
#include "../../detail/portable.h"
#include "../../detail/strcasestr.h"
#include "../../detail/cpu.h"
#include "../../detail/os.h"


#if defined DIVERSALIS__OS__MICROSOFT
#define SLASH "\\"
#define MODULEEXT ".dll"
#elif defined DIVERSALIS__OS__APPLE
#define SLASH "/"
#define	MODULEEXT ".dylib"
#define _MAX_PATH 4096
#else
#define SLASH "/"
#define MODULEEXT ".so"
#define _MAX_PATH 4096
#endif


static void psy_audio_machineinfo_make(psy_Property* parent,
	const psy_audio_MachineInfo* info)
{
	char key[512];		
	psy_Property* p;
	
	assert(parent);
	
	if (!info) {
		return;
	}
	machineinfo_catchername(info, key);	
	p = psy_property_append_section(parent, key);
	psy_property_append_int(p, "type", info->type, 0, 0);
	psy_property_append_int(p, "flags", info->flags, 0, 0);
	psy_property_append_int(p, "mode", info->mode, 0, 0);
	psy_property_append_str(p, "name", info->name);
	psy_property_append_str(p, "shortname", info->shortname);
	psy_property_append_str(p, "author", info->author);
	psy_property_append_str(p, "command", info->command);
	psy_property_append_str(p, "path", info->modulepath);
	psy_property_append_str(p, "desc", info->desc);		
	psy_property_append_int(p, "shellidx", info->shellidx, 0, 0);
	psy_property_append_int(p, "apiversion", info->apiversion, 0, 0);
	psy_property_append_int(p, "plugversion", info->plugversion, 0, 0);
	psy_property_append_str(p, "category", info->category);
	psy_property_append_int(p, "favorite", 0, 0, 0);	
}

static intptr_t plugins_comp_favorite(psy_audio_MachineInfo* p, psy_audio_MachineInfo* q);
static intptr_t plugins_comp_name(psy_audio_MachineInfo* p, psy_audio_MachineInfo* q);
static intptr_t plugins_comp_type(psy_audio_MachineInfo* p, psy_audio_MachineInfo* q);
static intptr_t plugins_comp_mode(psy_audio_MachineInfo* p, psy_audio_MachineInfo* q);

typedef enum PluginsSortMode {
	PLUGINSSORTMODE_NONE,
	PLUGINSSORTMODE_FAVORITE,
	PLUGINSSORTMODE_NAME,
	PLUGINSSORTMODE_TYPE,
	PLUGINSSORTMODE_MODE,
} PluginsSortMode;

static void plugins_sort(psy_audio_Plugins* source, psy_fp_comp,
	psy_audio_Plugins* rv);
static int isplugin(intptr_t type);

void plugins_sort(psy_audio_Plugins* source, psy_fp_comp comp,
	psy_audio_Plugins* rv)
{	
	psy_audio_plugins_clear(rv);
	if (source) {
		uintptr_t i;
		uintptr_t num;
		psy_List* p;
		psy_Table ptrs;

		num = psy_audio_plugins_size(source);
		psy_table_init(&ptrs);
		p = psy_audio_plugins_begin(source);
		for (i = 0; p != NULL && i < num; psy_list_next(&p), ++i) {
			psy_table_insert(&ptrs, i, (psy_audio_MachineInfo*)
				psy_list_entry(p));
		}
		psy_qsort(&ptrs,
			(psy_fp_set_index_double)psy_table_insert,
			(psy_fp_index_double)psy_table_at,
			0, (int)(num - 1), comp);		
		for (i = 0; i < num; ++i) {
			psy_audio_plugins_add(rv, (const psy_audio_MachineInfo*)
				psy_table_at(&ptrs, i));
		}		
		psy_table_dispose(&ptrs);
	}	
}

intptr_t plugins_comp_favorite(psy_audio_MachineInfo* p, psy_audio_MachineInfo* q)
{
	int left;
	int right;

	left = (int)0; // psy_property_at_int(p, "favorite", 0);
	right = (int)0; // psy_property_at_int(q, "favorite", 0);
	return right - left;
}

intptr_t plugins_comp_name(psy_audio_MachineInfo* p, psy_audio_MachineInfo* q)
{
	const char* left;
	const char* right;

	left = p->name;
	if (psy_strlen(left) == 0) {		
		left = 0; //psy_property_key(p);
	}
	right = q->name;
	if (psy_strlen(right) == 0) {		
		right = 0; //psy_property_key(q);
	}
	return strcmp(left, right);
}

intptr_t plugins_comp_type(psy_audio_MachineInfo* p, psy_audio_MachineInfo* q)
{
	intptr_t left;
	intptr_t right;

	left = p->type;
	left = isplugin(left) ? left : 0;
	right = q->type;
	right = isplugin(right) ? right : 0;
	return left - right;
}

intptr_t plugins_comp_mode(psy_audio_MachineInfo* p, psy_audio_MachineInfo* q)
{
	return ((int)p->mode - (int)q->mode);
}

int isplugin(intptr_t type)
{
	return (type == psy_audio_PLUGIN) ||
		(type == psy_audio_VST) ||
		(type == psy_audio_VSTFX) ||
		(type == psy_audio_LUA) ||
		(type == psy_audio_LADSPA) || 
		(type == psy_audio_LV2);
}



/* PluginFilterItem */

void pluginfilteritem_init(PluginFilterItem* self, uintptr_t key,
	const char* name, bool active)
{
	self->key = key;
	self->name = psy_strdup(name);
	self->active = active;
}

void pluginfilteritem_dispose(PluginFilterItem* self)
{	
	free(self->name);
	self->name = NULL;	
}

PluginFilterItem* pluginfilteritem_allocinit(uintptr_t key, const char* name,
	bool active)
{
	PluginFilterItem* rv;
	
	rv = (PluginFilterItem*)malloc(sizeof(PluginFilterItem));
	if (rv) {
		pluginfilteritem_init(rv, key, name, active);
	}
	return rv;
}

/* PluginFilterGroup */

static PluginFilterItem* pluginfiltergroup_item(PluginFilterGroup*,
	uintptr_t key);

void pluginfiltergroup_init(PluginFilterGroup* self, uintptr_t id,
	const char* label)
{
	self->items = NULL;	
	self->id = id;
	self->label = psy_strdup(label);
}

void pluginfiltergroup_dispose(PluginFilterGroup* self)
{
	psy_list_deallocate(&self->items, (psy_fp_disposefunc)
		pluginfilteritem_dispose);
	free(self->label);
	self->label = NULL;
}

PluginFilterGroup* pluginfiltergroup_allocinit(uintptr_t id,
	const char* label)
{
	PluginFilterGroup* rv;
	
	rv = (PluginFilterGroup*)malloc(sizeof(PluginFilterGroup));
	if (rv) {
		pluginfiltergroup_init(rv, id, label);
	}
	return rv;
}


bool pluginfiltergroup_add(PluginFilterGroup* self,
	uintptr_t key, const char* name, bool active)
{
	if (key == psy_INDEX_INVALID) {
		return FALSE;
	}
	if (pluginfiltergroup_item(self, key)) {
		return FALSE;
	}
	psy_list_append(&self->items, pluginfilteritem_allocinit(key, name,
		active));
	return TRUE;
}

void pluginfiltergroup_clear(PluginFilterGroup* self)
{
	psy_list_deallocate(&self->items, (psy_fp_disposefunc)
		pluginfilteritem_dispose);
}

bool pluginfiltergroup_remove(PluginFilterGroup* self, uintptr_t key)
{
	psy_List* p;
	
	for (p = self->items; p != NULL; p = p->next) {
		PluginFilterItem* curr;
		
		curr = (PluginFilterItem*)p->entry;
		if (curr->key && key == curr->key) {
			pluginfilteritem_dispose(curr);
			psy_list_remove(&self->items, p);
			break;
		}
	}	
	return (p != NULL);
}

void pluginfiltergroup_select(PluginFilterGroup* self, uintptr_t key)
{
	PluginFilterItem* item;
	
	item = pluginfiltergroup_item(self, key);
	if (item) {
		item->active = TRUE;
	}	
}

void pluginfiltergroup_select_all(PluginFilterGroup* self)
{
	psy_List* p;

	for (p = self->items; p != NULL; p = p->next) {
		PluginFilterItem* curr;
		
		curr = (PluginFilterItem*)p->entry;
		curr->active = TRUE;
	}	
}

void pluginfiltergroup_deselect(PluginFilterGroup* self, uintptr_t key)
{
	PluginFilterItem* item;
	
	item = pluginfiltergroup_item(self, key);
	if (item) {
		item->active = FALSE;
	}
}

void pluginfiltergroup_deselect_all(PluginFilterGroup* self)
{
	psy_List* p;

	for (p = self->items; p != NULL; p = p->next) {
		PluginFilterItem* curr;
		
		curr = (PluginFilterItem*)p->entry;
		curr->active = FALSE;
	}	
}

void pluginfiltergroup_toggle(PluginFilterGroup* self, uintptr_t key)
{
	if (pluginfiltergroup_selected(self, key)) {
		pluginfiltergroup_deselect(self, key);
	} else {
		pluginfiltergroup_select(self, key);
	}	
}

bool pluginfiltergroup_selected(const PluginFilterGroup* self, uintptr_t key)
{
	PluginFilterItem* item;
	
	item = pluginfiltergroup_item((PluginFilterGroup*)self, key);
	if (item) {
		return item->active;
	}
	return FALSE;
}

bool pluginfiltergroup_all_active(const PluginFilterGroup* self)
{
	bool rv;
	const psy_List* p;

	rv = TRUE;
	for (p = self->items; p != NULL; p = p->next) {
		const PluginFilterItem* curr;
		
		curr = (const PluginFilterItem*)p->entry;
		if (curr->active == FALSE) {
			rv = FALSE;
		}
	}
	return rv;
}

bool pluginfiltergroup_exists(const PluginFilterGroup* self, uintptr_t key)
{
	return (pluginfiltergroup_item((PluginFilterGroup*)self, key) != NULL);
}

PluginFilterItem* pluginfiltergroup_item(PluginFilterGroup* self, uintptr_t id)
{
	PluginFilterItem* rv;
	
	psy_List* p;
	rv = NULL;
	for (p = self->items; p != NULL; p = p->next) {
		PluginFilterItem* curr;
		
		curr = (PluginFilterItem*)p->entry;
		if (curr->key && id == curr->key) {
			rv = curr;
			break;
		}
	}
	return rv;
}

/* PluginFilter */

void pluginfilter_init(PluginFilter* self)
{
	assert(self);

	self->search_text = NULL;	
	pluginfiltergroup_init(&self->mode, psy_strhash("mode"), "Mode");
	pluginfiltergroup_add(&self->mode,
		psy_strhash("generator"), "Generator", TRUE);
	pluginfiltergroup_add(&self->mode,
		psy_strhash("effect"), "Effect", TRUE);		
	pluginfiltergroup_init(&self->types, psy_strhash("type"), "Type");
	pluginfiltergroup_add(&self->types,
		psy_strhash("intern"), "Internal", TRUE);
	pluginfiltergroup_add(&self->types,
		psy_strhash("native"), "Native", TRUE);
	pluginfiltergroup_add(&self->types,
		psy_strhash("vst"), "Vst", TRUE);
	pluginfiltergroup_add(&self->types,
		psy_strhash("lua"), "Lua", TRUE);		
	pluginfiltergroup_add(&self->types,
		psy_strhash("ladspa"), "Ladspa", TRUE);
	pluginfiltergroup_add(&self->types,
		psy_strhash("lv2"), "LV2", TRUE);
	pluginfiltergroup_init(&self->categories, psy_strhash("categories"),
		"newmachine.categories");
	pluginfiltergroup_init(&self->sort, psy_strhash("sort"),
		"newmachine.sort");		
	pluginfiltergroup_add(&self->sort, PLUGINSSORTMODE_FAVORITE,
		"newmachine.favorite", FALSE);
	pluginfiltergroup_add(&self->sort, PLUGINSSORTMODE_NAME,
		"newmachine.name",  FALSE);
	pluginfiltergroup_add(&self->sort, PLUGINSSORTMODE_TYPE,
		"newmachine.type", FALSE);
	pluginfiltergroup_add(&self->sort, PLUGINSSORTMODE_MODE,
		"newmachine.mode", FALSE);	
	psy_signal_init(&self->signal_changed);
}

void pluginfilter_dispose(PluginFilter* self)
{		
	pluginfiltergroup_dispose(&self->mode);
	pluginfiltergroup_dispose(&self->types);
	pluginfiltergroup_dispose(&self->categories);
	pluginfiltergroup_dispose(&self->sort);
	psy_signal_dispose(&self->signal_changed);
	free(self->search_text);
	self->search_text = NULL;	
}

void pluginfilter_notify(PluginFilter* self)
{	
	psy_signal_emit(&self->signal_changed, self, 0);
}

void pluginfilter_select_all(PluginFilter* self)
{	
	psy_strreset(&self->search_text, "");
	pluginfiltergroup_select_all(&self->mode);
	pluginfiltergroup_select_all(&self->types);	
	pluginfiltergroup_select_all(&self->categories);
	pluginfilter_notify(self);
}

void pluginfilter_set_search_text(PluginFilter* self, const char* text)
{
	psy_strreset(&self->search_text, text);
	pluginfilter_notify(self);
}


/* psy_audio_Plugins */

/* prototypes */
static void psy_audio_plugins_set_category(psy_audio_Plugins*, const char* key,
	const char* category);

/* implementation */
void psy_audio_plugins_init(psy_audio_Plugins* self)
{
	char ini_path[_MAX_PATH];

	assert(self);

	ini_path[0] = '\0';
	if (psy_dir_config()) {
		psy_snprintf(ini_path, _MAX_PATH, "%s", psy_dir_config());
	}	
#if (DIVERSALIS__CPU__SIZEOF_POINTER == 4)
	strcat(ini_path, SLASH "psycle-plugin-scanner-cache32.ini");
#else
	strcat(ini_path, SLASH "psycle-plugin-scanner-cache64.ini");
#endif
	self->ini_path_ = psy_strdup(ini_path);
	self->plugins_ = NULL;
	psy_audio_plugins_reset(self);
	
}

void psy_audio_plugins_dispose(psy_audio_Plugins* self)
{	
	assert(self);

	free(self->ini_path_);
	self->ini_path_ = NULL;
	psy_list_deallocate(&self->plugins_, (psy_fp_disposefunc)
		machineinfo_dispose);
}

void psy_audio_plugins_copy(psy_audio_Plugins* self,
	const psy_audio_Plugins* other)
{
	assert(self);
	assert(other);
	
	if (self != other) {
		psy_List* p;
		
		psy_audio_plugins_clear(self);
		for (p = other->plugins_; p != NULL; p = p->next) {			
			psy_audio_MachineInfo* source;
			psy_audio_MachineInfo* copy;

			source = (psy_audio_MachineInfo*)psy_list_entry(p);
			copy = machineinfo_clone(source);
			if (copy) {				
				psy_list_append(&self->plugins_, (void*)copy);
			}			
		}
	}
}

void psy_audio_plugins_reset(psy_audio_Plugins* self)
{
	psy_audio_plugins_clear(self);
}

void psy_audio_plugins_clear(psy_audio_Plugins* self)
{	
	psy_list_deallocate(&self->plugins_, (psy_fp_disposefunc)
		machineinfo_dispose);		
}

int psy_audio_plugins_load(psy_audio_Plugins* self)
{
	int rv;
	psy_PropertyReader propertyreader;	
	psy_Property properties;
	psy_Property* plugins;	
	
	psy_audio_plugins_reset(self);
	psy_property_init_key(&properties, NULL);
	psy_property_set_comment(&properties,
		"Psycle Plugin Scanner Cache created by\r\n; " PSYCLE__BUILD__IDENTIFIER("\r\n; "));
	plugins = psy_property_append_section(&properties, "plugins");	
	psy_propertyreader_init(&propertyreader, &properties, self->ini_path_);
	psy_propertyreader_allow_append(&propertyreader);	
	rv = psy_propertyreader_load(&propertyreader);	
	psy_propertyreader_dispose(&propertyreader);	
	if (plugins) {
		psy_List* p;
		
		for (p = psy_property_begin(plugins); p != NULL; p = p->next) {
			psy_Property* property;
			psy_audio_MachineInfo* plugin;

			property = (psy_Property*)psy_list_entry(p);
			plugin = machineinfo_alloc();
			if (plugin) {
				psy_audio_machineinfo_init_property(plugin, property);
				psy_list_append(&self->plugins_, (void*)plugin);
			}			
		}
	}
	psy_property_dispose(&properties);	
	return rv;
}

int psy_audio_plugins_save(psy_audio_Plugins* self)
{
	int rv;
	psy_PropertyWriter propertywriter;
	psy_Property properties;
	psy_Property* plugins;
	psy_List* p;
	
	psy_property_init_key(&properties, NULL);
	psy_property_set_comment(&properties,
		"Psycle Plugin Scanner Cache created by\r\n; " PSYCLE__BUILD__IDENTIFIER("\r\n; "));
	plugins = psy_property_append_section(&properties, "plugins");
	for (p = self->plugins_; p != NULL; psy_list_next(&p)) {
		psy_audio_MachineInfo* plugin;
		
		plugin = (psy_audio_MachineInfo*)psy_list_entry(p);
		psy_audio_machineinfo_make(plugins, plugin);	
	}
	psy_propertywriter_init(&propertywriter, &properties, self->ini_path_);
	rv = psy_propertywriter_save(&propertywriter);
	psy_propertywriter_dispose(&propertywriter);
	psy_property_dispose(&properties);
	return rv;
}

void psy_audio_plugins_add(psy_audio_Plugins* self, const psy_audio_MachineInfo*
	macinfo)
{		
	if (macinfo) {				
		psy_audio_MachineInfo* copy;
		
		copy = machineinfo_clone(macinfo);
		if (copy) {				
			psy_list_append(&self->plugins_, (void*)copy);
		}		
	}
}

void psy_audio_plugins_remove(psy_audio_Plugins* self, const char* id)
{
	
}

bool psy_audio_plugins_exists(psy_audio_Plugins* self,
	const psy_audio_MachineInfo* plugin)
{	
	const psy_audio_MachineInfo* search;
	char key[512];
				
	machineinfo_catchername(plugin, key);
	search = psy_audio_plugins_at_id_const(self, key);
	if (search && search->type == plugin->type) {
		return TRUE;
	}	
	return FALSE;
}

uintptr_t psy_audio_plugins_size(const psy_audio_Plugins* self)
{
	return psy_list_size(self->plugins_);	
}

psy_List* psy_audio_plugins_begin(psy_audio_Plugins* self)
{
	return self->plugins_;
}

const psy_audio_MachineInfo* psy_audio_plugins_at_const(
	const psy_audio_Plugins* self, uintptr_t index)
{
	return psy_list_entry_at_const(self->plugins_, index);	
}

const psy_audio_MachineInfo* psy_audio_plugins_at_id_const(
	const psy_audio_Plugins* self, const char* key)
{
	const psy_audio_MachineInfo* rv;
	const psy_List* p;	
	
	rv = NULL;
	for (p = self->plugins_; p != NULL; p = p->next) {
		const psy_audio_MachineInfo* plugin;
		char curr_key[512];
		
		plugin = (const psy_audio_MachineInfo*)p->entry;
		machineinfo_catchername(plugin, curr_key);		
		if (strcmp(curr_key, key) == 0) {
			rv = plugin;
			break;			
		}
	}
	return rv;
}

psy_audio_MachineInfo* psy_audio_plugins_at_id(psy_audio_Plugins* self,
	const char* key)
{
	return (psy_audio_MachineInfo*)psy_audio_plugins_at_id_const(self, key);	
}


/* psy_audio_PluginScanTask */
void psy_audio_pluginscantask_init_all(psy_audio_PluginScanTask* self,
	psy_audio_MachineType type, const char* wildcard, const char* label,
	const char* key, int recursive)
{
	self->type = type;
	psy_snprintf(self->wildcard, 255, "%s", wildcard);
	psy_snprintf(self->label, 255, "%s", label);
	psy_snprintf(self->key, 255, "%s", key);
	self->recursive = recursive;
}

/*void psy_audio_plugincategories_update(psy_audio_PluginCategories* self,
	const psy_audio_Plugins* plugins)
{
	psy_List* p;	
	
	assert(self);

	psy_table_dispose_all(&self->container, NULL);
	psy_table_init(&self->container);
	if (!plugins) {
		return;
	}	
	for (p = psy_audio_plugins_begin((psy_audio_Plugins*)plugins); p != NULL;
			psy_list_next(&p)) {
		const psy_audio_MachineInfo* q;
		const char* category;		

		q = (const psy_audio_MachineInfo*)psy_list_entry(p);
		category = q->category;
		if (category && (psy_strlen(category) > 0) &&
				(!psy_table_exists_strhash(&self->container, category))) {
			psy_table_insert_strhash(&self->container, category,
				(void*)psy_strdup(category));
		}		
	}
}*/

// psy_TableIterator psy_audio_plugincategories_begin(
// 	psy_audio_PluginCategories* self)
// {
// 	return psy_table_begin(&self->container);
//}

static void searchfilter(const psy_audio_MachineInfo* plugin, PluginFilter*,
	psy_audio_Plugins* parent);

void psy_audio_plugins_filter(psy_audio_Plugins* self, PluginFilter* filter,
	psy_audio_Plugins* rv)
{
	psy_List* p;		

	assert(filter);
	assert(rv);

	psy_audio_plugins_clear(rv);	
	for (p = psy_audio_plugins_begin(self); p != NULL; psy_list_next(&p)) {
		const psy_audio_MachineInfo* plugin;

		plugin = (psy_audio_MachineInfo*)psy_list_entry(p);
		if (psy_strlen(filter->search_text) == 0) {
			searchfilter(plugin, filter, rv);
		} else if (strcasestr(plugin->name, filter->search_text)) {
			searchfilter(plugin, filter, rv);
		}
	}	
}

static bool internal_type(intptr_t type);

void searchfilter(const psy_audio_MachineInfo* plugin, PluginFilter* filter,
	psy_audio_Plugins* parent)
{	
	bool intern;
	intptr_t mactype;
	intptr_t macmode;
	const char* category;
				
	mactype = plugin->type;
	intern = internal_type(mactype);
	macmode = plugin->mode;
	category = plugin->category;
	if ((psy_strlen(category) > 0) && (!pluginfiltergroup_selected(
			&filter->categories, psy_strhash(category)))) {		
		return;
	}	
	if (!pluginfiltergroup_selected(&filter->types, psy_strhash("vst")) &&
			(mactype == psy_audio_VST || mactype == psy_audio_VSTFX)) {		
		return;
	}	
	if (!pluginfiltergroup_selected(&filter->types, psy_strhash("native")) &&
			mactype == psy_audio_PLUGIN) {
		return;
	}	
	if (!pluginfiltergroup_selected(&filter->types, psy_strhash("ladspa")) &&
			mactype == psy_audio_LADSPA) {		
		return;
	}
	if (!pluginfiltergroup_selected(&filter->types, psy_strhash("lv2")) &&
			mactype == psy_audio_LV2) {
		return;
	}	
	if (!pluginfiltergroup_selected(&filter->types, psy_strhash("lua")) &&
			mactype == psy_audio_LUA) {
		return;
	}
	if (!pluginfiltergroup_selected(&filter->types, psy_strhash("intern")) &&
			intern) {
		return;
	}	
	if (!pluginfiltergroup_selected(&filter->mode, psy_strhash("effect")) &&
			(macmode == psy_audio_MACHMODE_FX)) {		
		return;		
	}
	if (!pluginfiltergroup_selected(&filter->mode, psy_strhash("generator")) &&
			(macmode == psy_audio_MACHMODE_GENERATOR)) {
		return;
	}
	psy_audio_plugins_add(parent, plugin);
}

bool internal_type(intptr_t type)
{	
	return (!(type == psy_audio_PLUGIN ||
		type == psy_audio_LUA ||
		type == psy_audio_VST ||
		type == psy_audio_VSTFX ||
		type == psy_audio_LADSPA));
}

void psy_audio_plugins_sort(psy_audio_Plugins* self, PluginFilter* filter,
	psy_audio_Plugins* rv)
{	
	if (pluginfiltergroup_selected(&filter->sort, PLUGINSSORTMODE_NAME)) {
		plugins_sort(self, (psy_fp_comp)plugins_comp_name, rv);
	}
	if (pluginfiltergroup_selected(&filter->sort, PLUGINSSORTMODE_TYPE)) {				
		psy_audio_Plugins sort;
				
		psy_audio_plugins_init(&sort);
		psy_audio_plugins_copy(&sort, rv);
		plugins_sort(&sort, (psy_fp_comp)plugins_comp_type, rv);		
		psy_audio_plugins_dispose(&sort);		
	}
	if (pluginfiltergroup_selected(&filter->sort, PLUGINSSORTMODE_MODE)) {
		psy_audio_Plugins sort;
				
		psy_audio_plugins_init(&sort);
		psy_audio_plugins_copy(&sort, rv);
		plugins_sort(&sort, (psy_fp_comp)plugins_comp_mode, rv);		
		psy_audio_plugins_dispose(&sort);		
	}
}

void psy_audio_plugins_set_native_categories(psy_audio_Plugins* self)
{	
	psy_audio_plugins_set_category(self, "alk-muter:0", "mixer");
	psy_audio_plugins_set_category(self, "arguru-compressor:0", "compressor");
	psy_audio_plugins_set_category(self, "arguru-distortion:0", "distortion");
	psy_audio_plugins_set_category(self, "arguru-freeverb:0", "reverb");
	psy_audio_plugins_set_category(self, "arguru-goaslicer:0", "effect");
	psy_audio_plugins_set_category(self, "arguru-reverb:0", "reverb");
	psy_audio_plugins_set_category(self, "arguru-synth-2f:0", "synth");
	psy_audio_plugins_set_category(self, "arguru-xfilter:0", "filter");
	psy_audio_plugins_set_category(self, "audacity-compressor:0", "compressor");	
	psy_audio_plugins_set_category(self, "audacity-phaser:0", "phaser");
	psy_audio_plugins_set_category(self, "bexphase:0", "phaser");
	psy_audio_plugins_set_category(self, "blitz12:0", "synth");
	psy_audio_plugins_set_category(self, "blitzn:0", "synth");
	psy_audio_plugins_set_category(self, "crasher:0", "unknown");
	psy_audio_plugins_set_category(self, "delay:0", "delay");
	psy_audio_plugins_set_category(self, "distortion:0", "distortion");
	psy_audio_plugins_set_category(self, "dw-eq:0", "EQ");
	psy_audio_plugins_set_category(self, "dw-granulizer:0", "unknown");
	psy_audio_plugins_set_category(self, "dw-iopan:0", "unknown");
	psy_audio_plugins_set_category(self, "dw-tremolo:0", "unknown");
	psy_audio_plugins_set_category(self, "eq3:0", "EQ");
	psy_audio_plugins_set_category(self, "feedme:0", "unknown");
	psy_audio_plugins_set_category(self, "filter-2-poles:0", "unknown");
	psy_audio_plugins_set_category(self, "flanger:0", "unknown");
	psy_audio_plugins_set_category(self, "gainer:0", "unknown");
	psy_audio_plugins_set_category(self, "gamefx13:0", "drum");
	psy_audio_plugins_set_category(self, "gamefxn:0", "drum");
	psy_audio_plugins_set_category(self, "haas:0", "unknown");
	psy_audio_plugins_set_category(self, "jmdrum:0", "drum");
	psy_audio_plugins_set_category(self, "karlkox-surround:0", "unknown");
	psy_audio_plugins_set_category(self, "koruz:0", "unknown");
	psy_audio_plugins_set_category(self, "ladspa-gverb:0", "unknown");
	psy_audio_plugins_set_category(self, "legasynth-303:0", "unknown");
	psy_audio_plugins_set_category(self, "m3:0", "unknown");
	psy_audio_plugins_set_category(self, "maeq:0", "unknown");
	psy_audio_plugins_set_category(self, "negative:0", "unknown");
	psy_audio_plugins_set_category(self, "nrs-7900-fractal:0", "unknown");
	psy_audio_plugins_set_category(self, "phantom:0", "synth");
	psy_audio_plugins_set_category(self, "pluckedstring:0", "synth");
	psy_audio_plugins_set_category(self, "pooplog-autopan:0", "unknown");
	psy_audio_plugins_set_category(self, "pooplog-delay-light:0", "delay");
	psy_audio_plugins_set_category(self, "pooplog-delay:0", "delay");
	psy_audio_plugins_set_category(self, "pooplog-filter:0", "filter");
	psy_audio_plugins_set_category(self, "pooplog-fm-laboratory:0", "synth");
	psy_audio_plugins_set_category(self, "pooplog-fm-light:0", "synth");
	psy_audio_plugins_set_category(self, "pooplog-fm-ultralight:0", "synth");
	psy_audio_plugins_set_category(self, "pooplog-lofi-processor:0", "unknown");
	psy_audio_plugins_set_category(self, "pooplog-scratch-master-2:0", "unknown");
	psy_audio_plugins_set_category(self, "pooplog-scratch-master:0", "unknown");
	psy_audio_plugins_set_category(self, "ring-modulator:0", "unknown");
	psy_audio_plugins_set_category(self, "s-chorus:0", "chorus");
	psy_audio_plugins_set_category(self, "slicit:0", "unknown");
	psy_audio_plugins_set_category(self, "stk-plucked:0", "synth");
	psy_audio_plugins_set_category(self, "stk-reverbs:0", "unknown");
	psy_audio_plugins_set_category(self, "stk-shakers:0", "unknown");
	psy_audio_plugins_set_category(self, "softsat:0", "unknown");
	psy_audio_plugins_set_category(self, "sublime:0", "unknown");
	psy_audio_plugins_set_category(self, "thunderpalace-softsat:0", "unknown");
	psy_audio_plugins_set_category(self, "vdallpass:0", "unknown");
	psy_audio_plugins_set_category(self, "vdecho:0", "unknown");
	psy_audio_plugins_set_category(self, "vdnoisegate:0", "unknown");
	psy_audio_plugins_set_category(self, "vsfastverb:0", "reverb");
	psy_audio_plugins_set_category(self, "wahwah:0", "effect");
	psy_audio_plugins_set_category(self, "ymidi:0", "midi");
	psy_audio_plugins_set_category(self, "zephod-superfm:0", "unknown");
	psy_audio_plugins_set_category(self, "singlefrequency:0", "unknown");
	psy_audio_plugins_set_category(self, "s-filter:0", "effect");
	psy_audio_plugins_set_category(self, "s-reverb:0", "reverb");
}

void psy_audio_plugins_set_category(psy_audio_Plugins* self, const char* key,
	const char* category)
{
	psy_audio_MachineInfo* plugin;
	
	plugin = psy_audio_plugins_at_id(self, key);
	if (plugin) {
		psy_strreset(&plugin->category, category);
	}
}

void psy_audio_plugins_update_filter_categories(psy_audio_Plugins* self,
	PluginFilter* filter)
{
	psy_List* p;
	
	assert(self);
	assert(filter);
		
	pluginfiltergroup_clear(&filter->categories);	
	for (p = psy_audio_plugins_begin(self); p != NULL; psy_list_next(&p)) {
		const psy_audio_MachineInfo* plugin;

		plugin = (const psy_audio_MachineInfo*)psy_list_entry(p);
		if (psy_strlen(plugin->category) > 0 && !pluginfiltergroup_exists(
			&filter->categories, psy_strhash(plugin->category))) {				
				pluginfiltergroup_add(&filter->categories, psy_strhash(
					plugin->category), plugin->category, TRUE);
		}
	}
}
