// File: cmd_return.c

#include "common.h"
#include "cmd_return.h"

// #include <string.h>

// #include "print.h"
// #include "shell.h"


//==========================================================================================================================
//
PUBLIC const atom_t* do_return_command(context_t* context, const command_t* cmd, size_t argc, const atom_t* argv)
{
    UNUSED(cmd);

    if (argc > 1) { return Result_Error(context, "RETURN only takes 1 argument!"); }

    if (!argc) { return NULL; }

    return argv;
}


// End of file: cmd_return.c
