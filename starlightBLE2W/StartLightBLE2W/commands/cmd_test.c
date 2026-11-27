// File: cmd_test.c

#include "common.h"
#include "cmd_test.h"

#include <string.h>
#include "print.h"
#include "shell.h"


//==========================================================================================================================
//
PUBLIC const atom_t* do_test_command(context_t* context, const command_t* cmd, size_t argc, const atom_t** argv)
{
    UNUSED(cmd);

    if (argc != 2) { return Result_Error(context, "TEST requires exactly 2 arguments"); }

    const atom_t* lhs = Eval_Expression(context, argv[0]);

    if (!lhs) { return NULL; }

    const atom_t* rhs = Eval_Expression(context, argv[1]);

    if (!rhs) { return NULL; }

    return Result_Bool(context, Atom_Equals(lhs, rhs));
}


// End of file: cmd_test.c
