/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uiprogressbar.h"

/* prototypes */
static void psy_ui_progressbar_on_draw(psy_ui_ProgressBar*, psy_ui_Graphics*);
static void psy_ui_progressbar_draw_step(psy_ui_ProgressBar*, psy_ui_Graphics*,
	psy_ui_RealPoint, bool fill);
static void psy_ui_progressbar_on_progress(psy_ui_ProgressBar*, psy_ProgressState);

/* vtable */
static psy_ui_ComponentVtable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(psy_ui_ProgressBar* self)
{
	if (!vtable_initialized) {
		vtable = *(self->component.vtable);
		vtable.ondraw =
			(psy_ui_fp_component_ondraw)
			psy_ui_progressbar_on_draw;
		vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &vtable);
}


/* logger vtable */
static psy_LoggerVTable logger_vtable;
static bool logger_initialized = FALSE;

static void logger_vtable_init(psy_ui_ProgressBar* self)
{
	if (!logger_initialized) {
		logger_vtable = *(self->logger.vtable);
		logger_vtable.progress =
			(fp_progress)
			psy_ui_progressbar_on_progress;
		logger_initialized = TRUE;
	}
	self->logger.vtable = &logger_vtable;
	self->logger.context_ = (void*)self;
}

/* implementation */
void psy_ui_progressbar_init(psy_ui_ProgressBar* self,
	psy_ui_Component* parent)
{	
	psy_ui_component_init(&self->component, parent, NULL);
	vtable_init(self);
	psy_logger_init(&self->logger);
	logger_vtable_init(self);	
	psy_ui_component_set_style_type(&self->component, psy_ui_STYLE_PROGRESSBAR);
	psy_ui_component_set_preferred_size(&self->component,
		psy_ui_size_make_em(10.0, 0.0));
	self->progress = 0.0;
}

void psy_ui_progressbar_on_draw(psy_ui_ProgressBar* self, psy_ui_Graphics* g)
{	
	if (self->progress > 0.0) {
		psy_ui_RealSize size;
		psy_ui_RealSize step_size;
		psy_ui_RealPoint pt;
		double range;
		uintptr_t steps;
		uintptr_t i;
		const psy_ui_TextMetric* tm;		

		tm = psy_ui_component_textmetric(&self->component);
		step_size = psy_ui_realsize_make(2.0 * tm->tmAveCharWidth, 0.2 * tm->tmHeight);
		size = psy_ui_component_size_px(&self->component);
		steps = (uintptr_t)(size.width / step_size.width);
		range = self->progress * size.width;
		for (i = 0; i < steps; ++i) {
			pt = psy_ui_realpoint_make(i * step_size.width, 0.0);
			psy_ui_progressbar_draw_step(self, g, pt, (pt.x < range));			
		}
	}
}

void psy_ui_progressbar_draw_step(psy_ui_ProgressBar* self, psy_ui_Graphics* g,
	psy_ui_RealPoint pt, bool fill)
{
	psy_ui_RealPoint pts[5];
	psy_ui_RealSize size;
	const psy_ui_TextMetric* tm;
	double ident;

	tm = psy_ui_component_textmetric(&self->component);
	size = psy_ui_realsize_make(2.0 * tm->tmAveCharWidth, psy_max(1.0, 0.2 * tm->tmHeight));
	ident = psy_max(1.0, floor(size.width / 4.0));
	pts[0] = psy_ui_realpoint_make(0.0 + pt.x, size.height + pt.y);
	pts[1] = psy_ui_realpoint_make(ident + pt.x, 0.0 + pt.y);
	pts[2] = psy_ui_realpoint_make(size.width + pt.x, 0.0 + pt.y);
	pts[3] = psy_ui_realpoint_make(size.width - ident + pt.x, size.height + pt.y);
	pts[4] = psy_ui_realpoint_make(0.0 + pt.x, size.height + pt.y);
	if (fill) {
		psy_ui_graphics_draw_solid_polygon(g, pts, 5,
			psy_ui_component_colour(progressbar_base(self)),
			psy_ui_component_colour(progressbar_base(self)));
	}
}

void psy_ui_progressbar_set_progress(psy_ui_ProgressBar* self, double progress)
{
	self->progress = progress;
	psy_ui_component_invalidate(progressbar_base(self));
	psy_ui_component_update(progressbar_base(self));
}

void psy_ui_progressbar_tick(psy_ui_ProgressBar* self)
{
	if (self->progress + 0.05 > 1.0) {
		self->progress = 0.0;
	}
	psy_ui_progressbar_set_progress(self, self->progress + 0.05);	
}

void psy_ui_progressbar_on_progress(psy_ui_ProgressBar* self, psy_ProgressState state)
{
	switch (state) {
	case PSY_PROGRESS_STATE_TICK:
		psy_ui_progressbar_tick(self);
		break;
	case PSY_PROGRESS_STATE_END:
		psy_ui_progressbar_set_progress(self, 0.0);
		break;
	default:
		break;
	}
	
}
