// File: cmd_test.h

#ifndef CMD_TEST_H
#define CMD_TEST_H

#include "command.h"

#define SIG_TEST_STR \
    "TEST <expected> <expr> -> BOOL\n"

#define HELP_TEST_STR \
    "TEST <expected> <expr> -> BOOL - Evaulate <expr> and return true if result matches <expected>.\n"

extern CMD_RAW_PROTO(do_test_command);

#endif // CMD_TEST_H


// End of file: cmd_test.h
