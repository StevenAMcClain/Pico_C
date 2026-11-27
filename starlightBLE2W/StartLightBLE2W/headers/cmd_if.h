// File: cmd_if.h

#ifndef CMD_IF_H
#define CMD_IF_H

#include "command.h"

#define SIG_IF_STR \
    "IF <predicate> <true_expr> <false_expr>\n"

#define HELP_IF_STR \
    "IF <predicate> <true_expr> <false_expr>\n"

extern CMD_RAW_PROTO(do_if_command);

#endif // CMD_IF_H

// End of file: cmd_test.h
