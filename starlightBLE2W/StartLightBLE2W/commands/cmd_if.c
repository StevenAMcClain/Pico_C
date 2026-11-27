// File: cmd_if.c

#include "common.h"
#include "cmd_if.h"

#include <string.h>

#include "print.h"
#include "shell.h"


//==========================================================================================================================
//
PUBLIC const atom_t* do_if_command(context_t* context, const command_t* cmd, size_t argc, const atom_t** argv)
{
    UNUSED(cmd);

    if (argc < 2)
    {
        return Result_Error(context, "IF requires at least 2 arguments");
    }

    const atom_t* cond = Eval_Expression(context, argv[0]);

    if (!cond) { return NULL; }

    uint32_t is_true = false;
    
    if (!Atom_To_Bool(cond, &is_true)) { return NULL; }

    if (is_true)
    {
        return Eval_Expression(context, argv[1]);
    }

    if (argc >= 3)
    {
        return Eval_Expression(context, argv[2]);
    }

    return Result_Null(context);
}


// End of file: cmd_if.c
