/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(FILEBOX_H)
#define FILEBOX_H

#include "playlist.h"
/* ui */
#include <uibutton.h>
#include <uilabel.h>
#include <uiscroller.h>
/* file */
#include <dir.h>
#include "../../detail/portable.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct FileLine
** @brief A file or dir item in the FileBox
*/
typedef struct FileLine {
	/*! @extends */
	psy_ui_Component component;
	/* signal */
	psy_Signal signal_selected;
	/*! @internal */
	psy_ui_Button preview;
	psy_ui_Button name;
	psy_ui_Label size;	
	char* path;
	struct stat st;
} FileLine;

void fileline_init(FileLine*, psy_ui_Component* parent, const char* path,
	const char* title, struct stat st, bool has_preview);

FileLine* fileline_alloc(void);
FileLine* fileline_alloc_init(psy_ui_Component* parent, const char* path,
	const char* title, struct stat st, bool has_preview);

INLINE psy_ui_Component* fileline_base(FileLine* self)
{
	return &self->component;
}


struct InputHandler;

#define FILEBOX_MAX_STACK 16
/*!
** @struct FileBox
** @brief A file/dir listview
*/
typedef struct FileBox {
	/*! @extends */
	psy_ui_Scroller scroller;
	/* signal */
	psy_Signal signal_selected;
	psy_Signal signal_dir_changed;
	psy_Signal signal_preview;
	/*! @internal */	
	psy_ui_Component pane;		
	uintptr_t previewindex;
	psy_Path curr_dir;
	psy_Path restore_dir;
	bool rebuild;
	char* wildcard;
	bool dirsonly;
	bool has_preview;
	bool read_from_file_list;
	double top_stack[FILEBOX_MAX_STACK];
	uintptr_t sel_indexes[FILEBOX_MAX_STACK];
	intptr_t stack_position;
	uintptr_t num_lines_;
	uintptr_t pg_step;
} FileBox;

void filebox_init(FileBox*, psy_ui_Component* parent);

void filebox_read(FileBox*, const char* path);
uintptr_t filebox_selected(const FileBox*);
void filebox_set_wildcard(FileBox*, const char* wildcard);
void filebox_set_dir(FileBox*, const char* path);
const char* filebox_dir(const FileBox*);
const char* filebox_file_name(const FileBox*);
const char* filebox_preview_name(const FileBox*);
void filebox_full_name(const FileBox*, char* rv, uintptr_t maxlen);
void filebox_full_preview_name(const FileBox*, char* rv, uintptr_t maxlen);
void filebox_refresh(FileBox*);
void filebox_rebuild(FileBox*);
void filebox_enable_preview(FileBox*);
void filebox_disable_preview(FileBox*);
void filebox_show_only_directories(FileBox*);
void filebox_show_files_and_directories(FileBox*);
void filebox_execute_next(FileBox*);
bool filebox_execute_line(FileBox*);

INLINE psy_ui_Component* filebox_base(FileBox* self)
{
	return psy_ui_scroller_base(&self->scroller);
}

#ifdef __cplusplus
}
#endif

#endif /* FILEBOX_H */
