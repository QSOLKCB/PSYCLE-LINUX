// This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
// copyright 2000-2021 members of the psycle project http://psycle.sourceforge.net

#include "../../detail/prefix.h"
#include "../../detail/os.h"

#include "interpreter.h"

int interpreter_init(Interpreter* self, Workspace* workspace)
{
    self->workspace = workspace;
    return 0;
}

int interpreter_start(Interpreter* self)
{
	psy_Logger* logger;
		
	logger = self->workspace->terminal_output;
	if (!logger) {					
		return 0;			
	}
    psy_logger_output(logger, PSYCLE__TITLE);
    psy_logger_output(logger, "\n");
    psy_logger_output(logger, PSYCLE__COPYRIGHT);
    psy_logger_output(logger, "\n");
    psy_logger_output(logger, "ready.\n");
#if !defined(PSYCLE_USE_MACHINEPROXY)
    psy_logger_output(logger, "NO MACHINEPROXY PROTECTION\n");
#endif
    return 0;
}

void interpreter_dispose(Interpreter* self)
{    
}

int interpreter_onidle(Interpreter* self)
{
    return 0;
}
