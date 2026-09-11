/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(MACHINEPARAMCONFIG_H)
#define MACHINEPARAMCONFIG_H

/* ui */
#include <uifont.h>
/* container */
#include <configuration.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PARAMWIDTH 26.0
#define PARAMWIDTH_SMALL 18.0

struct psy_DiskOp;

/*!
** @struct MachineParamConfig
** @brief Configures the machine parameter uis
*/
typedef struct MachineParamConfig {
	/*! @implements */
	psy_CustomConfiguration configuration;
    /*! @internal */	
	struct psy_Property* theme;	
    /* references */	
	struct psy_DiskOp* disk_op;
} MachineParamConfig;

void machineparamconfig_init(MachineParamConfig*, struct psy_Property* parent,
	struct psy_DiskOp*);
void machineparamconfig_dispose(MachineParamConfig*);


INLINE psy_Configuration* machineparamconfig_base(MachineParamConfig* self)
{
	assert(self);

	return psy_customconfiguration_base(&self->configuration);
}

#ifdef __cplusplus
}
#endif

#endif /* MACHINEPARAMCONFIG_H */
