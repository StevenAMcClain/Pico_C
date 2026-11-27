// File: cmd_print.h

#ifndef CMD_PRINT_H
#define CMD_PRINT_H

#include "command.h"

#define SIG_PRINT_STR \
    "PRINT [<expr> ...]\n"

#define HELP_PRINT_STR \
    "PRINT...\n"

extern CMD_COOKED_PROTO(do_print_command);

#endif // CMD_PRINT_H

// End of file: cmd_print.h
