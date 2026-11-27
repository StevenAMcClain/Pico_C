// File: cmd_parms.h

#ifndef CMD_PARMS_H
#define CMD_PARMS_H

#include "command.h"

#define SIG_PARMS_STR \
    "PARMS [<expr> ...]\n"

#define HELP_PARMS_STR \
    "PARMS ...ATTR - Display list of commands.\n"\
    "LIST CMDS - Display list of commands.\n"\
    "LIST VARS - Display list of variables.\n"

extern CMD_RAW_PROTO(do_parms_command);

#endif // CMD_PARMS_H

// End of file: cmd_parms.h
