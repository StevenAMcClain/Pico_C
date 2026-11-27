// File: cmd_watch.h

#ifndef CMD_WATCH_H
#define CMD_WATCH_H

#include "command.h"

#define SIG_WATCH_STR \
    "WATCH....\n"\

#define HELP_WATCH_STR \
    "WATCH.... ATTR - Display list of commands.\n"\
    "LIST CMDS - Display list of commands.\n"\
    "LIST VARS - Display list of variables.\n"

extern CMD_RAW_PROTO(do_watch_command);

#endif // CMD_WATCH_H

// End of file: cmd_watch.h
