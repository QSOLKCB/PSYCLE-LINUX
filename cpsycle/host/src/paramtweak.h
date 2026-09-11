/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(PARAMTWEAK_H)
#define PARAMTWEAK_H

#include "../../detail/psydef.h"


#ifdef __cplusplus
extern "C" {
#endif

/* Forward Declarations */

struct psy_audio_Machine;
struct psy_audio_MachineParam;
struct psy_ui_MouseEvent;

/*!
** @struct ParamTweak
** @brief MouseEvent Helper for psy_audio_MachineParam ui components
*/
typedef struct ParamTweak {
    /*! @internal */
    double tweak_base_;
    double tweak_val_;
    double tweak_scale_;
    bool active_;
    uintptr_t param_idx_;
    /* references */
    struct psy_audio_Machine* machine_;
    struct psy_audio_MachineParam* param_;    
} ParamTweak;

void paramtweak_init(ParamTweak*);

/*
** activate tweak
**      Either machine and paramidx or machineparam needs to be set
**      machineparam is used only if machine or paramidx are not valid
*/
void paramtweak_begin(ParamTweak*, struct psy_audio_Machine*,
    uintptr_t param_index, struct psy_audio_MachineParam*);
/* deactivate tweak */
void paramtweak_end(ParamTweak*);
/* called by the uis to delegate the mouse events */
void paramtweak_on_mouse_down(ParamTweak*, struct psy_ui_MouseEvent*);
void paramtweak_on_mouse_move(ParamTweak*, struct psy_ui_MouseEvent*);
/* return tweak status */
INLINE bool paramtweak_active(const ParamTweak* self)
{
    return self->active_;
}

#ifdef __cplusplus
}
#endif

#endif /* PARAMTWEAK_H */
