/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "playlist.h"
/* file */
#include <dir.h>
#include <filereader.h>
/* container */
#include <tokenizer.h>
/* platform */
#include "../../detail/portable.h"

#define SLEEP_BETWEEN_STOP_SONG_LOAD_NS 200000

void playlistiteminfo_init(PlayListItemInfo* self, uintptr_t length_s,
	const char* title, const char* album)
{
	assert(self);

	self->length_s = length_s;
	self->title = psy_strdup(title);
	self->album = psy_strdup(album);
}

void playlistiteminfo_dispose(PlayListItemInfo* self)
{
	assert(self);

	free(self->title);
	self->title = NULL;
	free(self->album);
	self->album = NULL;
}

PlayListItemInfo* playlistiteminfo_clone(const PlayListItemInfo* other)
{
	PlayListItemInfo* rv;

	assert(other);

	rv = (PlayListItemInfo*)malloc(sizeof(PlayListItemInfo));
	if (rv) {
		rv->length_s = other->length_s;
		rv->title = psy_strdup(other->title);
		rv->album = psy_strdup(other->album);
	}
	return rv;
}

void playlistiteminfo_clear(PlayListItemInfo* self)
{
	assert(self);

	self->length_s = 0;
	free(self->title);
	self->title = NULL;
	free(self->album);
	self->album = NULL;
}

void playlistiteminfo_set_title(PlayListItemInfo* self, const char* title)
{
	assert(self);

	psy_strreset(&self->title, title);
}

void playlistiteminfo_set_album(PlayListItemInfo* self, const char* title)
{
	assert(self);

	psy_strreset(&self->album, title);
}

bool playlistiteminfo_empty(const PlayListItemInfo* self)
{
	assert(self);

	return ((self->length_s == 0) && (psy_strlen(self->title) == 0) &&
		(psy_strlen(self->album) == 0));
}

/* PlayListItem */

/* implementation */
void playlistitem_init(PlayListItem* self, const char* path,
	const PlayListItemInfo* item_info)
{
	assert(self);

	self->path = psy_strdup(path);
	if (item_info) {
		self->item_info = playlistiteminfo_clone(item_info);
	} else {
		self->item_info = NULL;
	}
}

void playlistitem_dispose(PlayListItem* self)
{
	assert(self);

	free(self->path);
	self->path = NULL;
	if (self->item_info) {
		playlistiteminfo_dispose(self->item_info);
		free(self->item_info);
		self->item_info = NULL;
	}
}

PlayListItem* playlistitem_alloc(void)
{
	return (PlayListItem*)malloc(sizeof(PlayListItem));
}

PlayListItem* playlistitem_alloc_init(const char* path,
	const PlayListItemInfo* item_info)
{
	PlayListItem* rv;

	rv = playlistitem_alloc();
	if (rv) {
		playlistitem_init(rv, path, item_info);
	}
	return rv;
}

PlayListItem* playlistitem_clone(const PlayListItem* other)
{
	PlayListItem* rv;

	assert(other);

	rv = playlistitem_alloc();
	if (rv) {		
		playlistitem_init(rv, other->path, other->item_info);		
	}
	return rv;
}

/* PlayList */

/* prototypes */
static intptr_t playlist_parse_header(PlayList*, psy_Tokenizer*);
static intptr_t playlist_parse_path(PlayList*, psy_Tokenizer*,
	const PlayListItemInfo*);
static intptr_t playlist_parse_directive(PlayList*, psy_Tokenizer*,
	PlayListItemInfo*);
static intptr_t playlist_parse_title(PlayList*, psy_Tokenizer*);
static intptr_t playlist_parse_item_info(PlayList*, psy_Tokenizer*,
	PlayListItemInfo*);
static intptr_t playlist_parse_album(PlayList*, psy_Tokenizer*,
	PlayListItemInfo*);

/* implementation */
void playlist_init(PlayList* self, const char* path)
{
	assert(self);
		
	self->path = psy_strdup(path);	
	self->title = psy_strdup("");	
	self->items = NULL;	
}

void playlist_dispose(PlayList* self)
{
	assert(self);

	psy_list_deallocate(&self->items, (psy_fp_disposefunc)
		playlistitem_dispose);
	free(self->path);
	self->path = NULL;
	free(self->title);
	self->title = NULL;	
}

void playlist_clear(PlayList* self)
{
	assert(self);

	psy_list_deallocate(&self->items, (psy_fp_disposefunc)
		playlistitem_dispose);	
	free(self->title);
	self->title = NULL;
}

intptr_t playlist_load(PlayList* self)
{
	if (psy_file_readable(self->path)) {
		return playlist_parse(self);		
	}
	return PSY_ERRFILE;
}

void playlist_save(PlayList* self)
{
	FILE* fp;
	bool rv;

	assert(self);

	rv = TRUE;
	fp = fopen(self->path, "w");
	if (fp) {		
		const psy_List* p;		

		assert(self);
				
		fwrite("#EXTM3U", 1, psy_strlen("#EXTM3U"), fp);
		fputc('\n', fp);
		fputc('\n', fp);
		fwrite("#PLAYLIST:", 1, psy_strlen("#PLAYLIST:"), fp);
		if (psy_strlen(self->title) > 0) {
			fwrite(self->title, 1, psy_strlen(self->title), fp);
		}		
		fputc('\n', fp);
		p = self->items;
		while (p) {
			const PlayListItem* item;

			item = psy_list_entry_const(p);
			assert(item);
			if (item->item_info) {
				char length[64];

				fwrite("#EXTINF:", 1, psy_strlen("#EXTINF:"), fp);
				psy_snprintf(length, 64, "%d,", (int)item->item_info->length_s);
				fwrite(length, 1, psy_strlen(length), fp);
				fwrite(item->item_info->title, 1, psy_strlen(item->item_info->title), fp);
				fputc('\n', fp);
				if (psy_strlen(item->item_info->album) > 0) {
					fwrite("#EXTALB:", 1, psy_strlen("#EXTALB:"), fp);										
					fwrite(item->item_info->album, 1, psy_strlen(item->item_info->album), fp);
					fputc('\n', fp);
				}
			}
			fwrite(item->path, 1, psy_strlen(item->path), fp);
			fputc('\n', fp);
			p = p->next;
		}
		fclose(fp);		
	}	
}

void playlist_append(PlayList* self, const PlayListItem* item)
{
	assert(self);	

	if (!item) {
		return;
	}
	if (psy_strlen(item->path) > 0) {
		if (item->path[0] != 'C') {
			self = self;
		}
		psy_list_append(&self->items, playlistitem_clone(item));
	}
}

bool playlist_exist(PlayList* self, const char* filename)
{
	bool rv;
	const psy_List* p;	

	assert(self);
	
	rv = FALSE;	
	p = self->items;
	while (p) {
		const PlayListItem* item;

		item = psy_list_entry_const(p);
		assert(item);
		if (item->path && strcmp(item->path, filename) == 0) {
			rv = TRUE;
			break;
		}
		p = p->next;
	}			
	return rv;
}

void playlist_set_title(PlayList* self, const char* title)
{
	assert(self);

	psy_strreset(&self->title, title);
}

const char* playlist_title(const PlayList* self)
{
	assert(self);

	return self->title;
}


intptr_t playlist_parse(PlayList* self)
{		
	intptr_t status;
	psy_FileReader file_reader;
	char* buffer;
	psy_Tokenizer t;	
	PlayListItemInfo item_info;
	int ch;
	
	assert(self);

	playlist_clear(self);
	psy_filereader_init(&file_reader);	
	status = psy_filereader_load(&file_reader, self->path, &buffer);
	psy_filereader_dispose(&file_reader);
	if (status != PSY_OK) {		
		return status;
	}		
	playlistiteminfo_init(&item_info, 0, NULL, NULL);	
	psy_tokenizer_init(&t, buffer);
	status = playlist_parse_header(self, &t);
	if (status == PSY_OK) {
		ch = psy_tokenizer_skip_line(&t);
		if (ch == '\n') {
			while (TRUE) {
				ch = psy_tokenizer_curr_char(&t);
				if (ch == '\0') {
					break;
				}
				if (ch == '#') {
					playlist_parse_directive(self, &t, &item_info);
				} else {
					status = playlist_parse_path(self, &t, &item_info);				
					if (psy_strlen(psy_tokenizer_lexeme(&t)) > 0) {
						playlistiteminfo_clear(&item_info);
					}
				}
				if (!psy_tokenizer_has_next(&t)) {
					break;
				}
			}
		}
	}
	playlistiteminfo_dispose(&item_info);	
	psy_tokenizer_dispose(&t);
	free(buffer);
	buffer = NULL;
	return status;
}

intptr_t playlist_parse_header(PlayList* self, psy_Tokenizer* t)
{
	assert(self);
	assert(t);

	psy_tokenizer_next(t);
	if (!psy_tokenizer_check_delim(t, "#")) {				
		return PSY_ERRFILEFORMAT;
	}
	psy_tokenizer_next(t);
	if (!psy_tokenizer_check_ident(t, "EXTM3U")) {				
		return PSY_ERRFILEFORMAT;
	}
	return PSY_OK;
}

intptr_t playlist_parse_path(PlayList* self, psy_Tokenizer* t,
	const PlayListItemInfo* item_info)
{
	assert(self);
	assert(t);
	assert(item_info);

	psy_tokenizer_read_line(t);	
	if (psy_strlen(psy_tokenizer_lexeme(t)) > 0) {
		PlayListItem item;
		
		playlistitem_init(&item, psy_tokenizer_lexeme(t), item_info);
		playlist_append(self, &item);
		playlistitem_dispose(&item);
	}
	return PSY_OK;
}

intptr_t playlist_parse_directive(PlayList* self, psy_Tokenizer* t,
	PlayListItemInfo* item_info)
{	
	assert(self);
	assert(t);
	assert(item_info);	
	
	psy_tokenizer_next(t);
	if (!psy_tokenizer_has_next(t)) {
		return PSY_OK;
	}
	if (!psy_tokenizer_check_delim(t, "#")) {
		return PSY_ERRFILEFORMAT;
	}
	psy_tokenizer_next(t);
	if (!psy_tokenizer_has_next(t)) {
		return PSY_OK;
	}
	if (psy_tokenizer_check_ident(t, "PLAYLIST")) {		
		playlist_parse_title(self, t);
	} else if (psy_tokenizer_check_ident(t, "EXTINF")) {
		playlist_parse_item_info(self, t, item_info);
	} else if (psy_tokenizer_check_ident(t, "EXTALB")) {
		playlist_parse_album(self, t, item_info);
	}
	return PSY_OK;
}

intptr_t playlist_parse_title(PlayList* self, psy_Tokenizer* t)
{
	assert(self);
	assert(t);

	psy_tokenizer_next(t);
	if (!psy_tokenizer_check_delim(t, ":")) {
		return PSY_ERRRUN;
	}
	psy_tokenizer_read_line(t);
	playlist_set_title(self, psy_tokenizer_lexeme(t));
	return PSY_OK;
}

intptr_t playlist_parse_item_info(PlayList* self, psy_Tokenizer* t,
	PlayListItemInfo* item_info)
{
	assert(self);
	assert(t);
	assert(item_info);	

	psy_tokenizer_next(t);
	if (!psy_tokenizer_check_delim(t, ":")) {
		return PSY_ERRRUN;
	}
	psy_tokenizer_next(t);
	if (!psy_tokenizer_check_int(t)) {
		return PSY_ERRRUN;
	}
	item_info->length_s = atoi(psy_tokenizer_lexeme(t));
	psy_tokenizer_next(t);
	if (!psy_tokenizer_check_delim(t, ",")) {
		return PSY_ERRRUN;
	}
	psy_tokenizer_read_line(t);
	playlistiteminfo_set_title(item_info, psy_tokenizer_lexeme(t));	
	return PSY_OK;
}

intptr_t playlist_parse_album(PlayList* self, psy_Tokenizer* t,
	PlayListItemInfo* item_info)
{
	assert(self);
	assert(t);
	assert(item_info);	

	psy_tokenizer_next(t);
	if (!psy_tokenizer_check_delim(t, ":")) {
		return PSY_ERRRUN;
	}
	psy_tokenizer_read_line(t);
	playlistiteminfo_set_album(item_info, psy_tokenizer_lexeme(t));	
	return PSY_OK;
}
