/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_LOGGER_H
#define psy_LOGGER_H

#include "../../detail/psydef.h"

#include "list.h"
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	PSY_PROGRESS_STATE_TICK,
	PSY_PROGRESS_STATE_END
} psy_ProgressState;

typedef void (*fp_string_output)(void*, const char*);
typedef int (*fp_logger_status)(const void*);
typedef void (*fp_progress)(void*, psy_ProgressState);

typedef struct psy_LoggerVTable {
	fp_string_output output;
	fp_string_output warn;
	fp_string_output error;
	fp_logger_status status;
	fp_progress progress;
} psy_LoggerVTable;

/*!
** @struct psy_Logger
** @brief Abstract logger class
*/
typedef struct psy_Logger {
	psy_LoggerVTable* vtable;
	void* context_;
} psy_Logger;

void psy_logger_init(psy_Logger*);

INLINE void psy_logger_output(psy_Logger* self, const char* text)
{
	assert(self);

	self->vtable->output(self->context_, text);
}

INLINE void psy_logger_warn(psy_Logger* self, const char* text)
{
	assert(self);

	self->vtable->output(self->context_, text);
}

INLINE void psy_logger_error(psy_Logger* self, const char* text)
{
	assert(self);

	self->vtable->error(self->context_, text);
}

INLINE int psy_logger_status(const psy_Logger* self)
{
	assert(self);

	return self->vtable->status(self->context_);
}

INLINE void psy_logger_progress(psy_Logger* self, psy_ProgressState state)
{
	assert(self);

	self->vtable->progress(self->context_, state);
}

/*!
** @struct psy_Loggers
** @brief Combines multiple loggers
*/
typedef struct psy_Loggers {
	psy_Logger logger;
	/* @internal */
	psy_List* container;
} psy_Loggers;

void psy_loggers_init(psy_Loggers*);
void psy_loggers_dispose(psy_Loggers*);

void psy_loggers_add(psy_Loggers*, psy_Logger*);

INLINE psy_Logger* psy_loggers_base(psy_Loggers* self)
{
	assert(self);

	return &self->logger;
}

#ifdef __cplusplus
}
#endif

#endif /* psy_LOGGER_H */
