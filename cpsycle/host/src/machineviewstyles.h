/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(MACHINEVIEWSTYLES_H)
#define MACHINEVIEWSTYLES_H

/* container */
#include <configuration.h>


#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct MachineViewStyles
** @brief Styles for the machine uis in the machineview
*/
typedef struct MachineViewStyles {
	psy_Configuration* dir_cfg;
} MachineViewStyles;

void machineviewstyles_init(MachineViewStyles*, psy_Configuration* dir_cfg);

void machineviewstyles_make_view_default(MachineViewStyles*);
void machineviewstyles_make_machine_default_em(MachineViewStyles*);
void machineviewstyles_make_machine_default(MachineViewStyles*);
void machineviewstyles_load(MachineViewStyles*, const char* skin_name);


/*!
** @struct MachineStyleConfigurator
** @brief Updates the machine ui styles to the machine view configuration
*/
typedef struct MachineStyleConfigurator {
	/*! @internal */
	MachineViewStyles* styles;
	psy_Configuration* cfg;
} MachineStyleConfigurator;

void machinestyleconfigurator_init(MachineStyleConfigurator*,
	MachineViewStyles*, psy_Configuration* mac_cfg);
	
void machinestyleconfigurator_connect(MachineStyleConfigurator*);
void machinestyleconfigurator_configure(MachineStyleConfigurator*);

#ifdef __cplusplus
}
#endif

#endif /* MACHINEVIEW_H */
