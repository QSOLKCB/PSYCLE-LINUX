/* This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_TIMERS_H
#define psy_TIMERS_H

#include "../../detail/psydef.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*psy_fp_timerwork)(void*, uintptr_t);

struct psy_List;
struct psy_Signal;

typedef struct psy_TimerTask {
	/*! @internal */
	uintptr_t id_;	
	void* context_;
	psy_fp_timerwork timer_work_;
	struct psy_Signal* timer_signal_;
	uintptr_t custom_id_;
	uintptr_t interval_;
	uintptr_t counter_;
	bool expired_;	
} psy_TimerTask;

void psy_timertask_init(psy_TimerTask*, uintptr_t id,
	void* context, psy_fp_timerwork, struct psy_Signal* timer_signal,
	uintptr_t custom_id, uintptr_t interval);

psy_TimerTask* psy_timertask_alloc(void);
psy_TimerTask* psy_timertask_alloc_init(uintptr_t id,
	void* context, psy_fp_timerwork, struct psy_Signal* timer_signal,
	uintptr_t custom_id, uintptr_t interval);

INLINE bool psy_timertask_expired(const psy_TimerTask* self)
{
	return self->expired_;
}

INLINE void psy_timertask_expire(psy_TimerTask* self)
{
	self->expired_ = TRUE;
}

INLINE void psy_timertask_set_interval(psy_TimerTask* self, uintptr_t interval)
{
	self->interval_ = interval;
}

INLINE bool psy_timertask_check(const psy_TimerTask* self, uintptr_t id,
	uintptr_t custom_id)
{
	return ((self->id_ == id) && ((custom_id == psy_INDEX_INVALID) ||
		(self->custom_id_ == custom_id)));
}

typedef struct psy_Timers {	
	/*! @internal */
	struct psy_List* tasks_;	
} psy_Timers;

void psy_timers_init(psy_Timers*);
void psy_timers_dispose(psy_Timers*);

void psy_timers_tick(psy_Timers*);
void psy_timers_add_timer(psy_Timers*, uintptr_t id, void* context,
	psy_fp_timerwork timer_work, struct psy_Signal* timer_signal,
	uintptr_t custom_id, uintptr_t interval);
void psy_timers_remove_timer(psy_Timers*, uintptr_t id, uintptr_t custom_id);

uintptr_t psy_gettime_ms(void);

#ifdef __cplusplus
}
#endif

#endif /* psy_TIMERS_H */
