/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(PLAYLIST_H)
#define PLAYLIST_H

#include <list.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct PlayListItemInfo {
	uintptr_t length_s;
	char* title;
	char* album;	
} PlayListItemInfo;

void playlistiteminfo_init(PlayListItemInfo*, uintptr_t length_s,
	const char* title, const char* album);
void playlistiteminfo_dispose(PlayListItemInfo*);

PlayListItemInfo* playlistiteminfo_clone(const PlayListItemInfo* other);

void playlistiteminfo_set_title(PlayListItemInfo*, const char* title);
void playlistiteminfo_set_album(PlayListItemInfo*, const char* title);
void playlistiteminfo_clear(PlayListItemInfo*);
bool playlistiteminfo_empty(const PlayListItemInfo*);

typedef struct PlayListItem {
	char* path;	
	PlayListItemInfo* item_info;
} PlayListItem;

void playlistitem_init(PlayListItem*, const char* path,
	const PlayListItemInfo*);
void playlistitem_dispose(PlayListItem*);

PlayListItem* playlistitem_alloc(void);
PlayListItem* playlistitem_alloc_init(const char* path,
	const PlayListItemInfo*);
PlayListItem* playlistitem_clone(const PlayListItem* other);

/*! @struct PlayList */
typedef struct PlayList {
	psy_List* items;	
	char* path;
	char* title;	
} PlayList;

void playlist_init(PlayList*, const char* path);
void playlist_dispose(PlayList*);

intptr_t playlist_load(PlayList*);
intptr_t playlist_parse(PlayList*);
void playlist_save(PlayList*);

void playlist_append(PlayList*, const PlayListItem*);
bool playlist_exist(PlayList*, const char* filename);
void playlist_clear(PlayList*);

void playlist_set_title(PlayList*, const char* title);
const char* playlist_title(const PlayList*);

#ifdef __cplusplus
}
#endif

#endif /* PLAYLIST_H */
