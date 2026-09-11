/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(INPUTHANDLER_H)
#define INPUTHANDLER_H


/* driver */
#include "../../driver/eventdriver.h"
/* container */
#include <list.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*fp_inputhandler_input)(void* context, void* sender);
typedef bool (*fp_inputhandler_hostcallback)(void* context, int message, void* param1);

#define INPUTHANDLER_STOP TRUE;
#define INPUTHANDLER_CONTINUE FALSE;

typedef enum {
	INPUTHANDLER_IMM = 1,
	INPUTHANDLER_FOCUS = 2	
} InputHandlerType;

typedef enum {
	INPUTHANDLER_HASFOCUS	
} InputHandlerMessage;

/* InputSlot */
typedef struct InputSlot {
	InputHandlerType type;
	psy_EventDriverCmdType cmdtype;
	uintptr_t id;	
	void* context;
	char* section;
	void* focus;
	fp_inputhandler_input input;
} InputSlot;

void inputslot_init(InputSlot*, InputHandlerType, psy_EventDriverCmdType,
	const char* section, uintptr_t id, void* context, void* focus,
	fp_inputhandler_input);
void inputslot_dispose(InputSlot*);

InputSlot* inputslot_alloc(void);
InputSlot* inputslot_alloc_init(InputHandlerType type,
	psy_EventDriverCmdType, const char* section,
	uintptr_t id, void* context, void* focus, fp_inputhandler_input);

/*!
** @struct InputHandler
** @brief Distributes event driver commands to the host components
*/
typedef struct InputHandler {
	/*! @internal */
	psy_List* slots_;
	psy_EventDriverCmd cmd_;
	psy_EventDriverInput input_;
	void* hostcontext_;
	fp_inputhandler_hostcallback hostcallback_;
	psy_EventDriver* sender_;
} InputHandler;

void inputhandler_init(InputHandler*);
void inputhandler_dispose(InputHandler*);

void inputhandler_connect(InputHandler*, InputHandlerType type,
	psy_EventDriverCmdType cmdtype, const char* section,
	uintptr_t id, void* context, void* focus, fp_inputhandler_input);
void inputhandler_disconnect(InputHandler*, void* context, fp_inputhandler_input);
void inputhandler_connect_host(InputHandler*, void* context,
	fp_inputhandler_hostcallback);
psy_EventDriverCmd inputhandler_cmd(const InputHandler*);
psy_EventDriverInput inputhandler_input(const InputHandler*);
psy_EventDriver* inputhandler_sender(const InputHandler*);
InputSlot* inputhandler_at(InputHandler*, void* context, fp_inputhandler_input);

void inputhandler_event_driver_input(InputHandler*, psy_EventDriver*);
void inputhandler_send(InputHandler*, const char* section, psy_EventDriverCmd);

#ifdef __cplusplus
}
#endif

#endif /* INPUTHANDLER_H */
