/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "filechooser.h"

/* host */
#include "fileview.h"
#include "workspace.h"
/* ui */
#include <uifolderdialog.h>
#include <uiopendialog.h>
#include <uisavedialog.h>


/* prototypes */
static void filechooser_on_opendialog_file_name_change(FileChooser*,
	psy_ui_OpenDialog* sender);
static void filechooser_execute_wait(FileChooser*, psy_FileSelect* load,
	psy_FileSelect* save, psy_FileSelect* dir);
static void filechooser_execute(FileChooser*, psy_FileSelect* load,
	psy_FileSelect* save, psy_FileSelect* dir);

/* vtable */
static psy_DiskOpVtable filcechooser_vtable;
static bool filcechooser_vtable_initialized = FALSE;

static void filcechooser_vtable_init(FileChooser* self)
{
	assert(self);
	
	if (!filcechooser_vtable_initialized) {
		filcechooser_vtable = *(self->disk_op.vtable);
		filcechooser_vtable.execute =
			(psy_fp_diskop_execute)
			filechooser_execute;
		filcechooser_vtable.execute_wait =
			(psy_fp_diskop_execute)
			filechooser_execute_wait;
		filcechooser_vtable_initialized = TRUE;
	}
	self->disk_op.vtable = &filcechooser_vtable;
}

/* implementation */
void filechooser_init(FileChooser* self, Workspace* workspace)
{
	assert(self);
	
	psy_diskop_init(&self->disk_op);
	filcechooser_vtable_init(self);
	self->workspace_ = workspace;
	self->file_view_ = NULL;
	self->use_file_view_ = FALSE;
	self->curr_file_select_ = NULL;
}

void filechooser_dispose(FileChooser* self)
{
	assert(self);

}

void filechooser_set_file_view(FileChooser* self, FileView* file_view)
{
	assert(self);
	
	self->file_view_ = file_view;	
}

void filechooser_use_file_view(FileChooser* self)
{
	assert(self);

	self->use_file_view_ = TRUE;
}

void filechooser_use_native(FileChooser* self)
{
	assert(self);

	self->use_file_view_ = FALSE;
}

void filechooser_execute_wait(FileChooser* self, psy_FileSelect* load,
	psy_FileSelect* save, psy_FileSelect* dir)
{
	assert(self);
		
	filechooser_execute(self, load, save, dir);
	psy_ui_app_wait(psy_ui_app());
}	

void filechooser_execute(FileChooser* self, psy_FileSelect* load,
	psy_FileSelect* save, psy_FileSelect* dir)
{
	assert(self);
	
	workspace_save_view(self->workspace_);
	self->curr_file_select_ = NULL;
	if (self->use_file_view_) {
		if (self->file_view_) {
			if (!psy_fileselect_equal_connections(&self->file_view_->select_load_, load)) {
				fileview_set_load_select(self->file_view_, load);
			}
			if (!psy_fileselect_equal_connections(&self->file_view_->select_save_, save)) {
				fileview_set_save_select(self->file_view_, save);
			}
			if (!psy_fileselect_equal_connections(&self->file_view_->select_dir_, dir)) {
				fileview_set_dir_select(self->file_view_, dir);
			}
		}
	} else if (load) {
		psy_ui_OpenDialog dialog;		
		static char filter[1024];

		psy_fileselect_filter_str(load, filter, 1024);
		psy_ui_opendialog_init_all(&dialog, 0, psy_fileselect_title(load),
			filter, psy_fileselect_default_extension(load),
			psy_fileselect_directory(load));
		if (load->has_preview) {
			psy_signal_connect(&dialog.signal_filename_changed, self,
				filechooser_on_opendialog_file_name_change);
		}
		self->curr_file_select_ = load;
		if (psy_ui_opendialog_execute(&dialog)) {
			psy_fileselect_set_value(load, psy_path_full(
				psy_ui_opendialog_path(&dialog)));
			psy_fileselect_notify(load);			
		}
		self->curr_file_select_ = NULL;
		psy_ui_opendialog_dispose(&dialog);
	} else if (save) {
		psy_ui_SaveDialog dialog;		
		static char filter[1024];

		psy_fileselect_filter_str(save, filter, 1024);
		psy_ui_savedialog_init_all(&dialog, 0, psy_fileselect_title(save),
			filter, psy_fileselect_default_extension(save),
			psy_fileselect_directory(save));
		//dialog.prevent_preview = !load->has_preview;
		if (psy_ui_savedialog_execute(&dialog)) {
			psy_fileselect_set_value(save, psy_path_full(
				psy_ui_savedialog_path(&dialog)));
			psy_fileselect_notify(save);			
		}
		psy_ui_savedialog_dispose(&dialog);
	} else if (dir) {
		psy_ui_FolderDialog dialog;		
		
		psy_ui_folderdialog_init_all(&dialog, NULL, psy_fileselect_title(dir),
			psy_fileselect_directory(dir));
		if (psy_ui_folderdialog_execute(&dialog)) {
			psy_fileselect_set_value(dir, psy_ui_folderdialog_path(&dialog));
			psy_fileselect_notify(dir);			
		}
		psy_ui_folderdialog_dispose(&dialog);
	}
	if (self->use_file_view_) {	
		workspace_select_view(self->workspace_, viewindex_make(
			VIEW_ID_FILEVIEW));
		/* A disk operation that opens the embedded FileView must leave the
		** keyboard in the file list, rather than depending on whichever
		** component happened to own focus before the view switch.  This keeps
		** navigation/selection deterministic for tracker-first workflows. */
		if (self->file_view_) {
			psy_ui_component_set_focus(filebox_base(&self->file_view_->file_box_));
		}
	}
}

void filechooser_on_opendialog_file_name_change(FileChooser* self,
	psy_ui_OpenDialog* sender)
{
	assert(self);
	
	if (self->curr_file_select_) {
		psy_fileselect_set_value(self->curr_file_select_, psy_path_full(
			psy_ui_opendialog_path(sender)));
		psy_fileselect_notify_file_name_change(self->curr_file_select_);
	}
}
