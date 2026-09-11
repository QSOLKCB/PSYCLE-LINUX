/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(STARTSCRIPT_H)
#define STARTSCRIPT_H

/* host */
#include "workspace.h"
/* audio */
#include <psyclescript.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Link {
 /*! @internal */
   char* label_;
   char* path_;
   int viewport_;
   int userinterface_;
} Link;

void link_init(Link*);
void link_init_all(Link*, const char* path, const char* label,
	int viewport, int user_interface);
void link_dispose(Link*);

Link* link_alloc(void);
Link* link_alloc_init(void);
Link* link_clone(const Link*);
void link_copy(Link*, const Link* other);

const char* link_path(const Link*);

typedef struct Links {
	psy_Table container;
} Links;

void links_init(Links*);
void links_dispose(Links*);

void links_add(Links*, const Link* link);
const Link* links_at(const Links*, uintptr_t index);

struct MainFrame;

typedef struct StartScript {	
	psy_PsycleScript script;        
	/* references */
	struct MainFrame* mainframe;
} StartScript;

void startscript_init(StartScript*, struct MainFrame* mainframe);
void startscript_dispose(StartScript*);

void startscript_run(StartScript*);

#ifdef __cplusplus
}
#endif

#endif /* STARTSCRIPT_H */
