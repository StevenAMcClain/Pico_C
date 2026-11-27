// File: cmd_list.h

#ifndef CMD_LIST_H
#define CMD_LIST_H

#include "command.h"

#define SIG_LIST_STR "LIST [ATTR | CMDS | VARS]"

#define HELP_LIST_STR \
    "LIST ATTR - Display list of Attr Servers.\n"\
    "LIST CMDS - Display list of commands.\n"\
    "LIST VARS - Display list of variables."

extern CMD_COOKED_PROTO(do_list_command);

#endif // CMD_LIST_H

// End of file: cmd_list.h
