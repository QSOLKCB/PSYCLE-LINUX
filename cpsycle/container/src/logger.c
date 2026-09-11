/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "logger.h"
/* platform */
#include "../../detail/portable.h"


static void logger_output(psy_Logger* self, const char* str)
{
}

static int logger_status(const psy_Logger* self)
{
	return PSY_OK;
}

static void logger_progress(psy_Logger* self, psy_ProgressState state)
{	
}

/* logger_vtable */
static psy_LoggerVTable logger_vtable;
static bool logger_vtable_initialized = FALSE;

static void logger_vtable_init(void)
{
	if (!logger_vtable_initialized) {		
		logger_vtable.output =
		logger_vtable.warn =
		logger_vtable.error =
			(fp_string_output)
			logger_output;
		logger_vtable.status =
			(fp_logger_status)
			logger_status;
		logger_vtable.progress =
			(fp_progress)
			logger_progress;
		logger_vtable_initialized = TRUE;
	}
}

/* implementation */
void psy_logger_init(psy_Logger* self)
{	
	logger_vtable_init();
	self->vtable = &logger_vtable;
	self->context_ = NULL;
}


/* psy_Loggers */

/* prototypes */
static void psy_loggers_on_output(psy_Loggers*, const char* str);
static void psy_loggers_on_warn(psy_Loggers*, const char* str);
static void psy_loggers_on_error(psy_Loggers*, const char* str);
static void psy_loggers_on_progress(psy_Loggers* loggers, psy_ProgressState);

/* logger vtable */
static psy_LoggerVTable loggers_vtable;
static bool loggers_initialized = FALSE;

static void loggers_vtable_init(psy_Loggers* self)
{
	if (!loggers_initialized) {
		loggers_vtable = *(self->logger.vtable);
		loggers_vtable.output =
			(fp_string_output)
			psy_loggers_on_output;
		loggers_vtable.warn =
			(fp_string_output)
			psy_loggers_on_warn;
		loggers_vtable.error =
			(fp_string_output)
			psy_loggers_on_error;
		loggers_vtable.progress =
			(fp_progress)
			psy_loggers_on_progress;
		loggers_initialized = TRUE;
	}
	self->logger.vtable = &loggers_vtable;
	self->logger.context_ = self;
}

void psy_loggers_init(psy_Loggers* self)
{
	assert(self);

	psy_logger_init(&self->logger);
	loggers_vtable_init(self);		
	self->container = NULL;
}

void psy_loggers_dispose(psy_Loggers* self)
{
	assert(self);

	psy_list_free(self->container);
	self->container = NULL;
}

void psy_loggers_add(psy_Loggers* self, psy_Logger* logger)
{
	assert(self);

	if (logger) {
		psy_list_append(&self->container, (void*)logger);
	}
}

void psy_loggers_on_output(psy_Loggers* self, const char* str)
{
	psy_List* p;

	assert(self);

	for (p = self->container; p != NULL; p = p->next) {
		psy_Logger* logger;

		logger = (psy_Logger*)p->entry;
		psy_logger_output(logger, str);
	}
}

void psy_loggers_on_warn(psy_Loggers* self, const char* str)
{
	psy_List* p;

	assert(self);

	for (p = self->container; p != NULL; p = p->next) {
		psy_Logger* logger;

		logger = (psy_Logger*)p->entry;
		psy_logger_warn(logger, str);
	}
}

void psy_loggers_on_error(psy_Loggers* self, const char* str)
{
	psy_List* p;

	assert(self);

	for (p = self->container; p != NULL; p = p->next) {
		psy_Logger* logger;

		logger = (psy_Logger*)p->entry;
		psy_logger_error(logger, str);
	}
}

void psy_loggers_on_progress(psy_Loggers* self, psy_ProgressState state)
{
	psy_List* p;

	assert(self);

	for (p = self->container; p != NULL; p = p->next) {
		psy_Logger* logger;

		logger = (psy_Logger*)p->entry;
		psy_logger_progress(logger, state);
	}
}
