// File: cmd_return.h

#ifndef CMD_RETURN_H
#define CMD_RETURN_H

#include "command.h"

#define SIG_RETURN_STR \
    "RETURN [<expr>]\n"

#define HELP_RETURN_STR \
    "PRINT...\n"

extern CMD_COOKED_PROTO(do_return_command);

#endif // CMD_RETURN_H

// End of file: cmd_return.h
