// File: cmd_print.c

#include "common.h"
#include "cmd_print.h"

// #include <string.h>

#include "print.h"
// #include "shell.h"


//==========================================================================================================================
//
PUBLIC const atom_t* do_print_command(context_t* context, const command_t* cmd, size_t argc, const atom_t* argv)
{
    UNUSED(context);
    UNUSED(cmd);
    UNUSED(argc);

    if (Balloc_Reserve(context->heap))
    {
        char* msg = Balloc_Top(context->heap);
        char* msgp = msg;
        size_t len = Balloc_Left(context->heap);

        Print_Atom(argv, &msgp, &len, true);
        Balloc_Release(context->heap);

        len = strlen(msg) + 1;           // Including the null.

        if (len > 1) 
        {
            Balloc(context->heap, len);
            DBG(0, DEBUG_COMMAND, "%s\n", msg);
            return Result_CString(context, msg);
        }
    }

    return NULL;
}


// End of file: cmd_print.c
