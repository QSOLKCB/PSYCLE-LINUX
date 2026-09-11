/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(FILECHOOSER_H)
#define FILECHOOSER_H

#include <fileselect.h>

#ifdef __cplusplus
extern "C" {
#endif
	
struct FileView;	
struct Workspace;
	
/*!
** @struct FileChooser
** @brief Implementation of psy_DiskOp using either the os or the ft2 file view
*/
typedef struct FileChooser {
	/*! @implements */
	psy_DiskOp disk_op;
	/*! @internal */
	struct FileView* file_view_;
	psy_FileSelect* curr_file_select_;
	bool use_file_view_;
	struct Workspace* workspace_;
} FileChooser;

void filechooser_init(FileChooser*, struct Workspace*);
void filechooser_dispose(FileChooser*);

void filechooser_set_file_view(FileChooser*, struct FileView*);
void filechooser_use_file_view(FileChooser*);
void filechooser_use_native(FileChooser*);

INLINE psy_DiskOp* filechooser_base(FileChooser* self)
{
	assert(self);

	return &self->disk_op;
}

#ifdef __cplusplus
}
#endif

#endif /* FILECHOOSER_H */
