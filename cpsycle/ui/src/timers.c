/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "timers.h"
/* container */
#include <list.h>
#include <signal.h>
/* std */
#include <stdlib.h>

/* time calculation */
#ifdef WIN32
#include <Windows.h>
#else
#include <sys/time.h>
#endif


/* psy_TimerTask */

/* prototypes */
static void psy_timertask_tick(psy_TimerTask*);

/* implmentation */
void psy_timertask_init(psy_TimerTask* self, uintptr_t id,
	void* context, psy_fp_timerwork timerwork, psy_Signal* timersignal,
	uintptr_t custom_id, uintptr_t interval)
{
	self->id_ = id;
	self->context_ = context;
	self->timer_work_ = timerwork;
	self->timer_signal_ = timersignal;
	self->custom_id_ = custom_id;
	self->interval_ = interval;
	self->counter_ = 0;
	self->expired_ = FALSE;
}

psy_TimerTask* psy_timertask_alloc(void)
{
	return (psy_TimerTask*)malloc(sizeof(psy_TimerTask));
}

psy_TimerTask* psy_timertask_alloc_init(uintptr_t id,
	void* context, psy_fp_timerwork timer_work, psy_Signal* timer_signal,
	uintptr_t custom_id, uintptr_t interval)
{
	psy_TimerTask* rv;
	
	rv = psy_timertask_alloc();
	if (rv) {
		psy_timertask_init(rv, id, context, timer_work, timer_signal,
			custom_id, interval);
	}	
	return rv;
}

void psy_timertask_tick(psy_TimerTask* self)
{		
	self->counter_ += 10;
	if (self->counter_ >= self->interval_) {
		self->counter_ = 0;
		if (self->timer_signal_) {
			psy_signal_emit(self->timer_signal_, self->context_, 1,
				self->custom_id_);
		}
		if (self->timer_work_) {
			self->timer_work_(self->context_, self->custom_id_);
		}		
	}
}


/* psy_Timers */

/* prototypes */
static psy_TimerTask* psy_timers_task(psy_Timers*, uintptr_t id,
	uintptr_t customid);

/* implmentation */
void psy_timers_init(psy_Timers* self)
{
	self->tasks_ = NULL;
}

void psy_timers_dispose(psy_Timers* self)
{
	psy_list_deallocate(&self->tasks_, (psy_fp_disposefunc)NULL);
}

void psy_timers_tick(psy_Timers* self)
{
	psy_List* p;
	psy_List* q;
	
	for (p = q = self->tasks_; q != NULL; p = q) {
		psy_TimerTask* task;
		
		q = p->next;
		task = (psy_TimerTask*)p->entry;
		if (psy_timertask_expired(task)) {
			psy_list_remove(&self->tasks_, p);
			free(task);			
		} else {
			psy_timertask_tick(task);
		}
	}	
}

void psy_timers_add_timer(psy_Timers* self, uintptr_t id, void* context,
	psy_fp_timerwork timer_work, psy_Signal* timer_signal,
	uintptr_t custom_id, uintptr_t interval)
{
	psy_TimerTask* task;	
	
	task = psy_timers_task(self, id, custom_id);
	if (task && !task->expired_) {		
		psy_timertask_set_interval(task, interval);
	} else {
		task = psy_timertask_alloc_init(id, context, timer_work, timer_signal,
			custom_id, interval);
		if (task) {
			psy_list_append(&self->tasks_, (void*)task);		
		}
	}
}

psy_TimerTask* psy_timers_task(psy_Timers* self, uintptr_t id,
	uintptr_t custom_id)
{
	psy_TimerTask* rv;
	psy_List* p;	
	
	rv = NULL;
	for (p = self->tasks_; p != NULL; p = p->next) {
		psy_TimerTask* task;
				
		task = (psy_TimerTask*)p->entry;
		if (psy_timertask_check(task, id, custom_id)) {
			rv = task;			
		}		
	}
	return rv;
}

void psy_timers_remove_timer(psy_Timers* self, uintptr_t id,
	uintptr_t custom_id)
{
	psy_TimerTask* task;

	task = psy_timers_task(self, id, custom_id);
	if (task) {	
		psy_timertask_expire(task);		
	}
}

/* http://stackoverflow.com/questions/1861294/how-to-calculate-execution-time-of-a-code-snippet-in-c */
uintptr_t psy_gettime_ms(void)
{
#ifdef WIN32
	/* Windows */
	FILETIME ft;
	LARGE_INTEGER li;

	/* Get the amount of 100 nano seconds intervals elapsed since January 1, 1601 (UTC) and copy it
	 * to a LARGE_INTEGER structure. */
	GetSystemTimeAsFileTime(&ft);
	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;

	uintptr_t ret = li.QuadPart;
	ret -= 116444736000000000LL; /* Convert from file time to UNIX epoch time. */
	ret /= 10000; /* From 100 nano seconds (10^-7) to 1 millisecond (10^-3) intervals */

	return ret;
#else
	/* Linux */
	struct timeval tv;

	gettimeofday(&tv, NULL);

	uintptr_t ret = tv.tv_usec;
	/* Convert from micro seconds (10^-6) to milliseconds (10^-3) */
	ret /= 1000;

	/* Adds the seconds (10^0) after converting them to milliseconds (10^-3) */
	ret += (tv.tv_sec * 1000);

	return ret;
#endif
}
