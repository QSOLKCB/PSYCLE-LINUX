/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(PARAMVIEWSTYLES_H)
#define PARAMVIEWSTYLES_H

/* container */
#include <configuration.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct ParamViewStyles
** @brief Styles for the param uis
*/
typedef struct ParamViewStyles {
	psy_Configuration* dir_cfg;
} ParamViewStyles;

void paramviewstyles_init(ParamViewStyles*, psy_Configuration* dir_cfg);

void paramviewstyles_make_default(ParamViewStyles*);
void paramviewstyles_load(ParamViewStyles*, const char* path);

typedef struct ParamStyleConfigurator {
	ParamViewStyles* styles;
	psy_Configuration* cfg;
} ParamStyleConfigurator;

void paramstyleconfigurator_init(ParamStyleConfigurator*,
	ParamViewStyles*, psy_Configuration* mac_cfg);
	
void paramstyleconfigurator_connect(ParamStyleConfigurator*);
void paramstyleconfigurator_configure(ParamStyleConfigurator*);

#ifdef __cplusplus
}
#endif

#endif /* PARAMVIEWSTYLES_H */
