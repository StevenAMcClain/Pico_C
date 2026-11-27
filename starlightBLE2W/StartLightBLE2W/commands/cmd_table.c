// File: cmd_misc.c

#include "common.h"
#include "cmd_table.h"

#include "cmd_help.h"
#include "cmd_if.h"
#include "cmd_list.h"
#include "cmd_print.h"
#include "cmd_parms.h"
#include "cmd_return.h"
#include "cmd_srvs.h"
#include "cmd_test.h"
#include "cmd_watch.h"


PUBLIC CMD_COOKED_PROTO(do_check_command);
#define SIG_CHECK_STR "CHECK"
#define HELP_CHECK_STR "CHECK"

PUBLIC CMD_COOKED_PROTO(do_config_command);
#define SIG_CONFIG_STR "CONFIG"
#define HELP_CONFIG_STR "CONFIG"

PUBLIC CMD_COOKED_PROTO(do_load_command);
#define SIG_LOAD_STR "LOAD"
#define HELP_LOAD_STR "LOAD"

// num_params == 0 passes params == NULL. Every command function must guard for this.

PRIVATE command_t cmd_list[] = 
{ 
    {"CHECK",  ALT(CMD_CODE_CHECK ), 1, 1, do_check_command,   NIL,                    NIL, false, SIG_CHECK_STR,  HELP_CHECK_STR},
    {"CONFIG", ALT(CMD_CODE_CONFIG), 1, 1, do_config_command,  NIL,                    NIL, false, SIG_CONFIG_STR, HELP_CONFIG_STR},
    {"HELP",   ALT(CMD_CODE_HELP  ), 0, 1, do_help_command,    NIL,                    NIL, false, SIG_HELP_STR,   HELP_HELP_STR},
    {"IF",     ALT(CMD_CODE_IF    ), 2, 3,                     NIL, do_if_command,     NIL, true,  SIG_IF_STR,     HELP_IF_STR},
    {"LIST",   ALT(CMD_CODE_LIST  ), 0, 3, do_list_command,    NIL,                    NIL, false, SIG_LIST_STR,   HELP_LIST_STR},
    {"LOAD",   ALT(CMD_CODE_LOAD  ), 1, 1, do_load_command,    NIL,                    NIL, false, SIG_LOAD_STR,   HELP_LOAD_STR},
    {"PARMS",  ALT(CMD_CODE_PARMS ), 0, 10,                    NIL, do_parms_command,  NIL, true,  SIG_PARMS_STR,  HELP_PARMS_STR},
    {"PRINT",  ALT(CMD_CODE_PRINT ), 0, 10, do_print_command,  NIL,                    NIL, false, SIG_PRINT_STR,  HELP_PRINT_STR},
    {"RETURN", ALT(CMD_CODE_RETURN), 0, 10, do_return_command, NIL,                    NIL, false, SIG_RETURN_STR, HELP_RETURN_STR},
    {"SRVS",   ALT(CMD_CODE_SRVS  ), 1, 10,                    NIL, do_srvs_command,   NIL, true,  SIG_SRVS_STR,   HELP_SRVS_STR},
    {"TEST",   ALT(CMD_CODE_IF    ), 1, 2,                     NIL, do_test_command,   NIL, true,  SIG_TEST_STR,   HELP_TEST_STR},
    {"WATCH",  ALT(CMD_CODE_WATCH ), 0, 2,                     NIL, do_watch_command,  NIL, true,  SIG_WATCH_STR,  HELP_WATCH_STR},
};


PUBLIC command_array_t Command_Table = { ARRAY_SIZE(cmd_list), cmd_list };


// End of file: command_table.c
