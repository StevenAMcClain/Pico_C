// File: cmd_parms.c

#include "common.h"
#include "cmd_parms.h"

#include <string.h>
#include "print.h"
#include "shell.h"

//==========================================================================================================================
//
PUBLIC const atom_t* do_parms_command(context_t* context, const command_t* cmd, size_t argc, const atom_t** argv)
{
    UNUSED(context);
    UNUSED(cmd);

    if (Balloc_Reserve(context->heap))
    {
        char* msg = Balloc_Top(context->heap);
        char* msgp = msg;
        size_t len = Balloc_Left(context->heap);

        BUFF_WRITER_WRITE(&msgp, &len, "PARMS: %zu\n", argc);

        for (size_t i = 0; i < argc && len > 1; ++i)
        {
            BUFF_WRITER_WRITE(&msgp, &len, "arg[%zu]\n", i);
            Dump_Expr(argv[i], &msgp, &len);
        }

        len = strlen(msg) + 1;           // Including the null.

        Balloc_Release(context->heap);

        if (len > 1) 
        {
            Balloc(context->heap, len);
            DBG(0, DEBUG_COMMAND, "%s\n", msg);
            return Result_CString(context, msg);
        }
    }

    return NULL;
}


// End of file: cmd_parms.c
