// File: cmd_help.h

#ifndef CMD_HELP_H
#define CMD_HELP_H

#include "command.h"

#define SIG_HELP_STR \
    "HELP ...\n"

#define HELP_HELP_STR \
    "HELP ...ATTR - Display list of commands.\n"\
    "LIST CMDS - Display list of commands.\n"\
    "LIST VARS - Display list of variables.\n"

extern CMD_COOKED_PROTO(do_help_command);

#endif // CMD_HELP_H

// End of file: cmd_help.h
