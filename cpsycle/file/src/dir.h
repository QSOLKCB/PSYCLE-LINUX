/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_DIR_H
#define psy_DIR_H

#include "../../detail/psydef.h"
/* container */
#include <list.h>
/* platform */
#include "../../detail/portable.h"

#ifdef __cplusplus
extern "C" {
#endif

struct FileSearch;

typedef struct psy_Path {
	char* path;
	char* prefix;
	char* name;
	char* ext;
	char* filename;
	struct stat st;
} psy_Path;

void psy_path_init(psy_Path*, const char* path);
void psy_path_init_file(psy_Path*, const char* path);
void psy_path_init_all(psy_Path*, const char* prefix, const char* name,
	const char* ext);
void psy_path_dispose(psy_Path*);

void psy_path_clear(psy_Path*);
void psy_path_copy(psy_Path*, const psy_Path* other);
void psy_path_set_path(psy_Path*, const char* path);
void psy_path_set_prefix(psy_Path*, const char* prefix);
void psy_path_set_name(psy_Path*, const char* name);
void psy_path_set_ext(psy_Path*, const char* name);
void psy_path_append_dir(psy_Path*, const char* dir);
bool psy_path_remove_dir(psy_Path*);

INLINE const char* psy_path_full(const psy_Path* self)
{
	return self->path;
}

INLINE const char* psy_path_name(const psy_Path* self)
{
	return self->name;
}

INLINE const char* psy_path_prefix(const psy_Path* self)
{
	return self->prefix;
}

INLINE const char* psy_path_ext(const psy_Path* self)
{
	return self->ext;
}

INLINE const char* psy_path_filename(const psy_Path* self)
{
	return self->filename;
}

INLINE const char* psy_path_dir(const psy_Path* self)
{
	if (S_ISDIR(self->st.st_mode)) {
		return self->path;
	}
	return self->prefix;
}

bool psy_path_has_prefix(const psy_Path*);
bool psy_path_has_ext(const psy_Path*);
bool psy_path_has_name(const psy_Path*);

uintptr_t psy_path_max(void);

typedef int (*psy_fp_findfile)(void* context, const char* path, int flag);

void psy_dir_enumerate(void* self, const char* path, const char* wildcard,
	int flag, psy_fp_findfile enumproc);
int psy_dir_enumerate_recursive(void* self, const char* path,
	const char* wildcard, int flag, psy_fp_findfile enumproc);
void psy_dir_find_file(const char* searchpath, const char* wildcard, char* filepath);
const char* psy_dir_config(void);
const char* psy_dir_home(void);
char* psy_workdir(char* buffer);
const char* pathenv(void);
void setpathenv(const char* path);
void psy_insertpathenv(const char* path);
void psy_mkdir(const char* path);
void psy_chdir(const char* path);
int psy_remove(const char* path);
bool psy_dir_exists(const char* path);
bool psy_file_readable(const char* fname);
psy_List* psy_drives(void);
psy_List* psy_directories(const char* path);
psy_List* psy_files(const char* path, const char* wildcard, int recursive);

uintptr_t psy_file_size(const char* path);
bool psy_file_is_directory(const char* path);

bool psy_dir_normalize(const char* path, char* rv);

#ifdef __cplusplus
}
#endif

#endif /* psy_DIR_H */

